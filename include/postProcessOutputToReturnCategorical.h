/*
 * File: postProcessOutputToReturnCategorical.h
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 09-Dec-2024 16:20:26
 */

#ifndef POSTPROCESSOUTPUTTORETURNCATEGORICAL_H
#define POSTPROCESSOUTPUTTORETURNCATEGORICAL_H

/* Include Files */
#include "rtwtypes.h"
#include "runPrediction_internal_types.h"
#include "runPrediction_types.h"
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
unsigned int
c_DeepLearningNetwork_postProce(const cell_wrap_32 scores,
                                cell_wrap_1 c_processedOutput_0_categoryNam[],
                                int *d_processedOutput_0_categoryNam);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for postProcessOutputToReturnCategorical.h
 *
 * [EOF]
 */
