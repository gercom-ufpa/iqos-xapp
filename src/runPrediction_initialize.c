/*
 * File: runPrediction_initialize.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 12-Dec-2024 16:04:45
 */

/* Include Files */
#include "runPrediction_initialize.h"
#include "rt_nonfinite.h"
#include "runPrediction_data.h"
#include "omp.h"

/* Function Definitions */
/*
 * Arguments    : void
 * Return Type  : void
 */
void runPrediction_initialize(void)
{
  omp_init_nest_lock(&runPrediction_nestLockGlobal);
  isInitialized_runPrediction = true;
}

/*
 * File trailer for runPrediction_initialize.c
 *
 * [EOF]
 */
