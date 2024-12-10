/*
 * File: predict.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 09-Dec-2024 16:20:26
 */

/* Include Files */
#include "predict.h"
#include "predictForRNN.h"
#include "rt_nonfinite.h"
#include "runPrediction_internal_types.h"
#include "runPrediction_types.h"

/* Function Definitions */
/*
 * Arguments    : c_coder_internal_ctarget_DeepLe *obj
 *                const cell_wrap_0 *varargin_1
 *                float varargout_1[3]
 * Return Type  : void
 */
void DeepLearningNetwork_predict(c_coder_internal_ctarget_DeepLe *obj,
                                 const cell_wrap_0 *varargin_1,
                                 float varargout_1[3])
{
  cell_wrap_7 r;
  r.f1[0] = *varargin_1;
  c_DeepLearningNetwork_predictFo(obj, &r, varargout_1);
}

/*
 * File trailer for predict.c
 *
 * [EOF]
 */
