/*
 * File: postProcessOutputToReturnCategorical.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 09-Dec-2024 16:20:26
 */

/* Include Files */
#include "postProcessOutputToReturnCategorical.h"
#include "cellstr_sort.h"
#include "find.h"
#include "getCategoryNames.h"
#include "rt_nonfinite.h"
#include "runPrediction_internal_types.h"
#include "runPrediction_types.h"
#include "strcmp.h"
#include "strtrim.h"
#include "rt_nonfinite.h"
#include <math.h>
#include <string.h>

/* Type Definitions */
#ifndef c_typedef_emxArray_cell_wrap_1_
#define c_typedef_emxArray_cell_wrap_1_
typedef struct {
  cell_wrap_1 data[3];
  int size[2];
} emxArray_cell_wrap_1_1x3;
#endif /* c_typedef_emxArray_cell_wrap_1_ */

/* Function Definitions */
/*
 * Arguments    : const cell_wrap_32 scores
 *                cell_wrap_1 c_processedOutput_0_categoryNam[]
 *                int *d_processedOutput_0_categoryNam
 * Return Type  : unsigned int
 */
unsigned int
c_DeepLearningNetwork_postProce(const cell_wrap_32 scores,
                                cell_wrap_1 c_processedOutput_0_categoryNam[],
                                int *d_processedOutput_0_categoryNam)
{
  static const char cv[3] = {'1', '2', '3'};
  cell_wrap_1 inData;
  emxArray_cell_wrap_1_1x3 labelsCells;
  emxArray_cell_wrap_1_1x3 r;
  emxArray_cell_wrap_1_3 c;
  emxArray_cell_wrap_1_3 uB;
  int dIdx_data[3];
  int ib_data[3];
  int idx_data[3];
  int b_i;
  int i;
  int idx;
  int idx_size;
  int k;
  int nz;
  boolean_T b_d_data[3];
  boolean_T d_data[2];
  boolean_T exitg1;
  if (!rtIsNaNF(scores.f1[0])) {
    idx = 1;
  } else {
    idx = 0;
    k = 2;
    exitg1 = false;
    while ((!exitg1) && (k < 4)) {
      if (!rtIsNaNF(scores.f1[k - 1])) {
        idx = k;
        exitg1 = true;
      } else {
        k++;
      }
    }
  }
  if (idx == 0) {
    idx = 1;
  } else {
    float ex;
    ex = scores.f1[idx - 1];
    i = idx + 1;
    for (k = i; k < 4; k++) {
      float f;
      f = scores.f1[k - 1];
      if (ex < f) {
        ex = f;
        idx = k;
      }
    }
  }
  labelsCells.size[1] = 0;
  for (b_i = 0; b_i < 3; b_i++) {
    nz = labelsCells.size[1] + 1;
    labelsCells.size[1]++;
    labelsCells.data[nz - 1].f1.size[0] = 1;
    labelsCells.data[labelsCells.size[1] - 1].f1.size[1] = 1;
    labelsCells.data[labelsCells.size[1] - 1].f1.data[0] = cv[b_i];
  }
  strtrim(labelsCells.data[idx - 1].f1.data, labelsCells.data[idx - 1].f1.size,
          inData.f1.data, inData.f1.size);
  i = labelsCells.size[1];
  for (b_i = 0; b_i < i; b_i++) {
    strtrim(labelsCells.data[b_i].f1.data, labelsCells.data[b_i].f1.size,
            r.data[b_i].f1.data, r.data[b_i].f1.size);
  }
  if (labelsCells.size[1] != 0) {
    c.size[0] =
        cellstr_sort(r.data, labelsCells.size[1], c.data, idx_data, &idx_size);
  }
  *d_processedOutput_0_categoryNam = categorical_getCategoryNames(
      r.data, labelsCells.size[1], c_processedOutput_0_categoryNam);
  if (labelsCells.size[1] == 0) {
    uB.size[0] = 0;
  } else {
    int vlen_tmp;
    c.size[0] =
        cellstr_sort(r.data, labelsCells.size[1], c.data, idx_data, &idx_size);
    idx = c.size[0] - 1;
    nz = c.size[0];
    for (b_i = 0; b_i <= nz - 2; b_i++) {
      d_data[b_i] = !b_strcmp(c.data[b_i].f1.data, c.data[b_i].f1.size,
                              c.data[b_i + 1].f1.data, c.data[b_i + 1].f1.size);
    }
    vlen_tmp = c.size[0];
    b_d_data[0] = true;
    if (idx - 1 >= 0) {
      memcpy(&b_d_data[1], &d_data[0], (unsigned int)idx * sizeof(boolean_T));
    }
    nz = b_d_data[0];
    for (k = 2; k <= vlen_tmp; k++) {
      nz += b_d_data[k - 1];
    }
    uB.size[0] = (int)fmin(nz, labelsCells.size[1]);
    vlen_tmp = eml_find(b_d_data, c.size[0], dIdx_data);
    for (b_i = 0; b_i < vlen_tmp; b_i++) {
      if (b_i + 1 != vlen_tmp) {
        double y_data[3];
        double d;
        int y_size_idx_1;
        d = (double)dIdx_data[b_i + 1] - 1.0;
        i = dIdx_data[b_i];
        if (d < i) {
          y_size_idx_1 = 0;
        } else {
          nz = (int)d - i;
          y_size_idx_1 = nz + 1;
          for (idx = 0; idx <= nz; idx++) {
            y_data[idx] = (double)i + (double)idx;
          }
        }
        idx = idx_data[(int)y_data[0] - 1];
        for (k = 2; k <= y_size_idx_1; k++) {
          i = idx_data[(int)y_data[1] - 1];
          if (idx > i) {
            idx = i;
          }
        }
        ib_data[b_i] = idx;
      } else {
        double y_data[3];
        int y_size_idx_1;
        i = dIdx_data[b_i];
        if (idx_size < i) {
          y_size_idx_1 = 0;
        } else {
          nz = idx_size - i;
          y_size_idx_1 = nz + 1;
          for (i = 0; i <= nz; i++) {
            y_data[i] = dIdx_data[b_i] + i;
          }
        }
        idx = idx_data[(int)y_data[0] - 1];
        for (k = 2; k <= y_size_idx_1; k++) {
          i = idx_data[(int)y_data[k - 1] - 1];
          if (idx > i) {
            idx = i;
          }
        }
        ib_data[b_i] = idx;
      }
      uB.data[b_i].f1.size[0] = 1;
      i = r.data[idx - 1].f1.size[1];
      uB.data[b_i].f1.size[1] = i;
      if (i - 1 >= 0) {
        uB.data[b_i].f1.data[0] = r.data[idx - 1].f1.data[0];
      }
    }
  }
  i = 0;
  if (uB.size[0] > 0) {
    boolean_T is_less_than;
    nz = 0;
    if (inData.f1.size[1] <= uB.data[0].f1.size[1]) {
      idx_size = inData.f1.size[1];
    } else {
      idx_size = 0;
    }
    if (idx_size == 0) {
      is_less_than = (inData.f1.size[1] < uB.data[0].f1.size[1]);
    } else if (inData.f1.data[0] == uB.data[0].f1.data[0]) {
      is_less_than = (inData.f1.size[1] < uB.data[0].f1.size[1]);
    } else {
      is_less_than = (inData.f1.data[0] < uB.data[0].f1.data[0]);
    }
    if (!is_less_than) {
      if (b_strcmp(inData.f1.data, inData.f1.size, uB.data[0].f1.data,
                   uB.data[0].f1.size)) {
        i = ib_data[0];
      } else {
        exitg1 = false;
        while ((!exitg1) && (nz + 1 <= uB.size[0])) {
          idx = uB.data[nz].f1.size[1];
          if (inData.f1.size[1] <= idx) {
            idx_size = inData.f1.size[1];
          } else {
            idx_size = 0;
          }
          if (idx_size == 0) {
            is_less_than = (inData.f1.size[1] > idx);
          } else if (inData.f1.data[0] == uB.data[nz].f1.data[0]) {
            is_less_than = (inData.f1.size[1] > idx);
          } else {
            is_less_than = (inData.f1.data[0] > uB.data[nz].f1.data[0]);
          }
          if (is_less_than) {
            nz++;
          } else {
            exitg1 = true;
          }
        }
        if ((nz + 1 <= uB.size[0]) &&
            b_strcmp(inData.f1.data, inData.f1.size, uB.data[nz].f1.data,
                     uB.data[nz].f1.size)) {
          i = ib_data[nz];
        }
      }
    }
  }
  if (i < 0) {
    i = 0;
  }
  return (unsigned int)i;
}

/*
 * File trailer for postProcessOutputToReturnCategorical.c
 *
 * [EOF]
 */
