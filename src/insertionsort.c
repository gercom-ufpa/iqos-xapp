/*
 * File: insertionsort.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 12-Dec-2024 16:04:45
 */

/* Include Files */
#include "insertionsort.h"
#include "rt_nonfinite.h"
#include "runPrediction_types.h"
#include <math.h>

/* Function Definitions */
/*
 * Arguments    : int x_data[]
 *                int xend
 *                const cell_wrap_1 cmp_workspace_c_data[]
 * Return Type  : void
 */
void insertionsort(int x_data[], int xend,
                   const cell_wrap_1 cmp_workspace_c_data[])
{
  int k;
  for (k = 2; k <= xend; k++) {
    int idx;
    int xc;
    boolean_T exitg1;
    xc = x_data[k - 1] - 1;
    idx = k - 2;
    exitg1 = false;
    while ((!exitg1) && (idx + 1 >= 1)) {
      int b_k;
      int b_n_tmp;
      int j;
      int n;
      int n_tmp;
      boolean_T varargout_1;
      j = x_data[idx];
      n_tmp = cmp_workspace_c_data[xc].f1.size[1];
      b_n_tmp = cmp_workspace_c_data[x_data[idx] - 1].f1.size[1];
      n = (int)fmin(n_tmp, b_n_tmp);
      varargout_1 = (n_tmp < b_n_tmp);
      b_k = 0;
      int exitg2;
      do {
        exitg2 = 0;
        if (b_k <= n - 1) {
          if (cmp_workspace_c_data[xc].f1.data[0] !=
              cmp_workspace_c_data[x_data[idx] - 1].f1.data[0]) {
            varargout_1 = (cmp_workspace_c_data[xc].f1.data[0] <
                           cmp_workspace_c_data[x_data[idx] - 1].f1.data[0]);
            exitg2 = 1;
          } else {
            b_k = 1;
          }
        } else {
          if (n_tmp == b_n_tmp) {
            varargout_1 = (xc + 1 < j);
          }
          exitg2 = 1;
        }
      } while (exitg2 == 0);
      if (varargout_1) {
        x_data[idx + 1] = x_data[idx];
        idx--;
      } else {
        exitg1 = true;
      }
    }
    x_data[idx + 1] = xc + 1;
  }
}

/*
 * File trailer for insertionsort.c
 *
 * [EOF]
 */
