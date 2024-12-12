/*
 * File: introsort.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 12-Dec-2024 16:04:45
 */

/* Include Files */
#include "introsort.h"
#include "insertionsort.h"
#include "rt_nonfinite.h"
#include "runPrediction_types.h"

/* Function Definitions */
/*
 * Arguments    : int x_data[]
 *                int xend
 *                const cell_wrap_1 cmp_workspace_c_data[]
 * Return Type  : void
 */
void introsort(int x_data[], int xend, const cell_wrap_1 cmp_workspace_c_data[])
{
  if (xend > 1) {
    insertionsort(x_data, xend, cmp_workspace_c_data);
  }
}

/*
 * File trailer for introsort.c
 *
 * [EOF]
 */
