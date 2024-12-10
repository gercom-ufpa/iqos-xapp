/*
 * File: runPrediction_terminate.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 09-Dec-2024 16:20:26
 */

/* Include Files */
#include "runPrediction_terminate.h"
#include "rt_nonfinite.h"
#include "runPrediction_data.h"
#include "omp.h"

/* Function Definitions */
/*
 * Arguments    : void
 * Return Type  : void
 */
void runPrediction_terminate(void)
{
  omp_destroy_nest_lock(&runPrediction_nestLockGlobal);
  isInitialized_runPrediction = false;
}

/*
 * File trailer for runPrediction_terminate.c
 *
 * [EOF]
 */
