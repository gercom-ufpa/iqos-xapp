/*
 * File: strtrim.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 12-Dec-2024 16:04:45
 */

/* Include Files */
#include "strtrim.h"
#include "rt_nonfinite.h"

/* Function Definitions */
/*
 * Arguments    : const char x_data[]
 *                const int x_size[2]
 *                char y_data[]
 *                int y_size[2]
 * Return Type  : void
 */
void strtrim(const char x_data[], const int x_size[2], char y_data[],
             int y_size[2])
{
  static const boolean_T bv[128] = {
      false, false, false, false, false, false, false, false, false, true,
      true,  true,  true,  true,  false, false, false, false, false, false,
      false, false, false, false, false, false, false, false, true,  true,
      true,  true,  true,  false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false};
  int b_j1;
  int j2;
  b_j1 = 1;
  while ((b_j1 <= x_size[1]) && bv[(unsigned char)x_data[0] & 127] &&
         (x_data[0] != '\x00')) {
    b_j1 = 2;
  }
  j2 = x_size[1];
  while ((j2 > 0) && bv[(unsigned char)x_data[0] & 127] &&
         (x_data[0] != '\x00')) {
    j2 = 0;
  }
  b_j1 = (b_j1 <= j2);
  y_size[0] = 1;
  y_size[1] = b_j1;
  if (b_j1 - 1 >= 0) {
    y_data[0] = x_data[0];
  }
}

/*
 * File trailer for strtrim.c
 *
 * [EOF]
 */
