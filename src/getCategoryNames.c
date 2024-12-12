/*
 * File: getCategoryNames.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 12-Dec-2024 16:04:45
 */

/* Include Files */
#include "getCategoryNames.h"
#include "rt_nonfinite.h"
#include "runPrediction_types.h"

/* Function Definitions */
/*
 * Arguments    : const cell_wrap_1 valueSet_data[]
 *                int valueSet_size
 *                cell_wrap_1 outCategoryNames_data[]
 * Return Type  : int
 */
int categorical_getCategoryNames(const cell_wrap_1 valueSet_data[],
                                 int valueSet_size,
                                 cell_wrap_1 outCategoryNames_data[])
{
  int i;
  int outCategoryNames_size;
  if (valueSet_size != 0) {
    outCategoryNames_size = valueSet_size;
    for (i = 0; i < valueSet_size; i++) {
      int b_i;
      outCategoryNames_data[i].f1.size[0] = 1;
      b_i = valueSet_data[i].f1.size[1];
      outCategoryNames_data[i].f1.size[1] = b_i;
      if (b_i - 1 >= 0) {
        outCategoryNames_data[i].f1.data[0] = valueSet_data[i].f1.data[0];
      }
    }
  } else {
    outCategoryNames_size = 0;
  }
  return outCategoryNames_size;
}

/*
 * File trailer for getCategoryNames.c
 *
 * [EOF]
 */
