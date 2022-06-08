# Running Linreg

### Implementation of dynamic MSE linear regression f(t)
##### Version 1.0.0

### Description

`Running Linreg` (or Linear Regression) implements dynamic (and fast) recalculated mean squared error linear regression f(t) based on a pre-defined number of points. 

Each time a new point is added to the set, the **slope** and **intercept** values are recalculated based on the added point values **only.**

Use `addPoint(point_t point)` method to add a point to the set;

Then query recalculated **slope** and **intercept** values using methods:

```c++
double   slope();
double   intercept();
```

Methods 

```c++
double   count();
int16_t  status();
```

could be used to query total number of points already added, and if errors were encountered by the last method call. 

The point definition is:

```c++
typedef struct {
  double          t;
  double          value;
} point_t;
```

and a list of error codes is below:

```c++
#define LINREG_OK       0
#define LINREG_ERR      (-1) //  general error
#define LINREG_NUL      (-2) //  points pointer is NULL
#define LINREG_DEN      (-3) //  denominator is zero
```

**Enjoy!**





###### Changelog:

v1.0.0:

- 2015-11-18 - initial release

