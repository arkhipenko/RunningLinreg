# Running Linreg

### Least-squares line over the last n points, updated in constant time
##### Version 2.0.1

### Description

`RunningLinreg` fits the line `value = slope * t + intercept` to the last `n` points added, using ordinary least squares (the line with the smallest mean squared error). Every time a point is added, `slope` and `intercept` are updated at a cost that does not depend on `n`.

### How it works

Imagine you are fitting a line to the last 200 measurements of a time series. Each new measurement is added on the right, and once there are 200, the oldest one drops off on the left. Only two points change at each step, the one added and the one dropped, so the library does not refit all 200 points. It keeps running sums and adjusts them for those two points.

Two details keep the result accurate over long runs:

- The sums are kept relative to a reference point inside the window, not from `t = 0`. Large time stamps, such as `millis()` after days of uptime or Unix time, would otherwise cancel out and give a wrong slope.
- A fresh set of sums is started alongside the running one. Once every point in the window has been added to it, which happens every `n` points, it replaces the running set. Rounding error from adding and subtracting points therefore never builds up.

### Requirements

- The RngBuffer library, version 2.0.0 or later (github.com/arkhipenko/RngBuffer). It holds the window of points. PlatformIO installs it from GitHub with RunningLinreg (declared in `library.json`). With the Arduino IDE, download it from GitHub and copy its folder into your `libraries` folder.
- A C++11 compiler (the default for Arduino cores).

### Usage

```c++
#include <RunningLinreg.h>

RunningLinreg trend(50);                 // window of the last 50 points

void loop() {
  double t = millis() / 1000.0;          // time in seconds
  trend.addPoint({ t, (double) analogRead(A0) });
  if ( trend.status() == LINREG_OK ) {
    Serial.println(trend.slope());       // change per second
  }
}
```

The window can also live in an array you provide, so that no heap is used:

```c++
point_t storage[30];
RunningLinreg battery(30, storage);      // the array must outlive the object
```

See `examples/SensorTrend` and `examples/TimeToEmpty`.

### API

```c++
RunningLinreg(size_t n);                     // window of n points (below 2 means 2), storage from malloc
RunningLinreg(size_t n, point_t* storage);   // storage for n points (at least 2) provided by the caller

void    addPoint(point_t point);             // add a point; the oldest one drops off when the window is full
void    initialize();                        // drop all points

double  slope();                             // 0 unless status() is LINREG_OK
double  intercept();                         // 0 unless status() is LINREG_OK
size_t  count();                             // points in the window now
size_t  window();                            // window size n
bool    full();                              // count() == window()
int16_t status();                            // result of the last call
```

The point definition is:

```c++
typedef struct {
  double  t;
  double  value;
} point_t;
```

`status()` codes:

| Code | Value | Meaning |
|---|---|---|
| `LINREG_OK`  |  0 | `slope()` and `intercept()` are valid |
| `LINREG_ERR` | -1 | a storage call failed (a `rng_buffer_lock()` you supplied timed out); the point was not added. The window is kept, but can be one point shorter |
| `LINREG_NUL` | -2 | no storage: allocation failed, or the caller storage is NULL or holds fewer than 2 points |
| `LINREG_DEN` | -3 | every point in the window has the same `t`, so no single line fits |
| `LINREG_FEW` | -4 | fewer than 2 points in the window |

The object cannot be copied, because it owns its window.

### Choosing t

- `t` values do not have to be evenly spaced.
- On AVR boards (Uno, Nano, Mega) `double` is the same as `float`: about 7 significant digits. `t` as raw `millis()` loses millisecond resolution after about 4.6 hours. Time in seconds since start-up, as in the examples, keeps 0.01 s resolution for about 1.5 days. On boards with a 64-bit `double` (ESP32, most ARM boards) this is not a concern.
- `millis()` wraps to 0 after about 49.7 days. A jump back in `t` gives a wrong line until the window has moved past it. If your sketch runs that long, compute `t` from a time base that does not wrap, or call `initialize()` when it does.

### Memory

The window takes `n * sizeof(point_t)` bytes: 8 bytes per point on AVR and 16 on most 32-bit boards. A 100-point window is 800 bytes on an Uno, which has 2048 bytes of RAM in total.

### Migrating from 1.x

- The constructor no longer takes a `std::queue`. Pass the window size, and optionally your own `point_t` array. 1.x did not compile on AVR boards, because `<queue>` is not available there.
- `count()` returns `size_t` instead of `double`.
- The first point reports `LINREG_FEW`, not `LINREG_DEN`. `LINREG_ERR` is now used, for storage failures.
- 1.x lost accuracy with large `t` values and over long runs. 2.0 results can differ from 1.x for that reason.

### Practical applications

- Rate of change of a noisy sensor
- Trend and peak detection
- Predicting when a value crosses a limit (see `examples/TimeToEmpty`)
- PID control

**Enjoy!**


###### Changelog:

v2.0.1:

- 2026-09-28
- RngBuffer declared as a PlatformIO dependency in `library.json`, taken from GitHub at tag v2.0.0

v2.0.0:

- 2026-09-27
- accurate with large `t` and over long runs (sums relative to a moving reference, refreshed every n points)
- window kept in the RngBuffer library instead of a caller-owned `std::queue`: builds on AVR
- new `window()`, `full()` and status `LINREG_FEW`; `count()` returns `size_t`
- window size below 2 no longer causes undefined behavior
- examples, host unit tests and CI

v1.0.0:

- 2022-06-08 - initial release
