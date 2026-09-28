#include <stdlib.h>
#include <float.h>
#include "RunningLinreg.h"

RunningLinreg::RunningLinreg(size_t n) {

  if ( n < 2 ) n = 2;
  attach(fits(n) ? (uint8_t*) malloc(n * sizeof(point_t)) : NULL, n, true);
}

RunningLinreg::RunningLinreg(size_t n, point_t* storage) {

  //  caller storage cannot be enlarged: fewer than 2 points is the same as none
  attach(n >= 2 && fits(n) ? (uint8_t*) storage : NULL, n, false);
}

//  a byte size that does not fit in size_t (16-bit on AVR) counts as no storage
bool RunningLinreg::fits(size_t n) {

  return n <= ((size_t) -1) / sizeof(point_t);
}

RunningLinreg::~RunningLinreg() {

  if ( m_owned ) free(m_buffer);
}

void RunningLinreg::attach(uint8_t* buffer, size_t n, bool owned) {

  m_buffer = buffer;
  m_owned = owned;
  m_n = n;
  initialize();
}

void RunningLinreg::initialize() {

  clear(m_cur);
  clear(m_next);
  m_count = 0;
  m_slope = 0.0;
  m_intercept = 0.0;

  //  rng_buffer_init_elements() refuses a NULL buffer
  if ( rng_buffer_init_elements(&m_rng, m_buffer, sizeof(point_t), m_n) != RNG_BUFFER_OK ) {
    m_error = LINREG_NUL;
    return;
  }
  m_error = LINREG_FEW;
}

void RunningLinreg::addPoint(point_t point) {

  //  if storage was not allocated
  //  exit with an error set
  if ( m_buffer == NULL ) {
    m_error = LINREG_NUL;
    return;
  }

  //  if the window is full - take the oldest point out
  //  of the running sums
  //  element calls move a whole point or nothing, so a failure
  //  leaves the RngBuffer and the sums in step
  if ( m_count == m_n ) {
    point_t p;
    if ( rng_buffer_get_element(&m_rng, &p) != RNG_BUFFER_OK ) {
      fail();
      return;
    }
    remove(m_cur, p);
    m_count--;
  }

  //  now add the point to the window
  //  on failure the window is one point shorter: the oldest point
  //  was never in m_next (holding the whole window triggers the swap below),
  //  so both sets of sums already match it
  if ( rng_buffer_put_element(&m_rng, &point) != RNG_BUFFER_OK ) {
    fail();
    return;
  }
  m_count++;
  add(m_cur, point);
  add(m_next, point);

  //  every point in the window has been added to m_next since the last swap:
  //  its sums were only ever added to, so they replace the running ones
  //  and the add/subtract rounding error of m_cur is dropped with them
  if ( m_next.n == m_count ) {
    m_cur = m_next;
    clear(m_next);
  }

  solve();
}

//  the point was not added; the next successful addPoint() recalculates
void RunningLinreg::fail() {

  m_slope = 0.0;
  m_intercept = 0.0;
  m_error = LINREG_ERR;
}

void RunningLinreg::solve() {

  if ( m_count < 2 ) {
    m_slope = 0.0;
    m_intercept = 0.0;
    m_error = LINREG_FEW;
    return;
  }

  const Sums& s = m_cur;
  double n = (double) s.n;
  double sxx = s.stt - s.st * s.st / n;
  double sxy = s.sty - s.st * s.sy / n;

  //  all t equal (within rounding): no single line fits
  //  written as !(a > b) so that NaN also lands here
  if ( !(sxx > s.stt * (16 * DBL_EPSILON)) ) {
    m_slope = 0.0;
    m_intercept = 0.0;
    m_error = LINREG_DEN;
    return;
  }

  m_slope = sxy / sxx;
  m_intercept = (s.y0 + s.sy / n) - m_slope * (s.t0 + s.st / n);
  m_error = LINREG_OK;
}

void RunningLinreg::clear(Sums& s) {

  s.t0 = s.y0 = 0.0;
  s.st = s.stt = s.sy = s.sty = 0.0;
  s.n = 0;
}

void RunningLinreg::add(Sums& s, const point_t& p) {

  //  the first point sets the reference
  if ( s.n == 0 ) {
    s.t0 = p.t;
    s.y0 = p.value;
  }
  double t = p.t - s.t0;
  double y = p.value - s.y0;
  s.st  += t;
  s.stt += t * t;
  s.sy  += y;
  s.sty += t * y;
  s.n++;
}

void RunningLinreg::remove(Sums& s, const point_t& p) {

  double t = p.t - s.t0;
  double y = p.value - s.y0;
  s.st  -= t;
  s.stt -= t * t;
  s.sy  -= y;
  s.sty -= t * y;
  s.n--;
}
