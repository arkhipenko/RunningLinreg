#ifndef _RUNNING_LINREG_H_
#define _RUNNING_LINREG_H_

#include <queue>

#define LINREG_OK       0
#define LINREG_ERR      (-1) //  general error
#define LINREG_NUL      (-2) //  points pointer is NULL
#define LINREG_DEN      (-3) //  denominator is zero


typedef struct {
  double          t;
  double          value;
} point_t;


class RunningLinreg {
  
  public:
    RunningLinreg(std::queue<point_t>* points, size_t n);
    
    void  addPoint(point_t point);
    void  initialize();
    
    inline double   slope() { return m_slope; }
    inline double   intercept() { return m_intercept; }
    inline double   count() { return m_n_real; }
    inline int16_t  status() { return m_error; }
    
  private:
    std::queue<point_t>*  m_points;
    size_t  m_n;
    double  m_sum_t;
    double  m_sum_tt;
    double  m_sum_ty;
    double  m_sum_y;
    double  m_sum_yy;
    double  m_n_real;
    double  m_slope;
    double  m_intercept;
    double  m_denominator;
    int16_t m_error;
};


#endif  //   _RUNNING_LINREG_H_