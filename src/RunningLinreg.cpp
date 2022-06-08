#include <Arduino.h>
#include "RunningLinreg.h"

RunningLinreg::RunningLinreg(std::queue<point_t>* points, size_t n) {
  
  m_points = points;
  m_n = n;
  initialize();
}

void RunningLinreg::initialize() {
  
    m_sum_t = 0.0;
    m_sum_tt = 0.0;
    m_sum_ty = 0.0;
    m_sum_y = 0.0;
    m_sum_yy = 0.0;
    m_n_real = 0.0; // (double)(m_n);
    m_slope = 0.0;
    m_intercept = 0.0;
    m_denominator = 0.0;
    m_error = LINREG_OK;

    if ( m_points != NULL ) {
      while ( m_points->size() ) m_points->pop();
    }
    else {
      m_error = LINREG_NUL;
    }
}

void RunningLinreg::addPoint(point_t point) {
  
  //  if array of points was not allocated
  //  exit with an error set
  if ( m_points == NULL ) {
    m_error = LINREG_NUL;
    return;
  }

  //  if buffer is full - take one value from the back
  //  and recalculate
  if ( m_points->size() == m_n ) {
    point_t p = m_points->front();
    m_points->pop();
    m_sum_t  -= p.t;
    m_sum_tt -= p.t * p.t;
    m_sum_ty -= p.t * p.value;
    m_sum_y  -= p.value;
    m_sum_yy -= p.value * p.value;
  }
  
  //  now add the point to the array
  m_points->push(point);
  m_sum_t  += point.t;
  m_sum_tt += point.t * point.t;
  m_sum_ty += point.t * point.value;
  m_sum_y  += point.value;
  m_sum_yy += point.value * point.value;
  
  m_n_real = (double) m_points->size();
  
  //  recalcualte
  m_denominator = m_n_real * m_sum_tt - m_sum_t * m_sum_t;
  if ( m_denominator == 0.0 ) {
    m_slope = 0.0;
    m_intercept = 0.0; 
    m_error = LINREG_DEN;
    return;
  }
  
  m_slope = (m_n_real * m_sum_ty - m_sum_t * m_sum_y) / m_denominator;
  m_intercept = (m_sum_y - m_slope * m_sum_t) / m_n_real;

  m_error = LINREG_OK;
}