/*
 * File: find.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 09-Dec-2024 16:20:26
 */

/* Include Files */
#include "find.h"
#include "rt_nonfinite.h"

/* Function Definitions */
/*
 * Arguments    : const boolean_T x_data[]
 *                int x_size
 *                int i_data[]
 * Return Type  : int
 */
int eml_find(const boolean_T x_data[], int x_size, int i_data[])
{
  int i_size;
  int idx;
  int ii;
  boolean_T exitg1;
  idx = 0;
  i_size = x_size;
  ii = 0;
  exitg1 = false;
  while ((!exitg1) && (ii <= x_size - 1)) {
    if (x_data[ii]) {
      idx++;
      i_data[idx - 1] = ii + 1;
      if (idx >= x_size) {
        exitg1 = true;
      } else {
        ii++;
      }
    } else {
      ii++;
    }
  }
  if (x_size == 1) {
    if (idx == 0) {
      i_size = 0;
    }
  } else if (idx < 1) {
    i_size = 0;
  } else {
    i_size = idx;
  }
  return i_size;
}

/*
 * File trailer for find.c
 *
 * [EOF]
 */
