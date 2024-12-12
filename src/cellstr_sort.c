/*
 * File: cellstr_sort.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 12-Dec-2024 16:04:45
 */

/* Include Files */
#include "cellstr_sort.h"
#include "introsort.h"
#include "rt_nonfinite.h"
#include "runPrediction_types.h"

/* Function Definitions */
/*
 * Arguments    : const cell_wrap_1 c_data[]
 *                int c_size
 *                cell_wrap_1 sorted_data[]
 *                int idx_data[]
 *                int *idx_size
 * Return Type  : int
 */
int cellstr_sort(const cell_wrap_1 c_data[], int c_size,
                 cell_wrap_1 sorted_data[], int idx_data[], int *idx_size)
{
  int k;
  int n_tmp_tmp;
  int sorted_size;
  int yk;
  short y_data[3];
  n_tmp_tmp = (unsigned char)(c_size - 1) + 1;
  y_data[0] = 1;
  yk = 1;
  for (k = 2; k <= n_tmp_tmp; k++) {
    yk++;
    y_data[k - 1] = (short)yk;
  }
  *idx_size = (unsigned char)(c_size - 1) + 1;
  for (k = 0; k < n_tmp_tmp; k++) {
    idx_data[k] = y_data[k];
  }
  introsort(idx_data, c_size, c_data);
  sorted_size = c_size;
  for (yk = 0; yk < c_size; yk++) {
    sorted_data[yk].f1.size[0] = 1;
    k = c_data[idx_data[yk] - 1].f1.size[1];
    sorted_data[yk].f1.size[1] = k;
    if (k - 1 >= 0) {
      sorted_data[yk].f1.data[0] = c_data[idx_data[yk] - 1].f1.data[0];
    }
  }
  return sorted_size;
}

/*
 * File trailer for cellstr_sort.c
 *
 * [EOF]
 */
