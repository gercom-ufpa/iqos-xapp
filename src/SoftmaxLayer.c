/*
 * File: SoftmaxLayer.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 12-Dec-2024 16:04:45
 */

/* Include Files */
#include "SoftmaxLayer.h"
#include "internal_softmax.h"
#include "rt_nonfinite.h"

/* Function Definitions */
/*
 * Arguments    : const float X1_Data[3]
 *                float Z1_Data[3]
 * Return Type  : void
 */
void SoftmaxLayer_predict(const float X1_Data[3], float Z1_Data[3])
{
  iComputeSoftmaxForCpu(X1_Data, Z1_Data);
}

/*
 * File trailer for SoftmaxLayer.c
 *
 * [EOF]
 */
