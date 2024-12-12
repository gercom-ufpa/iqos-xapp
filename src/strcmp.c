/*
 * File: strcmp.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 12-Dec-2024 16:04:45
 */

/* Include Files */
#include "strcmp.h"
#include "rt_nonfinite.h"

/* Function Definitions */
/*
 * Arguments    : const char a_data[]
 *                const int a_size[2]
 *                const char b_data[]
 *                const int b_size[2]
 * Return Type  : boolean_T
 */
boolean_T b_strcmp(const char a_data[], const int a_size[2],
                   const char b_data[], const int b_size[2])
{
  boolean_T b;
  boolean_T b_bool;
  b_bool = false;
  b = (a_size[1] == 0);
  if (b && (b_size[1] == 0)) {
    b_bool = true;
  } else if (a_size[1] == b_size[1]) {
    int kstr;
    kstr = 0;
    int exitg1;
    do {
      exitg1 = 0;
      if (kstr <= b_size[1] - 1) {
        if (a_data[0] != b_data[0]) {
          exitg1 = 1;
        } else {
          kstr = 1;
        }
      } else {
        b_bool = true;
        exitg1 = 1;
      }
    } while (exitg1 == 0);
  }
  return b_bool;
}

/*
 * File trailer for strcmp.c
 *
 * [EOF]
 */
