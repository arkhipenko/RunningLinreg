//
// Running linear regression: the least-squares line value = slope * t + intercept
// over the last n points added, updated in constant time per point.
//
// The window of points is kept in a RngBuffer (RngBuffer library, 2.0.0 or later).
// Storage is n * sizeof(point_t) bytes: 8 per point on AVR, where double is
// 32-bit, and 16 on most 32-bit boards. It comes from malloc once in the
// constructor, or from a caller-provided array that must outlive the object.
//
#ifndef RUNNINGLINREG_H
#define RUNNINGLINREG_H

#include <stddef.h>
#include <stdint.h>
#include <RngBuffer.h>

#if !defined(RNG_BUFFER_VERSION) || RNG_BUFFER_VERSION < 20000
#error "RunningLinreg needs RngBuffer 2.0.0 or later"
#endif

// status() codes
enum {
  LINREG_OK  =  0,
  LINREG_ERR = -1,  // storage call failed (a rng_buffer_lock() you supplied timed out); the point was not added
  LINREG_NUL = -2,  // no storage: allocation failed, or caller storage is NULL or holds fewer than 2 points
  LINREG_DEN = -3,  // all points in the window have the same t: slope undefined
  LINREG_FEW = -4   // fewer than 2 points in the window
};

typedef struct {
  double  t;
  double  value;
} point_t;


class RunningLinreg {

  public:
    explicit RunningLinreg(size_t n);           // window of n points (below 2 means 2), storage from malloc
    RunningLinreg(size_t n, point_t* storage);  // storage for n points (at least 2) provided by the caller
    ~RunningLinreg();

    // Not copyable: the object owns its window.
    RunningLinreg(const RunningLinreg&) = delete;
    RunningLinreg& operator=(const RunningLinreg&) = delete;

    void    addPoint(point_t point);
    void    initialize();                           // drop all points

    double  slope() const { return m_slope; }         // 0 unless status() is LINREG_OK
    double  intercept() const { return m_intercept; } // 0 unless status() is LINREG_OK
    size_t  count() const { return m_count; }         // points in the window now
    size_t  window() const { return m_n; }            // window size
    bool    full() const { return m_count == m_n; }
    int16_t status() const { return m_error; }        // result of the last call, see LINREG_* codes

  private:
    // Sums of t and value relative to the first point added to them (t0, y0),
    // so that large t (millis(), Unix time) does not cancel out.
    struct Sums {
      double  t0, y0;
      double  st, stt, sy, sty;
      size_t  n;
    };

    static bool fits(size_t n);
    static void clear(Sums& s);
    static void add(Sums& s, const point_t& p);
    static void remove(Sums& s, const point_t& p);

    void    attach(uint8_t* buffer, size_t n, bool owned);
    void    fail();
    void    solve();

    RngBuffer   m_rng;
    uint8_t*    m_buffer;
    bool        m_owned;
    size_t      m_n;
    size_t      m_count;
    Sums        m_cur;    // every point in the window
    Sums        m_next;   // points added since m_cur was last replaced
    double      m_slope;
    double      m_intercept;
    int16_t     m_error;
};


#endif  // RUNNINGLINREG_H
