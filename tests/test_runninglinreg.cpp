//
// Host unit tests for RunningLinreg
// Build and run: make -C tests   (options: CXX=clang++ STD=c++17 RNGBUFFER_DIR=...)
//
// Built with AddressSanitizer and UndefinedBehaviorSanitizer by the Makefile,
// so buffer overruns and leaks fail the run, not only wrong values.
// Every result is compared with a two-pass least-squares fit of the same window.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <deque>
#include <type_traits>

// Allocation-failure hook: the library's malloc calls go through test_malloc.
static bool g_failMalloc = false;
static void* test_malloc(size_t n) { return g_failMalloc ? NULL : malloc(n); }
#define malloc test_malloc
#include "../src/RunningLinreg.cpp"
#undef malloc

// Storage-failure hook: replaces RngBuffer's weak rng_buffer_lock().
// The call numbered g_lockFailAt (counting from 0) fails, as a lock timeout would.
static long g_lockCalls = 0;
static long g_lockFailAt = -1;
extern "C" bool rng_buffer_lock(unsigned int) { return g_lockCalls++ != g_lockFailAt; }

static int g_fail = 0, g_pass = 0;

#define CHECK(c) do { if (c) g_pass++; else { g_fail++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
#define CHECK_EQ(a, b) do { long long _a = (long long)(a), _b = (long long)(b); \
    if (_a == _b) g_pass++; \
    else { g_fail++; printf("FAIL %s:%d: %s == %s (%lld vs %lld)\n", __FILE__, __LINE__, #a, #b, _a, _b); } } while (0)
#define CHECK_NEAR(a, b, tol) do { double _a = (a), _b = (b), _t = (tol); \
    if (fabs(_a - _b) <= _t) g_pass++; \
    else { g_fail++; printf("FAIL %s:%d: %s ~ %s (%.17g vs %.17g, tol %.3g)\n", __FILE__, __LINE__, #a, #b, _a, _b, _t); } } while (0)

static uint32_t g_rng = 12345;
static double rnd() { g_rng = g_rng * 1664525u + 1013904223u; return (g_rng >> 8) / 16777216.0; }  // [0, 1)

// Reference model: keeps the last `win` points and fits them in two passes, in long double.
struct Ref {
    std::deque<point_t> w;
    size_t win;
    explicit Ref(size_t n) : win(n) {}
    void push(point_t p) { w.push_back(p); if (w.size() > win) w.pop_front(); }
    void dropOldest() { w.pop_front(); }
    void fit(double& slope, double& intercept, double& scale) const {
        long double mt = 0, my = 0;
        for (const point_t& p : w) { mt += p.t; my += p.value; }
        mt /= w.size(); my /= w.size();
        long double sxx = 0, sxy = 0;
        for (const point_t& p : w) {
            long double dt = p.t - mt, dy = p.value - my;
            sxx += dt * dt; sxy += dt * dy;
        }
        slope = (double)(sxy / sxx);
        intercept = (double)(my - sxy / sxx * mt);
        scale = (double)(fabsl(my) + fabsl(sxy / sxx * mt) + 1);  // size of the terms the intercept is made of
    }
};

// Compare the filter with the reference; rel is the allowed relative error
static void checkFit(const RunningLinreg& lr, const Ref& r, double rel) {
    double m, b, scale;
    r.fit(m, b, scale);
    CHECK_EQ(lr.status(), LINREG_OK);
    CHECK_NEAR(lr.slope(), m, rel * fabs(m) + 1e-300);
    CHECK_NEAR(lr.intercept(), b, rel * scale);
}

// Not copyable: the object owns its window
static_assert(!std::is_copy_constructible<RunningLinreg>::value, "RunningLinreg must not be copyable");
static_assert(!std::is_copy_assignable<RunningLinreg>::value, "RunningLinreg must not be copy-assignable");

static void testExactLine() {
    RunningLinreg lr(10);
    for (int i = 0; i < 30; i++) {
        lr.addPoint({ (double) i, 2.0 * i + 1.0 });
        CHECK_EQ(lr.count(), i < 10 ? i + 1 : 10);
        CHECK(lr.full() == (i >= 9));
        if (i == 0) continue;
        CHECK_EQ(lr.status(), LINREG_OK);
        CHECK_NEAR(lr.slope(), 2.0, 1e-12);
        CHECK_NEAR(lr.intercept(), 1.0, 1e-12);
    }
}

static void testStatus() {
    RunningLinreg lr(3);
    CHECK_EQ(lr.status(), LINREG_FEW);
    CHECK_EQ(lr.count(), 0);
    CHECK_EQ(lr.window(), 3);

    lr.addPoint({ 1.0, 5.0 });                  // one point: no line yet
    CHECK_EQ(lr.status(), LINREG_FEW);
    CHECK_EQ(lr.slope(), 0);

    lr.addPoint({ 1.0, 7.0 });                  // two points, same t: slope undefined
    CHECK_EQ(lr.status(), LINREG_DEN);
    CHECK_EQ(lr.slope(), 0);
    CHECK_EQ(lr.intercept(), 0);

    lr.addPoint({ 2.0, 9.0 });
    CHECK_EQ(lr.status(), LINREG_OK);

    // window slides until every point has the same large t again
    for (int i = 0; i < 3; i++) lr.addPoint({ 1.0e9, (double) i });
    CHECK_EQ(lr.status(), LINREG_DEN);
    lr.addPoint({ 1.0e9 + 1, 0.0 });
    CHECK_EQ(lr.status(), LINREG_OK);
}

static void testAgainstReference() {
    const long windows[] = { 2, 3, 7, 64, 1000 };
    for (long w : windows) {
        RunningLinreg lr(w); Ref r(w);
        double t = rnd() * 100;
        for (int i = 0; i < 3000; i++) {
            t += 0.001 + rnd();                  // uneven steps
            point_t p = { t, rnd() * 50 - 25 + 0.3 * t };
            lr.addPoint(p); r.push(p);
            if (i > 0) checkFit(lr, r, 1e-9);
        }
    }
}

static void testLargeTime() {
    struct { double t0, dt, slope; } cases[] = {
        { 30 * 86400e3, 10, 1e-3 },   // millis() after 30 days
        { 4.0e9, 1000, 1e-6 },        // micros() near its wrap
        { 1.79e12, 1000, 1e-6 },      // Unix time in milliseconds
    };
    for (auto& c : cases) {
        RunningLinreg lr(200); Ref r(200);
        for (int i = 0; i < 500; i++) {
            double t = c.t0 + i * c.dt;
            point_t p = { t, 5 + c.slope * (t - c.t0) + (rnd() - 0.5) * 0.02 };
            lr.addPoint(p); r.push(p);
        }
        checkFit(lr, r, 1e-9);
    }
}

// Two million updates: add/subtract rounding must not build up
static void testNoDrift() {
    RunningLinreg lr(50); Ref r(50);
    for (long i = 0; i < 2000000; i++) {
        double t = i * 0.01;
        point_t p = { t, 3 + 0.5 * t + (rnd() - 0.5) * 0.2 };
        lr.addPoint(p); r.push(p);
    }
    checkFit(lr, r, 1e-9);
}

static void testWindowClamp() {
    RunningLinreg a(0), b(1);
    CHECK_EQ(a.window(), 2);
    CHECK_EQ(b.window(), 2);
    a.addPoint({ 0, 1 }); a.addPoint({ 1, 3 }); a.addPoint({ 2, 4 });
    CHECK_EQ(a.count(), 2);
    CHECK_NEAR(a.slope(), 1.0, 1e-12);          // only (1,3) and (2,4) are left
}

static void testCallerStorage() {
    {   // points go into the caller's array and nowhere past it
        struct { point_t storage[5]; point_t guard; } mem;
        memset(&mem.guard, 0xA5, sizeof mem.guard);
        point_t guard = mem.guard;
        RunningLinreg lr(5, mem.storage); Ref r(5);
        for (int i = 0; i < 40; i++) {
            point_t p = { i * 0.5, rnd() };
            lr.addPoint(p); r.push(p);
            if (i > 0) checkFit(lr, r, 1e-9);
        }
        CHECK(memcmp(&mem.guard, &guard, sizeof guard) == 0);
    }
    {   // NULL storage
        RunningLinreg lr(5, NULL);
        CHECK_EQ(lr.status(), LINREG_NUL);
        lr.addPoint({ 1, 1 });
        CHECK_EQ(lr.status(), LINREG_NUL);
        CHECK_EQ(lr.count(), 0);
    }
    {   // storage for fewer than 2 points cannot be used
        point_t one[1];
        RunningLinreg lr(1, one);
        CHECK_EQ(lr.status(), LINREG_NUL);
        lr.addPoint({ 1, 1 });
        CHECK_EQ(lr.status(), LINREG_NUL);
    }
}

static void testAllocFailure() {
    g_failMalloc = true;
    RunningLinreg lr(10);
    g_failMalloc = false;
    CHECK_EQ(lr.status(), LINREG_NUL);
    lr.addPoint({ 1, 1 });
    lr.addPoint({ 2, 2 });
    CHECK_EQ(lr.status(), LINREG_NUL);
    CHECK_EQ(lr.count(), 0);
    lr.initialize();
    CHECK_EQ(lr.status(), LINREG_NUL);

    RunningLinreg huge((size_t) -1);            // byte size overflows size_t
    CHECK_EQ(huge.status(), LINREG_NUL);
}

// A failed RngBuffer call leaves the point out and keeps the window in step with the sums
static void testStorageFailure() {
    // 0: removing the oldest point fails; 1: adding the new point fails after the oldest was removed;
    // 2: adding fails while the window is still filling
    for (int phase = 0; phase < 3; phase++) {
        RunningLinreg lr(4); Ref r(4);
        int fill = phase == 2 ? 2 : 4;
        for (int i = 0; i < fill; i++) { point_t p = { (double) i, rnd() }; lr.addPoint(p); r.push(p); }
        // each element call takes the lock once: with a full window get comes first, then put
        g_lockCalls = 0;
        g_lockFailAt = phase == 1 ? 1 : 0;
        lr.addPoint({ 10, 10 });
        g_lockFailAt = -1;
        CHECK_EQ(lr.status(), LINREG_ERR);
        CHECK_EQ(lr.slope(), 0);
        CHECK_EQ(lr.intercept(), 0);
        if (phase == 1) r.dropOldest();
        CHECK_EQ(lr.count(), r.w.size());

        for (int i = 0; i < 9; i++) {
            point_t p = { 20.0 + i, rnd() };
            lr.addPoint(p); r.push(p);
            checkFit(lr, r, 1e-9);
        }
    }
}

static void testInitialize() {
    RunningLinreg lr(8);
    for (int i = 0; i < 20; i++) lr.addPoint({ (double) i, rnd() });
    lr.initialize();
    CHECK_EQ(lr.count(), 0);
    CHECK_EQ(lr.status(), LINREG_FEW);
    CHECK_EQ(lr.slope(), 0);
    CHECK_EQ(lr.intercept(), 0);

    Ref r(8);
    for (int i = 0; i < 20; i++) {
        point_t p = { 100.0 + i, rnd() };
        lr.addPoint(p); r.push(p);
        if (i > 0) checkFit(lr, r, 1e-9);
    }
}

int main() {
    testExactLine();
    testStatus();
    testAgainstReference();
    testLargeTime();
    testNoDrift();
    testWindowClamp();
    testCallerStorage();
    testAllocFailure();
    testStorageFailure();
    testInitialize();

    printf("%d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
