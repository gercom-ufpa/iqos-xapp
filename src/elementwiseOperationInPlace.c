/*
 * File: elementwiseOperationInPlace.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 12-Dec-2024 16:04:45
 */

/* Include Files */
#include "elementwiseOperationInPlace.h"
#include "rt_nonfinite.h"
#include "omp.h"
#include <math.h>

/* Function Declarations */
static void b_lambdaForColumnMajorGeneric(float *X);

static void d_lambdaForColumnMajorGeneric(float *X);

/* Function Definitions */
/*
 * Arguments    : float *X
 * Return Type  : void
 */
static void b_lambdaForColumnMajorGeneric(float *X)
{
  *X = 1.0F / (expf(-*X) + 1.0F);
}

/*
 * Arguments    : float *X
 * Return Type  : void
 */
static void d_lambdaForColumnMajorGeneric(float *X)
{
  *X = tanhf(*X);
}

/*
 * Arguments    : float X[400]
 * Return Type  : void
 */
void c_lambdaForColumnMajorGeneric(float X[400])
{
  int iElem;
#pragma omp parallel for num_threads(omp_get_max_threads())

  for (iElem = 0; iElem < 400; iElem++) {
    d_lambdaForColumnMajorGeneric(&X[iElem]);
  }
}

/*
 * Arguments    : float X[400]
 * Return Type  : void
 */
void e_lambdaForColumnMajorGeneric(float X[400])
{
  int iElem;
#pragma omp parallel for num_threads(omp_get_max_threads())

  for (iElem = 0; iElem < 400; iElem++) {
    X[iElem] = tanhf(X[iElem]);
  }
}

/*
 * Arguments    : float X[3]
 * Return Type  : void
 */
void f_lambdaForColumnMajorGeneric(float X[3])
{
  X[0] = expf(X[0]);
  X[1] = expf(X[1]);
  X[2] = expf(X[2]);
}

/*
 * Arguments    : float X[1200]
 * Return Type  : void
 */
void lambdaForColumnMajorGeneric(float X[1200])
{
  int iElem;
#pragma omp parallel for num_threads(omp_get_max_threads())

  for (iElem = 0; iElem < 1200; iElem++) {
    b_lambdaForColumnMajorGeneric(&X[iElem]);
  }
}

/*
 * File trailer for elementwiseOperationInPlace.c
 *
 * [EOF]
 */
