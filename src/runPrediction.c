/*
 * File: runPrediction.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 09-Dec-2024 16:20:26
 */

/* Include Files */
#include "runPrediction.h"
#include "getCategoryNames.h"
#include "postProcessOutputToReturnCategorical.h"
#include "predict.h"
#include "rt_nonfinite.h"
#include "runPrediction_data.h"
#include "runPrediction_initialize.h"
#include "runPrediction_internal_types.h"
#include "runPrediction_types.h"

/* Function Definitions */
/*
 * Fazer a previsão
 *
 * Arguments    : const cell_wrap_0 A[1]
 *                categorical *pred
 * Return Type  : void
 */
void runPrediction(const cell_wrap_0 A[1], categorical *pred)
{
  c_coder_internal_ctarget_DeepLe net;
  cell_wrap_32 r;
  emxArray_cell_wrap_1_3 idx_categoryNames;
  unsigned int idx_codes;
  if (!isInitialized_runPrediction) {
    runPrediction_initialize();
  }
  net.IsNetworkInitialized = false;
  net.matlabCodegenIsDeleted = false;
  DeepLearningNetwork_predict(&net, &A[0], r.f1);
  idx_codes = c_DeepLearningNetwork_postProce(r, idx_categoryNames.data,
                                              &idx_categoryNames.size[0]);
  /*  Converter a previsão para rótulos */
  pred->categoryNames.size[0] = categorical_getCategoryNames(
      idx_categoryNames.data, idx_categoryNames.size[0],
      pred->categoryNames.data);
  if (idx_codes > 255U) {
    idx_codes = 255U;
  }
  pred->codes = (unsigned char)idx_codes;
}

/*
 * File trailer for runPrediction.c
 *
 * [EOF]
 */
