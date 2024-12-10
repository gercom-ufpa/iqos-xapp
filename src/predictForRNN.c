/*
 * File: predictForRNN.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 09-Dec-2024 16:20:26
 */

/* Include Files */
#include "predictForRNN.h"
#include "SoftmaxLayer.h"
#include "elementwiseOperationInPlace.h"
#include "rt_nonfinite.h"
#include "runPrediction_internal_types.h"
#include "runPrediction_types.h"
#include "omp.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

/* Function Declarations */
static char *computeFilePathUsingEnvVariable(const char *unresolvedFilePath);

static int div_nde_s32_floor(int numerator);

static int div_s32_floor(int numerator, int denominator);

static char *getCustomUserDataPathEnvVar(const char *unresolvedFilePath);

static int getPositionOfLastFileSeparator(const char *filePath);

static char *getRelativePathToParentFolder(const char *filePath);

static char *getResolvedFilePath(const char *unresolvedFilePath);

static void macroKernel1(int M, int K, int N, const float *A, int LDA,
                         const float *B, int LDB, float *C, int LDC);

static void macroKernel2(int M, int K, int N, const float *A, int LDA,
                         const float *B, int LDB, float *C, int LDC);

static void matrixMultiply1(int M, int K, int N, int blockSizeM, int blockSizeK,
                            int blockSizeN, const float *A, const float *B,
                            float *C);

static void matrixMultiply2(int M, int K, int N, int blockSizeM, int blockSizeK,
                            int blockSizeN, const float *A, const float *B,
                            float *C);

static void microKernel11(int K, const float *A, int LDA, const float *B,
                          float *C);

static void microKernel12(int K, const float *A, int LDA, const float *B,
                          float *C);

static void microKernel13(int K, const float *A, int LDA, const float *B,
                          float *C);

static void readDnnConstants(float *inputBufferPtr,
                             const char *unresolvedFilePath,
                             int numElementsToRead);

static char *resolveBinaryFilePath(const char *unresolvedFilePath);

static char *sanitizeFilePathForHSP(const char *unSanitizedFilePath);

static void stringConcat(char *destinationString, const char *sourceString,
                         size_t destBufferSize);

/* Function Definitions */
/*
 * Arguments    : const char *unresolvedFilePath
 * Return Type  : char *
 */
static char *computeFilePathUsingEnvVariable(const char *unresolvedFilePath)
{
  char *resolvedFilePath;
  char *stringDuplicate;
  size_t filePathLen;
  size_t sizeOfChar;
  filePathLen = strlen((char *)unresolvedFilePath) + 1;
  sizeOfChar = 1;
  stringDuplicate = (char *)calloc(filePathLen, sizeOfChar);
  stringConcat(stringDuplicate, unresolvedFilePath, filePathLen);
#if defined(MW_RUNTIME_DL_DATA_PATH) != 0
  extern char *mwGetRuntimeDLDataPath(const char *);
  resolvedFilePath = mwGetRuntimeDLDataPath((char *)unresolvedFilePath);
#elif defined(MW_DL_DATA_PATH) != 0
  resolvedFilePath = resolveBinaryFilePath(unresolvedFilePath);
#else
  char *coderDataPath;
  coderDataPath = getenv("CODER_DATA_PATH");
  if (coderDataPath != NULL) {
    resolvedFilePath = resolveBinaryFilePath(unresolvedFilePath);
  } else {
    resolvedFilePath = stringDuplicate;
  }
#endif
  return resolvedFilePath;
}

/*
 * Arguments    : int numerator
 * Return Type  : int
 */
static int div_nde_s32_floor(int numerator)
{
  int i;
  if ((numerator < 0) && (numerator % 28 != 0)) {
    i = -1;
  } else {
    i = 0;
  }
  return numerator / 28 + i;
}

/*
 * Arguments    : int numerator
 *                int denominator
 * Return Type  : int
 */
static int div_s32_floor(int numerator, int denominator)
{
  int quotient;
  if (denominator == 0) {
    if (numerator >= 0) {
      quotient = MAX_int32_T;
    } else {
      quotient = MIN_int32_T;
    }
  } else {
    unsigned int absDenominator;
    unsigned int absNumerator;
    unsigned int tempAbsQuotient;
    boolean_T quotientNeedsNegation;
    if (numerator < 0) {
      absNumerator = ~(unsigned int)numerator + 1U;
    } else {
      absNumerator = (unsigned int)numerator;
    }
    if (denominator < 0) {
      absDenominator = ~(unsigned int)denominator + 1U;
    } else {
      absDenominator = (unsigned int)denominator;
    }
    quotientNeedsNegation = ((numerator < 0) != (denominator < 0));
    tempAbsQuotient = absNumerator / absDenominator;
    if (quotientNeedsNegation) {
      absNumerator %= absDenominator;
      if (absNumerator > 0U) {
        tempAbsQuotient++;
      }
      quotient = -(int)tempAbsQuotient;
    } else {
      quotient = (int)tempAbsQuotient;
    }
  }
  return quotient;
}

/*
 * Arguments    : const char *unresolvedFilePath
 * Return Type  : char *
 */
static char *getCustomUserDataPathEnvVar(const char *unresolvedFilePath)
{
  const char *fileName;
  char *coderDataPath;
  char *resolvedFilePath;
  coderDataPath = getenv("CODER_DATA_PATH");
  if (coderDataPath != NULL) {
    int posOfLastPathSeparator;
    size_t filePathLength;
    size_t sizeOfChar;
    posOfLastPathSeparator = getPositionOfLastFileSeparator(unresolvedFilePath);
    fileName = &unresolvedFilePath[posOfLastPathSeparator];
    filePathLength = (strlen(coderDataPath) + strlen((char *)fileName)) + 1;
    sizeOfChar = 1;
    resolvedFilePath = (char *)calloc(filePathLength, sizeOfChar);
    stringConcat(resolvedFilePath, coderDataPath, filePathLength);
    stringConcat(resolvedFilePath, fileName, filePathLength);
  } else {
    resolvedFilePath = NULL;
  }
  return resolvedFilePath;
}

/*
 * Arguments    : const char *filePath
 * Return Type  : int
 */
static int getPositionOfLastFileSeparator(const char *filePath)
{
  int lastPathSeparatorUnix;
  int posOfLastPathSeparator;
  const char *ptrToLastPathSeparator;
  lastPathSeparatorUnix = '/';
  ptrToLastPathSeparator = strrchr((char *)filePath, lastPathSeparatorUnix);
  if (ptrToLastPathSeparator != NULL) {
    posOfLastPathSeparator = (int)(ptrToLastPathSeparator - filePath);
  } else {
    int lastPathSeparatorWindows;
    lastPathSeparatorWindows = '\\';
    ptrToLastPathSeparator =
        strrchr((char *)filePath, lastPathSeparatorWindows);
    if (ptrToLastPathSeparator != NULL) {
      posOfLastPathSeparator = (int)(ptrToLastPathSeparator - filePath);
    } else {
      posOfLastPathSeparator = -1;
    }
  }
  return posOfLastPathSeparator;
}

/*
 * Arguments    : const char *filePath
 * Return Type  : char *
 */
static char *getRelativePathToParentFolder(const char *filePath)
{
  int posOfLastPathSeparator;
  const char *fileName;
  const char *parentDir;
  char *resolvedFilePath;
  size_t filePathLength;
  size_t sizeOfChar;
  parentDir = "..";
  posOfLastPathSeparator = getPositionOfLastFileSeparator(filePath);
  fileName = &filePath[posOfLastPathSeparator];
  filePathLength = (strlen((char *)parentDir) + strlen((char *)fileName)) + 1;
  sizeOfChar = 1;
  resolvedFilePath = (char *)calloc(filePathLength, sizeOfChar);
  stringConcat(resolvedFilePath, parentDir, filePathLength);
  stringConcat(resolvedFilePath, fileName, filePathLength);
  return resolvedFilePath;
}

/*
 * Arguments    : const char *unresolvedFilePath
 * Return Type  : char *
 */
static char *getResolvedFilePath(const char *unresolvedFilePath)
{
  const char *fileOpenMode;
  char *computedPathUsingEnvVars;
  char *pathUsingEnvVarAndSanitizedPath;
  char *relativePathToParent;
  char *resolvedFilePath;
  char *sanitizedFilePath;
  char *stringDuplicate;
  FILE *filePtr;
  resolvedFilePath = NULL;
  fileOpenMode = "rb";
  filePtr = fopen((char *)unresolvedFilePath, (char *)fileOpenMode);
  if (filePtr) {
    size_t filePathLen;
    size_t sizeOfChar;
    filePathLen = strlen((char *)unresolvedFilePath) + 1;
    sizeOfChar = 1;
    stringDuplicate = (char *)calloc(filePathLen, sizeOfChar);
    stringConcat(stringDuplicate, unresolvedFilePath, filePathLen);
    resolvedFilePath = stringDuplicate;
    fclose(filePtr);
  } else {
    computedPathUsingEnvVars =
        computeFilePathUsingEnvVariable(unresolvedFilePath);
    filePtr = fopen(computedPathUsingEnvVars, (char *)fileOpenMode);
    if (filePtr) {
      resolvedFilePath = computedPathUsingEnvVars;
      fclose(filePtr);
    } else {
      free(computedPathUsingEnvVars);
      sanitizedFilePath = sanitizeFilePathForHSP(unresolvedFilePath);
      filePtr = fopen(sanitizedFilePath, (char *)fileOpenMode);
      if (filePtr) {
        resolvedFilePath = sanitizedFilePath;
        fclose(filePtr);
      } else {
        relativePathToParent =
            getRelativePathToParentFolder(unresolvedFilePath);
        filePtr = fopen(relativePathToParent, (char *)fileOpenMode);
        if (filePtr) {
          resolvedFilePath = relativePathToParent;
          fclose(filePtr);
        } else {
          free(relativePathToParent);
          pathUsingEnvVarAndSanitizedPath =
              computeFilePathUsingEnvVariable(sanitizedFilePath);
          filePtr =
              fopen(pathUsingEnvVarAndSanitizedPath, (char *)fileOpenMode);
          if (filePtr) {
            resolvedFilePath = pathUsingEnvVarAndSanitizedPath;
            fclose(filePtr);
          } else {
            free(pathUsingEnvVarAndSanitizedPath);
            exit(EXIT_FAILURE);
          }
        }
      }
    }
  }
  return resolvedFilePath;
}

/*
 * Arguments    : int M
 *                int K
 *                int N
 *                const float *A
 *                int LDA
 *                const float *B
 *                int LDB
 *                float *C
 *                int LDC
 * Return Type  : void
 */
static void macroKernel1(int M, int K, int N, const float *A, int LDA,
                         const float *B, int LDB, float *C, int LDC)
{
  int idxB;
  int j;
  j = 0;
  idxB = 0;
  while (j <= N - 1) {
    int i;
    int idxA;
    int idxC;
    idxC = LDC * j;
    i = 0;
    idxA = 0;
    while (i <= M - 28) {
      microKernel11(K, &A[idxA], LDA, &B[idxB], &C[idxC]);
      idxA += 28;
      idxC += 28;
      i += 28;
    }
    while (i <= M - 4) {
      microKernel12(K, &A[idxA], LDA, &B[idxB], &C[idxC]);
      idxA += 4;
      idxC += 4;
      i += 4;
    }
    while (i <= M - 1) {
      microKernel13(K, &A[idxA], LDA, &B[idxB], &C[idxC]);
      idxA++;
      idxC++;
      i++;
    }
    idxB += LDB;
    j++;
  }
}

/*
 * Arguments    : int M
 *                int K
 *                int N
 *                const float *A
 *                int LDA
 *                const float *B
 *                int LDB
 *                float *C
 *                int LDC
 * Return Type  : void
 */
static void macroKernel2(int M, int K, int N, const float *A, int LDA,
                         const float *B, int LDB, float *C, int LDC)
{
  int idxB;
  int j;
  j = 0;
  idxB = 0;
  while (j <= N - 1) {
    int i;
    int idxA;
    int idxC;
    idxC = LDC * j;
    i = 0;
    idxA = 0;
    while (i <= M - 4) {
      microKernel12(K, &A[idxA], LDA, &B[idxB], &C[idxC]);
      idxA += 4;
      idxC += 4;
      i += 4;
    }
    while (i <= M - 1) {
      microKernel13(K, &A[idxA], LDA, &B[idxB], &C[idxC]);
      idxA++;
      idxC++;
      i++;
    }
    idxB += LDB;
    j++;
  }
}

/*
 * Arguments    : int M
 *                int K
 *                int N
 *                int blockSizeM
 *                int blockSizeK
 *                int blockSizeN
 *                const float *A
 *                const float *B
 *                float *C
 * Return Type  : void
 */
static void matrixMultiply1(int M, int K, int N, int blockSizeM, int blockSizeK,
                            int blockSizeN, const float *A, const float *B,
                            float *C)
{
  const float *ptrB;
  int b_i;
  int b_j1;
  int i;
  int i0;
  int i0_ub;
  int k0;
  int k0_ub;
  memset(C, 0, (unsigned int)((M * N) << 2));
  if (blockSizeM >= M) {
    blockSizeM = M;
  } else {
    blockSizeM = div_nde_s32_floor(blockSizeM) * 28;
    if (blockSizeM <= 0) {
      blockSizeM = 1;
    }
  }
  if (blockSizeN >= N) {
    blockSizeN = N;
  } else if (blockSizeN <= 0) {
    blockSizeN = 1;
  }
  i0_ub = div_s32_floor(M - 1, blockSizeM) + 1;
  k0_ub = div_s32_floor(K - 1, blockSizeK) + 1;
  for (b_j1 = 0; b_j1 < N; b_j1 += blockSizeN) {
    int N2;
    if (b_j1 > N - blockSizeN) {
      N2 = N - b_j1;
    } else {
      N2 = blockSizeN;
    }
    for (k0 = 1; k0 <= k0_ub; k0++) {
      int K2;
      int k;
      k = (k0 - 1) * blockSizeK;
      if (k > K - blockSizeK) {
        K2 = K - k;
      } else {
        K2 = blockSizeK;
      }
      ptrB = &B[k + K * b_j1];
#pragma omp parallel for num_threads(omp_get_max_threads()) private(i, b_i)

      for (i0 = 1; i0 <= i0_ub; i0++) {
        i = (i0 - 1) * blockSizeM;
        if (i > M - blockSizeM) {
          b_i = M - i;
        } else {
          b_i = blockSizeM;
        }
        macroKernel1(b_i, K2, N2, &A[i + M * k], M, ptrB, K, &C[i + M * b_j1],
                     M);
      }
    }
  }
}

/*
 * Arguments    : int M
 *                int K
 *                int N
 *                int blockSizeM
 *                int blockSizeK
 *                int blockSizeN
 *                const float *A
 *                const float *B
 *                float *C
 * Return Type  : void
 */
static void matrixMultiply2(int M, int K, int N, int blockSizeM, int blockSizeK,
                            int blockSizeN, const float *A, const float *B,
                            float *C)
{
  const float *ptrB;
  int b_i;
  int b_j1;
  int i;
  int i0;
  int i0_ub;
  int k0;
  int k0_ub;
  memset(C, 0, (unsigned int)((M * N) << 2));
  if (blockSizeM >= M) {
    blockSizeM = M;
  } else {
    blockSizeM = (blockSizeM >> 2) << 2;
    if (blockSizeM <= 0) {
      blockSizeM = 1;
    }
  }
  if (blockSizeN >= N) {
    blockSizeN = N;
  } else if (blockSizeN <= 0) {
    blockSizeN = 1;
  }
  i0_ub = div_s32_floor(M - 1, blockSizeM) + 1;
  k0_ub = div_s32_floor(K - 1, blockSizeK) + 1;
  for (b_j1 = 0; b_j1 < N; b_j1 += blockSizeN) {
    int N2;
    if (b_j1 > N - blockSizeN) {
      N2 = N - b_j1;
    } else {
      N2 = blockSizeN;
    }
    for (k0 = 1; k0 <= k0_ub; k0++) {
      int K2;
      int k;
      k = (k0 - 1) * blockSizeK;
      if (k > K - blockSizeK) {
        K2 = K - k;
      } else {
        K2 = blockSizeK;
      }
      ptrB = &B[k + K * b_j1];
#pragma omp parallel for num_threads(omp_get_max_threads()) private(i, b_i)

      for (i0 = 1; i0 <= i0_ub; i0++) {
        i = (i0 - 1) * blockSizeM;
        if (i > M - blockSizeM) {
          b_i = M - i;
        } else {
          b_i = blockSizeM;
        }
        macroKernel2(b_i, K2, N2, &A[i + M * k], M, ptrB, K, &C[i + M * b_j1],
                     M);
      }
    }
  }
}

/*
 * Arguments    : int K
 *                const float *A
 *                int LDA
 *                const float *B
 *                float *C
 * Return Type  : void
 */
static void microKernel11(int K, const float *A, int LDA, const float *B,
                          float *C)
{
  __m128 b_c;
  __m128 c;
  __m128 c_c;
  __m128 d_c;
  __m128 e_c;
  __m128 f_c;
  __m128 g_c;
  int idxA;
  int idxB;
  int k;
  idxA = 0;
  idxB = 0;
  c = _mm_loadu_ps(&C[0]);
  b_c = _mm_loadu_ps(&C[4]);
  c_c = _mm_loadu_ps(&C[8]);
  d_c = _mm_loadu_ps(&C[12]);
  e_c = _mm_loadu_ps(&C[16]);
  f_c = _mm_loadu_ps(&C[20]);
  g_c = _mm_loadu_ps(&C[24]);
  for (k = 0; k < K; k++) {
    __m128 aFloat;
    __m128 b;
    __m128 b_aFloat;
    __m128 c_aFloat;
    __m128 d_aFloat;
    __m128 e_aFloat;
    __m128 f_aFloat;
    __m128 g_aFloat;
    float bFloat;
    aFloat = _mm_loadu_ps(&A[idxA]);
    b_aFloat = _mm_loadu_ps(&A[idxA + 4]);
    c_aFloat = _mm_loadu_ps(&A[idxA + 8]);
    d_aFloat = _mm_loadu_ps(&A[idxA + 12]);
    e_aFloat = _mm_loadu_ps(&A[idxA + 16]);
    f_aFloat = _mm_loadu_ps(&A[idxA + 20]);
    g_aFloat = _mm_loadu_ps(&A[idxA + 24]);
    bFloat = B[idxB];
    b = _mm_set1_ps(bFloat);
    c = _mm_add_ps(c, _mm_mul_ps(aFloat, b));
    b_c = _mm_add_ps(b_c, _mm_mul_ps(b_aFloat, b));
    c_c = _mm_add_ps(c_c, _mm_mul_ps(c_aFloat, b));
    d_c = _mm_add_ps(d_c, _mm_mul_ps(d_aFloat, b));
    e_c = _mm_add_ps(e_c, _mm_mul_ps(e_aFloat, b));
    f_c = _mm_add_ps(f_c, _mm_mul_ps(f_aFloat, b));
    g_c = _mm_add_ps(g_c, _mm_mul_ps(g_aFloat, b));
    idxA += LDA;
    idxB++;
  }
  _mm_storeu_ps(&C[0], c);
  _mm_storeu_ps(&C[4], b_c);
  _mm_storeu_ps(&C[8], c_c);
  _mm_storeu_ps(&C[12], d_c);
  _mm_storeu_ps(&C[16], e_c);
  _mm_storeu_ps(&C[20], f_c);
  _mm_storeu_ps(&C[24], g_c);
}

/*
 * Arguments    : int K
 *                const float *A
 *                int LDA
 *                const float *B
 *                float *C
 * Return Type  : void
 */
static void microKernel12(int K, const float *A, int LDA, const float *B,
                          float *C)
{
  __m128 c;
  int idxA;
  int idxB;
  int k;
  idxA = 0;
  idxB = 0;
  c = _mm_loadu_ps(&C[0]);
  for (k = 0; k < K; k++) {
    __m128 aFloat;
    float bFloat;
    aFloat = _mm_loadu_ps(&A[idxA]);
    bFloat = B[idxB];
    c = _mm_add_ps(c, _mm_mul_ps(aFloat, _mm_set1_ps(bFloat)));
    idxA += LDA;
    idxB++;
  }
  _mm_storeu_ps(&C[0], c);
}

/*
 * Arguments    : int K
 *                const float *A
 *                int LDA
 *                const float *B
 *                float *C
 * Return Type  : void
 */
static void microKernel13(int K, const float *A, int LDA, const float *B,
                          float *C)
{
  float c;
  int idxA;
  int idxB;
  int k;
  idxA = 0;
  idxB = 0;
  c = C[0];
  for (k = 0; k < K; k++) {
    float aFloat;
    float b;
    aFloat = A[idxA];
    b = B[idxB];
    c += aFloat * b;
    idxA += LDA;
    idxB++;
  }
  C[0] = c;
}

/*
 * Arguments    : float *inputBufferPtr
 *                const char *unresolvedFilePath
 *                int numElementsToRead
 * Return Type  : void
 */
static void readDnnConstants(float *inputBufferPtr,
                             const char *unresolvedFilePath,
                             int numElementsToRead)
{
  int elementSizeInBytes;
  const char *fileOpenMode;
  char *resolvedFilePath;
  FILE *filePtr;
  void *dataBufferPtr;
  resolvedFilePath = getResolvedFilePath(unresolvedFilePath);
  fileOpenMode = "rb";
  filePtr = fopen(resolvedFilePath, (char *)fileOpenMode);
  dataBufferPtr = &inputBufferPtr[0];
  elementSizeInBytes = 4;
  fread(dataBufferPtr, elementSizeInBytes, numElementsToRead, filePtr);
  fclose(filePtr);
  free(resolvedFilePath);
}

/*
 * Arguments    : const char *unresolvedFilePath
 * Return Type  : char *
 */
static char *resolveBinaryFilePath(const char *unresolvedFilePath)
{
  const char *c_filePathAfterSlicingRelativeP;
  const char *c_leadingPathSeparatorUnixAndWi;
  const char *codegenDirStrInMWDLDataPath;
  const char *d_filePathAfterSlicingRelativeP;
  const char *mwDLDataPath;
  char *codegenDir;
  char *coderDataPath;
  char *resolvedFilePath;
  char *updatedStartDir;
  size_t sizeOfChar;
#define XSTR(x) #x
#define STR(x) XSTR(x)
  coderDataPath = getenv("CODER_DATA_PATH");
  sizeOfChar = 1;
  if (coderDataPath != NULL) {
    resolvedFilePath = getCustomUserDataPathEnvVar(unresolvedFilePath);
  } else {
    size_t filePathLen;
    size_t posOfCodegenDir;
    size_t posOfLeadingPathSeparator;
    mwDLDataPath = STR(MW_DL_DATA_PATH);
    c_filePathAfterSlicingRelativeP = &unresolvedFilePath[2];
    c_leadingPathSeparatorUnixAndWi = "/\\";
    posOfLeadingPathSeparator =
        strcspn((char *)c_filePathAfterSlicingRelativeP,
                (char *)c_leadingPathSeparatorUnixAndWi);
    filePathLen = posOfLeadingPathSeparator + 1;
    codegenDir = (char *)calloc(filePathLen, sizeOfChar);
    strncpy(codegenDir, (char *)c_filePathAfterSlicingRelativeP,
            posOfLeadingPathSeparator);
    codegenDirStrInMWDLDataPath = strstr((char *)mwDLDataPath, codegenDir);
    if (codegenDirStrInMWDLDataPath == NULL) {
      posOfCodegenDir = strlen((char *)mwDLDataPath);
    } else {
      posOfCodegenDir = codegenDirStrInMWDLDataPath - mwDLDataPath;
    }
    if (posOfCodegenDir == strlen((char *)mwDLDataPath)) {
      size_t b_filePathLen;
      d_filePathAfterSlicingRelativeP = &unresolvedFilePath[1];
      b_filePathLen = (strlen((char *)mwDLDataPath) +
                       strlen((char *)d_filePathAfterSlicingRelativeP)) +
                      1;
      resolvedFilePath = (char *)calloc(b_filePathLen, sizeOfChar);
      stringConcat(resolvedFilePath, mwDLDataPath, b_filePathLen);
      stringConcat(resolvedFilePath, d_filePathAfterSlicingRelativeP,
                   b_filePathLen);
    } else {
      size_t c_filePathLen;
      c_filePathLen = posOfCodegenDir + 1;
      updatedStartDir = (char *)calloc(c_filePathLen, sizeOfChar);
      strncpy(updatedStartDir, (char *)mwDLDataPath, posOfCodegenDir);
      c_filePathLen = (strlen(updatedStartDir) +
                       strlen((char *)c_filePathAfterSlicingRelativeP)) +
                      1;
      resolvedFilePath = (char *)calloc(c_filePathLen, sizeOfChar);
      stringConcat(resolvedFilePath, updatedStartDir, c_filePathLen);
      stringConcat(resolvedFilePath, c_filePathAfterSlicingRelativeP,
                   c_filePathLen);
      free(updatedStartDir);
    }
    free(codegenDir);
  }
#undef XSTR
#undef STR
  return resolvedFilePath;
}

/*
 * Arguments    : const char *unSanitizedFilePath
 * Return Type  : char *
 */
static char *sanitizeFilePathForHSP(const char *unSanitizedFilePath)
{
  char *sanitizedFilePath;
  char *stringDuplicate;
  size_t charIdx;
  size_t filePathLen;
  size_t sizeOfChar;
  filePathLen = strlen((char *)unSanitizedFilePath) + 1;
  sizeOfChar = 1;
  stringDuplicate = (char *)calloc(filePathLen, sizeOfChar);
  stringConcat(stringDuplicate, unSanitizedFilePath, filePathLen);
  sanitizedFilePath = stringDuplicate;
  for (charIdx = 0; charIdx < strlen((char *)unSanitizedFilePath); charIdx++) {
    char charToCheckFor;
    charToCheckFor = unSanitizedFilePath[charIdx];
    if (isspace(charToCheckFor)) {
      sanitizedFilePath[charIdx] = '_';
    }
  }
  return sanitizedFilePath;
}

/*
 * Arguments    : char *destinationString
 *                const char *sourceString
 *                size_t destBufferSize
 * Return Type  : void
 */
static void stringConcat(char *destinationString, const char *sourceString,
                         size_t destBufferSize)
{
  size_t dstStringLen;
  size_t srcBuffIdx;
  dstStringLen = strlen(destinationString);
  srcBuffIdx = 0;
  while ((sourceString[srcBuffIdx] != '\x00') &&
         (dstStringLen < destBufferSize - 1)) {
    destinationString[dstStringLen] = sourceString[srcBuffIdx];
    dstStringLen++;
    srcBuffIdx++;
  }
  destinationString[dstStringLen] = '\x00';
}

/*
 * Arguments    : c_coder_internal_ctarget_DeepLe *obj
 *                const cell_wrap_7 *inputs
 *                float outputData[3]
 * Return Type  : void
 */
void c_DeepLearningNetwork_predictFo(c_coder_internal_ctarget_DeepLe *obj,
                                     const cell_wrap_7 *inputs,
                                     float outputData[3])
{
  static const float inputGateWeights[6000] = {
      0.00044601594F,   0.00403304072F,   -0.00619178172F,  0.0014839127F,
      0.00250688125F,   0.00885549188F,   -3.9817769E-7F,   -0.0229800735F,
      0.00659587F,      0.00396671053F,   0.00600356748F,   0.001224512F,
      0.00152880454F,   -0.000225110649F, -0.00217756839F,  -4.34422645E-7F,
      0.00565090077F,   -4.20087872E-6F,  0.00255841739F,   0.000656614429F,
      0.000733093882F,  -0.00776264211F,  -0.00699775433F,  -0.00648181746F,
      0.00513019133F,   0.00487823505F,   -5.34123728E-7F,  9.00084046E-7F,
      -3.87612181E-6F,  0.0080604637F,    0.0394937135F,    -0.00226010336F,
      -0.000171347347F, -1.51091126E-6F,  0.00096948829F,   0.00622721482F,
      8.4607811E-5F,    0.0210746247F,    -3.01848604E-6F,  -0.000333516538F,
      -5.60556941E-7F,  0.00452452386F,   -0.00158894795F,  0.000427111547F,
      -0.0111688264F,   -0.00134928315F,  -0.0038639328F,   -1.08634572E-6F,
      -0.00214216765F,  -1.60339869E-6F,  0.00143590209F,   0.0612179674F,
      -0.00376372971F,  -0.0271922555F,   -4.98455563E-7F,  -0.00210440252F,
      0.00161503954F,   -0.0091000665F,   -7.67394454E-7F,  -0.004496946F,
      0.00642299699F,   0.0170432739F,    -0.0174060892F,   0.00205277978F,
      0.000706158869F,  -0.00172392558F,  -0.00125239161F,  0.00135157153F,
      0.00528772501F,   -3.69691479E-5F,  -0.0109063135F,   -0.000899147301F,
      -0.00027481874F,  0.00255760853F,   -0.00694421679F,  -1.46377602E-6F,
      -0.000517086417F, 0.00160455576F,   -0.00229065865F,  -0.0032140438F,
      0.00357854739F,   -0.00893511716F,  0.00121816364F,   -1.19261244E-7F,
      -0.017902568F,    -0.00633376604F,  -1.21696758E-6F,  0.000357259851F,
      -6.32585943E-5F,  0.01326166F,      -3.30741796E-6F,  -0.00247566565F,
      -0.00470559346F,  0.0071639535F,    -3.45921171E-6F,  0.0134699149F,
      -4.05555397E-7F,  9.04956551E-7F,   -0.0027969745F,   0.0321455337F,
      -1.44356363E-7F,  -0.00256433804F,  -0.000106387466F, -2.63927086E-6F,
      0.00361276069F,   1.33314472E-6F,   0.00228889077F,   0.00508684805F,
      -0.00380642898F,  -0.00627919612F,  0.0068598385F,    4.29509091E-5F,
      -3.79591665E-6F,  -7.14754833E-8F,  -0.000953803F,    -1.99451506E-6F,
      0.00177817978F,   -0.00108038087F,  0.00432015443F,   -1.46330962E-7F,
      -0.00273765018F,  -2.82082306E-6F,  -1.52078055E-5F,  0.00896352436F,
      -5.209341E-6F,    0.00146659557F,   -5.42031967E-6F,  -2.1882488E-6F,
      0.000726355414F,  -0.000678471872F, -0.0164033361F,   -6.68058533E-7F,
      -0.00352839264F,  -0.00865095854F,  0.000443798082F,  -4.25128803E-7F,
      -0.000792418083F, -0.00103905413F,  -4.66534311E-6F,  -0.0175101627F,
      -0.00888886F,     0.00122551958F,   -0.00625586463F,  -0.000598369574F,
      -0.00784683414F,  -0.00381323928F,  0.00125261385F,   -0.00714591658F,
      -0.00399956293F,  0.00416362F,      -0.000769892649F, 0.00221914309F,
      -7.07003928E-5F,  -0.000117696691F, 5.73528378E-5F,   -0.000327624381F,
      0.0101592233F,    0.00725124963F,   -0.00323688518F,  -0.00792867411F,
      0.0345199257F,    -0.00134398311F,  0.00479584F,      0.00586899789F,
      0.00374879269F,   -0.00126800325F,  -0.00710552232F,  -0.0030762672F,
      0.0042262231F,    0.000440212723F,  0.00231395196F,   -0.00972686801F,
      0.0110570788F,    -0.000795294181F, -2.56158855E-5F,  0.0129685011F,
      -0.000193906351F, -0.00226757186F,  0.00201319158F,   -0.000237243759F,
      -2.19205094E-6F,  0.0172012597F,    -0.0232599881F,   0.00235992158F,
      0.000218648318F,  6.22452135E-5F,   -2.77084837E-6F,  0.0062553403F,
      -1.53528072E-5F,  -2.20240082E-7F,  0.00291056675F,   -0.000552596932F,
      -0.0172906946F,   0.00226585311F,   0.00260774582F,   -3.75512536E-6F,
      -7.55554595E-7F,  0.00754279085F,   0.00169299508F,   0.000662230828F,
      -0.000773513457F, 1.35126402E-6F,   0.00932215806F,   0.00370460702F,
      0.00199373043F,   -0.00287150149F,  -0.00192921574F,  0.000142380304F,
      0.00282465434F,   -0.000246551033F, 6.03974149E-6F,   0.000781145296F,
      -1.15850185E-6F,  0.0021591105F,    -5.11654E-7F,     0.00350950472F,
      0.000280724838F,  -0.00494891126F,  0.00484984554F,   -1.07090164E-5F,
      -0.00159755291F,  0.000448742037F,  -0.0133836595F,   -0.000372318289F,
      0.0046543628F,    -0.0125467433F,   -0.00329737226F,  -0.00800788496F,
      -0.0135111921F,   0.00166376494F,   -2.59892704E-6F,  -0.0100707207F,
      -1.77422703E-6F,  -2.39665042E-6F,  -9.0196E-7F,      0.00680850307F,
      0.0016967596F,    0.00473086769F,   0.00904411823F,   -3.44586661E-5F,
      -0.00183650211F,  0.00102777802F,   -0.00176703359F,  -0.0066905F,
      6.10880234E-5F,   -9.74912879E-8F,  -0.000720007287F, 0.000237255124F,
      0.00137516542F,   -4.28949352E-6F,  -3.24889243E-7F,  -0.00317847775F,
      0.0011964516F,    -0.00482127396F,  0.00822059531F,   0.000911437324F,
      4.88400474E-7F,   -0.00290004932F,  0.00282944925F,   0.00389728695F,
      0.00084461685F,   3.46948377E-6F,   -0.0246206447F,   -0.00112010015F,
      0.00153915992F,   0.000780391158F,  -0.00225080526F,  -7.98161892E-8F,
      0.0140569443F,    -0.000116526135F, 0.00326333381F,   0.00416277116F,
      -0.0174348783F,   -0.000491248094F, -8.28777556E-5F,  -0.000243208749F,
      0.00184364745F,   0.019606445F,     -0.00786899216F,  0.00509085879F,
      0.00237076823F,   0.000968612731F,  0.0024901412F,    0.00262318808F,
      2.88741376E-6F,   0.00431540282F,   -0.00440086937F,  -0.00971804466F,
      -0.0036715F,      -0.0120877773F,   -0.0182138439F,   0.0233320426F,
      -4.41131817E-7F,  0.00621575071F,   -0.000135592301F, 0.00558511F,
      0.0009142739F,    0.00709181419F,   -0.00620832341F,  -0.00158360775F,
      -0.00532616349F,  -0.0128183533F,   -0.0147193652F,   0.0166359972F,
      -0.00219286699F,  0.006735784F,     -0.00422810577F,  0.0111809606F,
      -0.012390567F,    0.00251827342F,   0.000364416599F,  -1.39889812E-6F,
      0.00104480307F,   0.00212049065F,   -0.000831339159F, -0.00902691204F,
      0.00465840893F,   -0.0205623414F,   0.00823173858F,   -7.56061672E-6F,
      0.003419057F,     -0.00649617054F,  0.00164655212F,   -1.52164775E-5F,
      2.53805974E-5F,   -1.02349186E-5F,  -0.0103538623F,   -0.00752866175F,
      -0.0126274507F,   0.00268844119F,   -0.000403586746F, -0.014398695F,
      0.00663021347F,   0.000675941643F,  -0.0117518129F,   -1.72213936E-6F,
      -2.45178308E-6F,  -8.45172908E-5F,  -5.23637596E-7F,  0.000188832462F,
      -3.60361737E-5F,  -0.000564265356F, -0.000830929785F, -0.00159173424F,
      -3.42124537E-7F,  -0.00657392293F,  -6.40805911E-6F,  -0.000123162012F,
      0.000743478886F,  0.0169642083F,    -0.0187823586F,   0.00274199061F,
      0.00393164624F,   -0.00391469058F,  -9.14292E-7F,     0.000305936584F,
      0.000105275489F,  0.0034187038F,    -0.00028693286F,  -0.000807291828F,
      -0.003407693F,    0.0064375517F,    -0.000887569506F, -0.00243085669F,
      -1.93314122E-7F,  -0.0128572928F,   -0.0132733872F,   -0.00466428185F,
      0.000677432923F,  0.00361359655F,   0.00210315199F,   -0.000996757532F,
      0.00151263049F,   -0.000922008185F, 6.86364274E-7F,   -0.0077307024F,
      -0.0018489873F,   0.00458968291F,   0.00244405633F,   0.00896103401F,
      -0.000205001139F, 0.0054406235F,    1.85801564E-5F,   -4.90643401E-7F,
      5.90128548E-5F,   -0.0143017778F,   -9.53932272E-7F,  0.00325699407F,
      -0.00283273286F,  -0.0201814324F,   -0.000499195594F, 0.00330122071F,
      -0.0098885335F,   -2.38822099E-6F,  0.00513753109F,   0.00385494763F,
      0.00402492937F,   -0.000263126829F, -0.00192754215F,  0.0011895427F,
      0.000227948112F,  0.000799668545F,  -0.00303482101F,  0.00108073419F,
      -6.38960701E-5F,  -0.000426446466F, 7.45894909E-7F,   -0.00948847923F,
      0.00137767009F,   -0.00596776744F,  0.00175622886F,   0.0095680831F,
      0.00404622F,      -0.000117353251F, -0.000752475753F, 1.22445467E-8F,
      0.00260047149F,   4.72219085E-7F,   0.00126618298F,   0.0126591166F,
      0.00262632151F,   -0.00222981069F,  -0.00156658678F,  -0.000661146129F,
      -0.019904634F,    0.0118474374F,    3.34403865E-8F,   7.40874917E-9F,
      1.02799458E-8F,   -0.0269376412F,   0.00865475181F,   0.0296540447F,
      -0.000195236804F, 9.10584603E-8F,   0.000306376052F,  -0.00228346442F,
      -0.000735809619F, 0.025790818F,     -5.65839E-9F,     -0.000323836051F,
      -5.9036056E-9F,   0.00145833695F,   -0.00270602619F,  1.587521E-5F,
      -0.00316337869F,  -7.9233927E-5F,   0.0784761831F,    1.49452859E-7F,
      0.00509685697F,   -7.02822263E-8F,  0.0199202038F,    0.000208153127F,
      -0.000981803285F, -0.00582094165F,  7.46312949E-8F,   -0.0306594539F,
      -0.000179964103F, -0.00232896232F,  8.61488E-8F,      -0.00119317428F,
      0.00467829034F,   -0.00111283874F,  -0.00143657904F,  6.19252169E-5F,
      -1.7902099E-5F,   -0.000520548492F, -0.000507472374F, 0.00150266406F,
      -0.000170376908F, -0.000306304515F, -0.00245722872F,  -0.0475514345F,
      -0.000119305201F, -0.000634898548F, -0.00491998391F,  -9.29123E-8F,
      -0.00111413829F,  0.00418029213F,   0.000184862525F,  0.000317034224F,
      -0.0166640989F,   -0.0116699608F,   -0.000691915222F, -3.71358304E-8F,
      -0.000874438032F, -0.00195146236F,  -1.15756258E-8F,  0.000945399F,
      -5.44457E-5F,     0.00310131162F,   -1.27636177E-8F,  0.0037007411F,
      -0.00317960628F,  5.68189862E-5F,   -4.13320933E-8F,  0.00269905408F,
      1.70104087E-7F,   8.0427E-8F,       -0.000388645247F, 0.00116753194F,
      -4.28739284E-8F,  -0.000763470249F, -0.000145933431F, 3.00438217E-7F,
      0.0231312867F,    -9.26850596E-10F, 0.00148155936F,   0.0118817929F,
      -0.00541683F,     -0.00177274144F,  -2.72566085E-5F,  -5.56940977E-5F,
      1.40526399E-8F,   -5.87941651E-9F,  -0.000300722837F, -2.98474312E-9F,
      0.000458876952F,  0.00108826312F,   -0.00589698646F,  1.46871022E-8F,
      -0.00926568F,     -9.50019086E-8F,  -3.2845854E-8F,   0.000275283179F,
      -2.76153731E-8F,  0.0208138432F,    -4.95525327E-8F,  1.97945383E-7F,
      0.000471461855F,  -0.00354812131F,  -0.00384971034F,  -1.40809124E-8F,
      -0.00184052414F,  -0.000728447281F, -0.000485461293F, -2.17701839E-8F,
      -0.00211896095F,  -0.00199679425F,  -3.04357606E-8F,  0.00537661789F,
      -0.00363908545F,  0.000356527453F,  -0.00361118605F,  -0.00167695654F,
      -0.00344710681F,  -0.00186985126F,  0.000231977174F,  -0.00452610338F,
      -0.00384625699F,  0.000428448489F,  0.00234204112F,   0.00149721315F,
      -3.47729042E-7F,  -0.000180450763F, 0.0208425298F,    -0.00233887415F,
      0.00107633253F,   0.0059975232F,    -0.00205810973F,  -0.0041556051F,
      -0.00107404706F,  -0.000106101725F, -0.020539321F,    -0.0345827676F,
      0.0206029303F,    0.00940939318F,   -0.00312822079F,  0.000944325235F,
      0.00137458439F,   9.63162747E-5F,   -0.0225336589F,   -0.0012046101F,
      0.00292600016F,   -0.000394263072F, -0.000268669566F, 0.00239148969F,
      0.000257840904F,  -0.00138285302F,  0.000324889348F,  0.000555230246F,
      1.49666164E-8F,   0.000374970637F,  -0.00498280255F,  0.000441573502F,
      -4.73678374E-5F,  0.000306818198F,  1.81770531E-7F,   -0.00045126793F,
      -1.11163297E-6F,  2.63559379E-8F,   0.0120196808F,    -0.000524076F,
      -0.00326497457F,  0.0015221904F,    0.0023963016F,    -5.5738159E-8F,
      -8.07000866E-9F,  -0.0204575658F,   0.0106340637F,    0.0135668F,
      -0.000666233245F, -2.05886039E-7F,  0.000196921988F,  0.000729000603F,
      0.000660884834F,  -0.000251017569F, 0.00296351197F,   0.0155093269F,
      -0.000339954393F, -0.000146005885F, 5.65640903E-7F,   0.000411252491F,
      -1.8906869E-8F,   0.0262499042F,    -5.3527982E-9F,   -0.00510656973F,
      -4.22978337E-5F,  -0.000970679102F, 0.00278945314F,   1.77924846E-6F,
      0.00061105995F,   -2.44717194E-6F,  -0.00366441277F,  0.00141111691F,
      0.00674163457F,   -0.00367616187F,  -0.00275225728F,  -0.000927359739F,
      -0.00404892955F,  0.00945262518F,   -2.45112801E-7F,  -0.0035885924F,
      -7.4099276E-8F,   1.90859719E-8F,   9.74856604E-8F,   0.00111795473F,
      0.00125489279F,   0.0157726426F,    0.0020439066F,    -6.74821695E-6F,
      -0.000485271041F, 0.0100440076F,    -0.00165950891F,  -0.00356348325F,
      -0.000116802737F, -3.12510817E-9F,  -0.00124281272F,  2.67090581E-6F,
      1.85725912E-6F,   1.73980879E-7F,   6.40503739E-9F,   -0.000934378942F,
      -0.00444950303F,  -0.00326014869F,  0.00117730559F,   0.0150829107F,
      -3.41503323E-8F,  -0.00141890184F,  0.0127233779F,    0.021129109F,
      -1.601652E-6F,    -4.04141E-7F,     -0.00662839785F,  -0.000614294084F,
      0.00128915242F,   0.0476949364F,    -0.00152271707F,  5.64480285E-9F,
      0.0203892905F,    -0.000104292711F, 6.36244949E-5F,   0.0115428101F,
      -0.00802622F,     -0.00144844677F,  -0.000103826125F, 2.95146638E-5F,
      0.000559511478F,  0.00135639205F,   -0.00244029611F,  0.00257086079F,
      -0.017482996F,    0.01185025F,      0.000434406335F,  0.0172155555F,
      -3.91017778E-7F,  -0.000439447438F, -0.00190354511F,  -0.00163398602F,
      0.000750231266F,  -0.00380386924F,  -0.00373294787F,  0.00317878765F,
      -1.50928763E-8F,  0.00547034573F,   -0.000149125917F, 0.00133635395F,
      0.0203968398F,    0.000292498531F,  -0.0025424615F,   9.42811166E-5F,
      -0.008307321F,    -0.00445914827F,  0.00219271309F,   0.00242775236F,
      -0.000932803261F, 0.00151330349F,   -0.0016289152F,   0.00301563251F,
      -0.00526613044F,  -0.000887761242F, 0.000291691889F,  -4.43054038E-9F,
      -0.000202296011F, 0.0274815187F,    -0.00033489737F,  -0.00360314664F,
      -0.000760669122F, -0.00447858684F,  0.00490846857F,   1.17842497E-7F,
      0.000789360085F,  -0.00196095277F,  -0.0107796397F,   0.000103956634F,
      0.000684378319F,  7.41394331E-7F,   -0.0095312465F,   -0.00448805839F,
      -0.00425114F,     0.0126924962F,    -1.62210872E-5F,  0.0480245389F,
      -0.0082461955F,   0.000134483038F,  -0.0046130768F,   -2.182421E-8F,
      2.93954869E-8F,   -0.000126077401F, 1.5618518E-8F,    -0.000116301F,
      -3.82244434E-6F,  -0.00121356815F,  -0.0040822369F,   -0.00319490535F,
      5.78212811E-9F,   0.00173757F,      -1.87116342E-7F,  -0.000222378483F,
      -0.00609390344F,  -0.00917885546F,  -0.00236536469F,  0.00834440533F,
      -0.0262542013F,   -0.00098969F,     1.22708315E-7F,   -9.16022054E-6F,
      0.00283460878F,   0.00136922754F,   -6.67482236E-5F,  -0.00102720095F,
      0.00252228742F,   0.00251417491F,   -0.00218814146F,  -0.00106531905F,
      -1.12629657E-7F,  -0.00235252F,     0.0186651163F,    -0.0114069795F,
      0.00225482974F,   -0.0130631272F,   0.00267449208F,   -0.00882707816F,
      0.0027844375F,    -0.000400856836F, 1.01318506E-7F,   -0.002114214F,
      -0.00139911962F,  0.00114663783F,   0.000643382315F,  -4.41142365E-5F,
      -9.83015489E-5F,  0.00224863878F,   -0.000123411854F, -6.31194919E-9F,
      -0.000422728219F, -0.00322679593F,  -1.31332909E-8F,  0.00567663275F,
      -0.00221312395F,  0.00568761723F,   -0.00203763321F,  0.000400571647F,
      -0.00320064882F,  -2.91379578E-8F,  0.00442695F,      0.000386143569F,
      0.0035208622F,    0.0042472505F,    -0.00177382398F,  0.00274239387F,
      -0.00049862155F,  0.00342418742F,   -0.0221089013F,   0.0082291523F,
      0.00148472039F,   0.0166576561F,    -7.5269827E-7F,   -0.00653880788F,
      0.00559934F,      -0.0188894738F,   0.00721999584F,   0.00152011123F,
      0.0178574827F,    -0.00142592494F,  -0.00117893389F,  -4.66284945E-7F,
      0.0119481962F,    -1.83467205E-6F,  0.00244738185F,   0.00130724732F,
      0.00118533592F,   -0.0302761737F,   -0.0188870784F,   -0.019650396F,
      0.00539590884F,   0.00691178394F,   -5.22250389E-7F,  8.89642195E-7F,
      -3.91220465E-6F,  -0.00153959112F,  0.0238657314F,    -0.00395256281F,
      -0.0007640369F,   -1.58133139E-6F,  0.00228769891F,   0.00291279098F,
      -0.0102488194F,   0.00238662609F,   -3.06906986E-6F,  -0.000918425561F,
      -5.89958177E-7F,  -0.0147061525F,   -0.0066286237F,   0.000162609096F,
      -0.0174594168F,   -0.000765981735F, -0.0233410411F,   -1.10703706E-6F,
      -0.000387174601F, -1.98841053E-6F,  0.00189654331F,   0.00406766543F,
      -0.010778564F,    -0.00589557551F,  -5.95648714E-7F,  0.00277713267F,
      0.00190586993F,   -0.0127609372F,   -7.6620563E-7F,   -0.00231054355F,
      0.028886877F,     0.00231182924F,   -0.0353388526F,   0.00265665771F,
      0.000109213186F,  -0.0048406329F,   -0.00173178629F,  0.0070066154F,
      0.00489986921F,   -0.000814416038F, -0.00851712562F,  -0.00171108521F,
      -0.00064990489F,  0.00245850859F,   -0.0263275951F,   -1.97168333E-6F,
      0.000124832266F,  0.00227428926F,   -0.0057020681F,   -0.0015915524F,
      0.00017859225F,   -0.0105841225F,   0.000436500064F,  2.32158754E-7F,
      -0.0333784968F,   -0.0134989386F,   -1.21856363E-6F,  0.00101474894F,
      -8.50521537E-5F,  0.0337658226F,    -3.27612838E-6F,  0.00498623587F,
      0.00606931141F,   0.00710742967F,   -3.60427111E-6F,  0.020630328F,
      -5.05115565E-7F,  7.94306516E-7F,   -0.00295862346F,  0.0276858304F,
      -3.16535761E-7F,  -0.00552974548F,  -0.000211247389F, -2.76040055E-6F,
      0.00726143317F,   1.29656928E-6F,   0.00314676017F,   0.00862956047F,
      -0.064482823F,    -0.0124718854F,   0.00667590741F,   -0.000549116405F,
      -3.82560756E-6F,  -7.76802267E-8F,  -0.000730874F,    -1.85118131E-6F,
      0.0105473548F,    -4.72732463E-5F,  -0.0020951021F,   -2.0044925E-7F,
      -0.0275011472F,   -3.15855391E-6F,  -1.49273019E-5F,  0.00918183569F,
      -5.16179807E-6F,  0.00101483986F,   -5.93112E-6F,     -2.28163481E-6F,
      -0.00179943768F,  -0.0767019838F,   -0.0442872532F,   -7.00690521E-7F,
      -0.00558447931F,  -0.00259981793F,  -7.72112326E-5F,  -4.27074752E-7F,
      -0.000740305055F, -0.0286870673F,   -4.61832224E-6F,  0.00397706823F,
      -0.0305298977F,   0.00114523142F,   -0.018728815F,    -0.0494079404F,
      -0.00608510291F,  -0.0247814376F,   0.00283561274F,   -0.0123666199F,
      -0.0307111572F,   0.00204028795F,   -0.0364569649F,   0.0163181815F,
      -6.9665788E-5F,   -0.00020656237F,  0.000525233278F,  -0.0602220669F,
      0.00982928555F,   0.0274376851F,    -0.0107980296F,   -0.0301275235F,
      0.0059835203F,    -0.00449520722F,  0.00223285635F,   0.000408724853F,
      0.010675814F,     0.0185093656F,    -0.0225028228F,   -0.00375919743F,
      0.00907377433F,   0.000101538455F,  0.00222790218F,   0.000478139729F,
      0.0261639655F,    -0.000396890973F, -0.000239542293F, 0.00828962401F,
      0.00707991747F,   -0.0228310134F,   0.00191840541F,   0.00946501642F,
      -2.14204556E-6F,  0.00149759813F,   -0.0584288687F,   0.00211655186F,
      0.00887051132F,   0.000914575416F,  -2.85673241E-6F,  0.0082296906F,
      -1.15130015E-5F,  -2.47162745E-7F,  0.00305030728F,   0.000209207472F,
      0.00387323066F,   0.0124492068F,    0.0108883595F,    -3.93354094E-6F,
      -7.46874036E-7F,  0.00287808501F,   0.0112228F,       0.0029486462F,
      0.00253978232F,   1.11028476E-6F,   0.00814301707F,   0.00514038419F,
      0.00526136765F,   0.00190720346F,   0.011654052F,     0.000171427295F,
      0.00123961014F,   -0.000382634491F, 5.96299378E-6F,   0.00186724565F,
      -1.13865883E-6F,  -0.0143094212F,   -5.05116361E-7F,  0.029209502F,
      0.000159158328F,  -0.00799754634F,  0.0225393325F,    -1.3814255E-5F,
      -0.00148261234F,  0.000164022305F,  -0.0211295076F,   0.000282442692F,
      0.00563678285F,   -0.0113274865F,   -0.0393197164F,   -0.00602707779F,
      -0.0120565F,      0.00210769405F,   -2.54000361E-6F,  -0.052197F,
      -2.11531892E-6F,  -2.35829384E-6F,  -9.48880597E-7F,  0.00740022073F,
      0.00309078093F,   -0.170576707F,    0.0314200111F,    -2.47685348E-5F,
      -0.00227930839F,  0.00306936447F,   -0.00281946105F,  -0.0174670685F,
      -6.06024423E-6F,  -9.62855964E-8F,  -0.0194234084F,   -6.41803E-5F,
      0.00117547682F,   -4.2232441E-6F,   -3.17518698E-7F,  -0.00215632864F,
      -0.0277623907F,   -0.0407865234F,   0.0180371162F,    0.0255845264F,
      4.49315905E-7F,   -0.00873819459F,  0.012636547F,     0.00532401F,
      0.000289011921F,  3.19024457E-6F,   -0.0197188538F,   -0.00286141317F,
      0.000310356292F,  0.00161192811F,   -0.0186627861F,   -8.27185929E-8F,
      -0.00148549827F,  -7.16345385E-5F,  0.0138723422F,    0.00632571056F,
      -0.0394828841F,   -0.0248154029F,   -0.000527816184F, -0.000325726578F,
      0.00369227421F,   0.0146908788F,    -0.0182656888F,   0.0173086785F,
      -0.00178602221F,  0.00121818262F,   0.00256052148F,   0.0133504057F,
      2.3792395E-6F,    0.00270582759F,   -0.00935768802F,  -0.0130230738F,
      -0.00334555F,     -0.0240282547F,   -0.060494978F,    0.0158396438F,
      -5.06767378E-7F,  0.0287410896F,    -0.000312942691F, 0.00530293817F,
      0.0145811271F,    0.00789379049F,   -0.0507956557F,   -0.00128864287F,
      -0.0136914011F,   -0.0291279405F,   -0.0177293476F,   0.00342211826F,
      -0.00727048097F,  0.00744968047F,   -0.0160508156F,   0.0223422516F,
      -0.0626597553F,   0.000578710926F,  0.00134661805F,   -1.44046203E-6F,
      0.000958336459F,  0.00566138141F,   -0.00304494123F,  -0.0180645566F,
      0.00644441461F,   -0.0270472802F,   0.035438437F,     -7.29942258E-6F,
      0.00304133142F,   -0.0217972808F,   -0.000487577636F, 0.00061727385F,
      -0.000103637227F, -8.40863777E-6F,  -0.0144356377F,   -0.0290445127F,
      -0.00467447937F,  0.0144064194F,    -0.000189764658F, -0.00145990064F,
      0.00218048505F,   0.000388563582F,  -0.0494952723F,   -1.68471593E-6F,
      -2.44011972E-6F,  -0.000174433895F, -5.35678396E-7F,  5.04902491E-5F,
      -9.63221464E-5F,  -0.00194006355F,  -0.0111308172F,   -0.0993055254F,
      -3.382751E-7F,    -0.01014324F,     -6.85734312E-6F,  -0.000137884403F,
      -0.00473564165F,  0.0143382717F,    -0.0023653442F,   0.00906280708F,
      0.00127873337F,   -0.00495501747F,  -9.37291361E-7F,  -0.000122260506F,
      0.006616503F,     0.00644632662F,   -0.00361884641F,  -0.0197182968F,
      0.00425792206F,   0.0229738634F,    -0.00303205755F,  -0.011166391F,
      -3.58679472E-7F,  -0.0234670788F,   0.00144478597F,   -0.0267899726F,
      0.00168539188F,   -0.000340134226F, 0.0161429569F,    -0.0136866653F,
      0.00151631841F,   -0.00183644856F,  6.72343106E-7F,   -0.00261100777F,
      -0.00826564152F,  0.00561778806F,   0.00301395776F,   0.0182586443F,
      -0.000100586149F, 0.0151044168F,    -6.47000124E-5F,  -4.73467907E-7F,
      -0.0169344191F,   -0.00299019576F,  -9.52363393E-7F,  0.00630701194F,
      -0.0336852781F,   0.00294691604F,   -0.0510594286F,   0.00337878801F,
      -0.0215882026F,   -2.39200881E-6F,  0.017506402F,     0.00714396685F,
      0.00517448364F,   0.00413984247F,   -0.00476962375F,  0.0125683146F,
      0.0355196372F,    -0.105236031F,    0.130768538F,     0.0158283338F,
      -0.0801489875F,   0.0193586145F,    -0.0162198469F,   0.0229867678F,
      0.150762647F,     0.131945491F,     0.00719831232F,   0.321542054F,
      0.146401778F,     0.0278891865F,    -0.0486918688F,   -0.00878774561F,
      -0.0371309295F,   -0.0238324758F,   0.0166453123F,    0.329624653F,
      0.0715686679F,    0.00735346647F,   0.069344461F,     -0.245038912F,
      0.173506156F,     0.363819361F,     -0.0122980336F,   -0.00640671561F,
      -0.0109728985F,   0.111124128F,     -0.0909816474F,   0.34997651F,
      -0.0598149337F,   -0.0107478397F,   -0.0371432416F,   0.028770918F,
      0.00511312764F,   0.246506765F,     -0.00850923453F,  -0.0412373506F,
      -0.00848846417F,  0.00730560115F,   -0.0035670714F,   -0.0774016678F,
      -0.0447428338F,   0.01413312F,      0.387318254F,     -0.0109055014F,
      0.202123225F,     -0.0139086181F,   0.370916396F,     -0.0788712278F,
      -0.158533F,       -0.0114039294F,   -0.0151706832F,   0.184208542F,
      -0.0587516613F,   -0.123526104F,    -0.0106615415F,   -0.0431046784F,
      0.081332542F,     -0.0163848028F,   -0.211353183F,    -0.0239579957F,
      -0.144411802F,    -0.0939321145F,   -0.0675712153F,   0.0703285933F,
      0.00991960149F,   0.0165154282F,    -0.0415911861F,   0.258305907F,
      -0.128306091F,    0.325651228F,     0.0312940627F,    -0.013888794F,
      -0.040525496F,    0.35344258F,      0.0526985452F,    0.0852643698F,
      0.215803102F,     0.0878597498F,    0.0165996775F,    -0.0111866361F,
      -0.239646703F,    -0.102600686F,    -0.00739998044F,  0.0100243706F,
      -0.0273469668F,   0.0672514066F,    -0.00696615735F,  0.0496689603F,
      0.176223814F,     0.0123349223F,    -0.0137108006F,   -0.0883028209F,
      -0.0110501423F,   -0.011187383F,    0.00975186285F,   0.0447334349F,
      -0.011898661F,    -0.000782342628F, -0.0199728422F,   -0.0120856632F,
      0.310884297F,     -0.0115809338F,   0.274894327F,     0.266186118F,
      0.0288596749F,    0.0751946568F,    0.0619245544F,    -0.127843678F,
      -0.00629307283F,  -0.00976330321F,  0.0372420289F,    -0.0111009385F,
      0.0458607674F,    0.127866238F,     0.106251866F,     -0.00971355196F,
      0.281996936F,     -0.0135343643F,   -0.00582504412F,  0.0162998457F,
      -0.00594946695F,  0.313385487F,     -0.0159865785F,   -0.00947586913F,
      -0.038141638F,    0.0607522875F,    -0.0460092947F,   -0.0120456694F,
      -0.162455827F,    -0.019685166F,    -0.107682623F,    -0.0101672923F,
      0.0466666967F,    0.0193064194F,    -0.00948599819F,  -0.0274676867F,
      -0.00206597405F,  -0.100824006F,    0.156825155F,     0.0563924611F,
      -0.0319710337F,   0.0239779931F,    0.028690258F,     0.154270604F,
      0.201320276F,     0.0148924282F,    0.273953438F,     0.0531230345F,
      -0.00443153922F,  -0.00839650445F,  0.349740207F,     0.039234221F,
      0.00449422654F,   0.0778236389F,    0.0126388548F,    -0.0435817055F,
      0.126472682F,     0.0354706347F,    0.295271605F,     0.278998345F,
      0.359608263F,     0.0114059178F,    -0.0262725018F,   0.0567691252F,
      -0.0640907139F,   -0.122212924F,    0.264315426F,     -0.0423618406F,
      -0.0724593326F,   -0.0492237546F,   -0.044870045F,    -0.0807792321F,
      0.0415226072F,    0.00434301374F,   -0.106497809F,    0.0544237606F,
      -0.0121934637F,   -0.0088433763F,   -0.021870397F,    -0.097159557F,
      0.00601412822F,   -0.00301774917F,  -0.00817171484F,  0.0357550234F,
      -0.0164362304F,   -0.0102720428F,   0.48581925F,      0.287577122F,
      -0.0186531562F,   0.0200816486F,    0.0462228656F,    -0.0109748794F,
      -0.00788366236F,  0.20747304F,      0.0819865838F,    0.302220404F,
      0.119317457F,     -0.0141703961F,   0.0215297F,       -0.109346449F,
      -0.0240126401F,   -0.05017519F,     -0.0184994489F,   0.351666808F,
      0.0562911257F,    0.0119262254F,    -0.0046954914F,   -0.0486658178F,
      -0.00803483743F,  0.326392531F,     -0.0109088318F,   0.189662531F,
      -0.0470493734F,   -0.092600584F,    0.0202711597F,    -0.0298276078F,
      0.137857258F,     -0.0820500627F,   -0.0582197197F,   0.333747327F,
      0.574080765F,     -0.0537307672F,   -0.0176810138F,   -0.15567711F,
      -0.0332943387F,   0.399830937F,     -0.0126215117F,   -0.085049063F,
      -0.0101530598F,   -0.007808025F,    -0.00969227124F,  -0.146219835F,
      0.0321707614F,    0.519554496F,     0.00664971909F,   -0.0179487374F,
      0.304842561F,     0.22076419F,      0.0738331F,       0.10394454F,
      -0.0312720835F,   -0.00593242887F,  0.0171948448F,    -0.10129679F,
      -0.0867249295F,   -0.00474996958F,  -0.00784823764F,  -0.0404542461F,
      0.0984362289F,    0.133515611F,     0.0058681271F,    0.188278049F,
      -0.0116934348F,   -0.0188207533F,   0.121214986F,     0.487182289F,
      -0.101406246F,    -0.0158787686F,   -0.0177595913F,   -0.00805750303F,
      0.0171419121F,    0.507690907F,     -0.0108920299F,   -0.0107373288F,
      0.0873120576F,    -0.0116220675F,   0.0254887696F,    0.26895231F,
      -0.0325808525F,   0.0590199046F,    -0.0452596806F,   0.334283978F,
      0.038412828F,     0.0208048541F,    -0.0360109F,      0.0580320321F,
      0.225853622F,     0.354148805F,     -0.0981004462F,   0.35819751F,
      -0.0144930175F,   0.0244365856F,    -0.0255604722F,   -0.023845505F,
      0.0622798093F,    0.0276979F,       -0.20872651F,     -0.101234883F,
      -0.0102179339F,   0.11291039F,      -0.0116473008F,   -0.116601773F,
      0.227430463F,     0.0148043651F,    -0.00845882669F,  -0.0439473391F,
      0.267290831F,     -0.0679156706F,   0.00925898273F,   -0.01761535F,
      0.0350473188F,    -0.0992689282F,   -0.0185977425F,   0.0227319263F,
      -0.0446079411F,   0.13947548F,      -0.00723818596F,  -0.00907751732F,
      -0.176155F,       0.334222645F,     0.381502092F,     0.0642863959F,
      0.364035279F,     -0.0564174205F,   0.0505310297F,    -0.00737790531F,
      -0.208667219F,    -0.04134712F,     0.227902874F,     0.00560659682F,
      0.063131F,        -0.0149909463F,   0.237210229F,     0.0014102452F,
      0.0136424853F,    0.534153223F,     -0.0732442588F,   0.180973187F,
      0.209735051F,     -0.138528571F,    -0.0518419147F,   -0.0069886609F,
      -0.00908505265F,  -0.0352833979F,   -0.0109050581F,   -0.0302260332F,
      -0.0771288425F,   0.162969738F,     0.0458317F,       0.0726451054F,
      -0.0086200051F,   0.0556345135F,    -0.0150500145F,   -0.0542178713F,
      0.103351563F,     0.164231107F,     -0.0174924526F,   0.472616881F,
      0.254840583F,     -0.0566197038F,   -0.00908986107F,  -0.0715698823F,
      0.0537594408F,    -0.0417006724F,   0.329173326F,     0.023277238F,
      0.0233612861F,    0.0380407162F,    0.0189680625F,    -0.037350148F,
      -0.0131987184F,   -0.108681619F,    -0.0148316249F,   0.107198909F,
      0.201678202F,     0.178920016F,     0.0501481444F,    0.494716942F,
      0.29065147F,      -0.260701746F,    -0.0103196073F,   0.0222670343F,
      -0.0482738912F,   0.345751137F,     -0.093511F,       0.0189921763F,
      -0.0462220348F,   -0.0337793231F,   -0.0304254405F,   -0.0106485737F,
      0.0248414073F,    -0.0211379F,      -0.010057454F,    0.515987158F,
      0.0221415274F,    -0.0217030533F,   0.0301805735F,    -0.018092785F,
      0.0171261933F,    -0.00760844024F,  0.0512500145F,    0.00653039571F,
      0.336623311F,     0.0393665582F,    -0.0225015208F,   0.0842088833F,
      0.0393267721F,    0.0127009107F,    0.0382251143F,    -0.00327687408F,
      0.0442339294F,    0.0246758591F,    -8.51180885E-5F,  -0.0389311872F,
      -0.011660859F,    -0.111680821F,    0.012487269F,     -0.0972467437F,
      0.024430804F,     0.00346333324F,   -0.0317358635F,   -3.62515698E-6F,
      0.0208280552F,    -8.3477571E-5F,   0.0233500376F,    -0.0829585046F,
      0.0420091935F,    0.0115558365F,    0.0141567299F,    0.0492885336F,
      -0.063676253F,    0.0488226637F,    -1.13169597E-6F,  1.01147566E-6F,
      -4.88194291E-6F,  -0.0502162948F,   0.0363168865F,    0.0138081582F,
      -0.00572481612F,  1.01375254E-5F,   -0.00615949696F,  0.077908963F,
      -0.00142339582F,  -0.0128944F,      -7.48630237E-6F,  -0.0101990532F,
      6.34645346E-7F,   0.0711088106F,    -0.00158315268F,  -0.000630016555F,
      -0.0192764644F,   0.00865238719F,   -0.1326098F,      5.34513583E-6F,
      -0.107301749F,    -2.02189676E-5F,  -0.0992416665F,   -0.133900598F,
      -0.00812763441F,  -0.0748748556F,   -3.13264627E-6F,  -0.0822678283F,
      0.0289718211F,    -0.0381479301F,   -6.01403E-6F,     -0.0194073096F,
      0.0503394566F,    0.0690251663F,    0.0474999845F,    0.0343681127F,
      -0.000762868032F, 0.014484833F,     -0.0168821849F,   -0.000304424175F,
      0.0192463025F,    -0.0346781574F,   0.0285088215F,    -0.0342063867F,
      0.00148813589F,   0.0153450733F,    -0.0257679094F,   -3.37647543E-5F,
      -0.052760873F,    -0.0655173287F,   0.00190548552F,   0.0312831216F,
      0.0362905823F,    0.0372265205F,    0.0323485769F,    -5.00095302E-6F,
      0.0650895312F,    -0.0200305153F,   -1.90908031E-6F,  -0.000155695394F,
      -0.000142281438F, -0.0413920842F,   -7.72298063E-6F,  0.0301244482F,
      0.0618082732F,    0.0232627746F,    -3.79150606E-7F,  0.0138101792F,
      7.00595274E-6F,   4.40936901E-7F,   -0.0302647837F,   0.00745268911F,
      -1.28873007E-5F,  -0.0291539263F,   -0.000409617263F, 1.79324743E-5F,
      0.0119187701F,    -2.24315909E-6F,  0.00675563887F,   0.0469549075F,
      0.00150286674F,   0.0385507941F,    0.0224482175F,    0.000919540878F,
      -3.2596929E-6F,   3.96130474E-7F,   -0.0360198617F,   1.78605069E-6F,
      0.03248908F,      0.0393630974F,    0.0122479051F,    -3.03093941E-7F,
      0.0523158163F,    -1.75019795E-5F,  -3.29563118E-5F,  0.023617113F,
      1.11407135E-5F,   -0.0606665947F,   -1.35057235E-5F,  1.17530099E-5F,
      0.0617362894F,    -0.00376812206F,  0.0110319695F,    7.19600621E-7F,
      -0.0366450585F,   -0.0455882773F,   0.000150874752F,  9.45494662E-7F,
      -0.0252160821F,   -0.00836506393F,  -5.450343E-6F,    0.0364062674F,
      -0.00754536642F,  0.00192676135F,   -0.00257360307F,  -0.00793578569F,
      -0.0728490874F,   0.0028466F,       0.00994747691F,   0.0483235642F,
      0.0732769221F,    0.108439907F,     0.00845163F,      0.00608229823F,
      -0.000209101083F, -0.01505295F,     -0.0723847374F,   0.0146919861F,
      0.0273590218F,    -0.0703855157F,   0.0269363094F,    0.0184900891F,
      0.0563511178F,    -0.0330792181F,   0.0580237173F,    0.0121597825F,
      -0.0207732432F,   0.0218158F,       -0.0174894612F,   -0.0690499246F,
      0.0143533824F,    0.000123491816F,  -0.102484509F,    -0.0373388156F,
      0.0363637656F,    -0.0543071888F,   -0.0138921356F,   0.0283213463F,
      0.0172978435F,    0.0299702808F,    0.0169952437F,    0.0238126479F,
      2.47004436E-6F,   0.0558107607F,    0.0205350965F,    0.0168161448F,
      0.0396440737F,    -0.00204669707F,  1.49045591E-5F,   0.0163872372F,
      -0.000128788801F, 1.88938293E-6F,   -0.113502949F,    -0.0920693353F,
      -0.0303460378F,   0.00830987375F,   0.0167273358F,    -7.57777934E-6F,
      -5.93989796E-7F,  -0.0848825052F,   -0.0338381343F,   -0.0919229F,
      0.00167078443F,   1.11312893E-5F,   0.0162088964F,    0.0111158639F,
      -0.00239998451F,  -0.042687241F,    0.0362999F,       -0.0238386802F,
      0.0306184683F,    -0.0187961645F,   -5.02411167E-5F,  0.00423450768F,
      -1.56912347E-6F,  -0.111893259F,    7.33544709E-7F,   0.0156556424F,
      0.000266393268F,  -0.0156363491F,   0.0353974923F,    -0.000196062683F,
      0.0166616924F,    0.000317789265F,  -0.018879259F,    0.0915870667F,
      0.140879795F,     -0.0212974269F,   0.00912077F,      0.0139536988F,
      -0.0393440872F,   -0.0924292803F,   -2.46843738E-5F,  0.0019715724F,
      -2.08127549E-5F,  4.6780433E-6F,    7.2525354E-6F,    0.0455881469F,
      0.0413649753F,    0.0873516724F,    0.0372027494F,    -0.000194933134F,
      0.0616168901F,    0.0182780679F,    -0.0853933543F,   0.0370834433F,
      -0.000331211748F, -4.94271308E-6F,  0.00541138742F,   -1.65720448E-5F,
      0.00974076707F,   -8.8921679E-6F,   -1.25609915E-6F,  -0.0349983945F,
      -0.0953520313F,   -0.108500294F,    0.0329852253F,    -0.0861318335F,
      -5.87008071E-6F,  -0.0139970481F,   -0.0469821543F,   0.0134744439F,
      -0.0335687622F,   -6.73670074E-5F,  -0.0745090619F,   0.00148564461F,
      0.0350506864F,    -0.0456395783F,   -0.000611435389F, 1.55989966E-7F,
      0.0142414086F,    0.000946644519F,  0.0390971F,       0.0199884027F,
      -0.0567934811F,   -0.00206703506F,  0.000663588406F,  0.0966996923F,
      0.0263129622F,    0.025608398F,     0.00198188215F,   0.0126949847F,
      0.0253402535F,    -0.0680923685F,   0.0299951918F,    0.0786487758F,
      -6.39575301E-5F,  0.0270563196F,    -0.000776573783F, -0.0131747201F,
      0.0316995569F,    0.029522026F,     -0.0317067169F,   -0.042761188F,
      -4.3218879E-6F,   -0.0575316325F,   -0.00192711584F,  0.0718884468F,
      -0.0377058126F,   0.025069328F,     0.0337193534F,    0.0402696282F,
      0.0944622606F,    -0.0755066052F,   0.0672639236F,    0.048689656F,
      0.00332288514F,   0.0102815907F,    0.00179433834F,   -0.0010189343F,
      0.0306273419F,    0.0955508724F,    -0.00129473768F,  -1.31874947E-6F,
      -0.000343085499F, 0.0417869836F,    0.0911609828F,    0.0366178229F,
      0.0295622759F,    0.0247359294F,    0.0229328778F,    3.68844076E-6F,
      0.00453333091F,   0.0154776787F,    0.0572967567F,    -0.00100577832F,
      -0.0056531704F,   -8.38748674E-5F,  0.0977417156F,    0.0283092037F,
      -0.0268467832F,   0.0580425635F,    -0.0134227043F,   -0.0800739452F,
      -0.00672298484F,  -0.000669560803F, 0.00403705798F,   -1.32518949E-6F,
      -2.94311394E-6F,  -0.00125411653F,  6.37849E-6F,      -0.00127972336F,
      -0.000339500839F, 0.0785532519F,    -0.0609912612F,   0.00698083313F,
      6.81936399E-7F,   -0.0224280711F,   -2.56306266E-5F,  -0.00457266485F,
      -0.00435086805F,  -0.127241597F,    -0.0720313F,      0.126827329F,
      0.0282221884F,    -0.0308412407F,   5.67992811E-6F,   0.00404280704F,
      0.0201437529F,    0.0111689325F,    0.0327696465F,    0.000315442157F,
      -0.0110009424F,   0.0603188723F,    0.00210731803F,   -0.00633293483F,
      -1.54704212E-5F,  0.0272442736F,    0.0294074379F,    0.020749066F,
      -0.0569557846F,   0.0225912891F,    0.0177159403F,    0.113926046F,
      -0.0704316124F,   0.0175456759F,    1.86166926E-6F,   0.030902639F,
      -0.0256798156F,   0.0148506761F,    0.0110692121F,    0.0614949726F,
      -0.00155512872F,  0.013923388F,     -0.000779803726F, -6.07528364E-7F,
      0.015467315F,     -0.0527862683F,   4.13995394E-7F,   0.0954323933F,
      -0.00930727832F,  0.0255788043F,    0.0028025012F,    0.0402174927F,
      0.0212652348F,    -9.60680791E-6F,  -0.0613399483F,   0.0462433621F,
      0.0769059807F,    0.0092377821F,    0.010279228F,     0.0230171494F,
      0.0412039645F,    0.0182925966F,    0.0186602753F,    -0.119829752F,
      -0.0539140776F,   0.0319909677F,    -0.00906073F,     0.186949596F,
      0.158594698F,     -0.0122816851F,   0.0101657761F,    0.315322518F,
      0.137645125F,     -0.0951498225F,   -0.00145770842F,  -0.00888529792F,
      0.0328222625F,    -0.0131890206F,   0.0264274441F,    0.335585624F,
      0.202641919F,     -0.0303368215F,   0.0640655085F,    -0.24274452F,
      0.204366624F,     0.384553969F,     -0.0124563202F,   -0.00642296346F,
      -0.0110805342F,   0.214011624F,     0.0973638F,       0.341093808F,
      -0.0729095414F,   -0.0108261984F,   -0.0545849614F,   0.0192827042F,
      -0.0388080887F,   0.329274356F,     -0.00840248819F,  -0.0695719197F,
      -0.00858343579F,  -0.0288852602F,   -0.0255229529F,   -0.0660148337F,
      -0.0108325947F,   0.0216146447F,    -0.0116710188F,   -0.0104550952F,
      0.18919006F,      -0.0143356947F,   0.368541449F,     0.181789339F,
      -0.273627758F,    0.0601868033F,    -0.0132062603F,   0.25604248F,
      0.0144661898F,    -0.0849503726F,   -0.0105184838F,   0.0246002916F,
      0.0781324357F,    0.166775703F,     -0.239406243F,    0.0604282543F,
      -0.0867274627F,   -0.0509265848F,   -0.013344083F,    0.0313131623F,
      0.0103532812F,    -0.0444698744F,   0.120907396F,     0.263795197F,
      -0.0781859383F,   0.382206947F,     0.0388858728F,    -0.014005716F,
      0.0426975302F,    0.316586673F,     -0.092444025F,    0.053812176F,
      0.258701533F,     0.187653482F,     0.0414671898F,    -0.0120552052F,
      -0.240932196F,    -0.120992161F,    -0.00742669497F,  -0.0433888026F,
      -0.0289090984F,   0.0449691787F,    -0.00698686F,     0.0503514968F,
      -0.0100662773F,   0.0177860614F,    -0.0140738804F,   -0.0954674482F,
      -0.0111828856F,   -0.011455493F,    -0.0117697986F,   0.058435984F,
      -0.0121825598F,   -0.0050416193F,   -0.0392644927F,   -0.0118716536F,
      0.359555215F,     -0.0115325721F,   0.296898156F,     0.310452819F,
      -0.0352592F,      0.0337167419F,    0.0515084155F,    -0.105251051F,
      -0.00628856849F,  -0.0097668F,      0.0443596765F,    -0.0114160711F,
      -0.0163578819F,   0.143684104F,     0.105816968F,     -0.00985329133F,
      0.108795211F,     -0.0134528019F,   -0.00578939682F,  0.0243859775F,
      -0.00586582581F,  0.432824612F,     -0.015929943F,    -0.00951803476F,
      0.094000563F,     -0.0773163438F,   -0.0560141653F,   -0.0120340213F,
      -0.0454934388F,   0.0180878397F,    0.00193328946F,   -0.010179677F,
      0.0620021522F,    -0.121810161F,    -0.0093586212F,   0.103854142F,
      -0.0374100134F,   -0.0621427931F,   0.149352103F,     -0.178699732F,
      0.0178244077F,    -0.0415536799F,   -0.134511739F,    0.0769243762F,
      0.0519534387F,    -0.0272862539F,   0.092510514F,     -0.112884134F,
      -0.00442013331F,  -0.0530806929F,   0.357941896F,     -0.0857330114F,
      0.00944542326F,   0.0732896775F,    -0.0664107725F,   -0.0305313841F,
      0.260636091F,     0.00414988399F,   0.302826673F,     0.21473442F,
      0.24098213F,      0.0230130516F,    -0.0652549118F,   0.068672888F,
      -0.099699907F,    -0.0949405506F,   0.299691975F,     0.0824497119F,
      -0.0990692452F,   0.0268057883F,    -0.0204371437F,   0.0780926123F,
      0.011603822F,     -0.0282350462F,   0.0107106818F,    0.0381446034F,
      -0.0125394203F,   0.103884794F,     -0.0800153762F,   0.038952589F,
      0.0100588007F,    -0.105563119F,    -0.00807178207F,  0.0226099547F,
      -0.0177315604F,   -0.0104597881F,   0.486770511F,     0.293470323F,
      0.0815890655F,    -0.150706619F,    0.0112300189F,    -0.0109149162F,
      -0.00782025885F,  0.0648948848F,    0.107104011F,     -0.0125561031F,
      -0.0780585855F,   -0.0141419694F,   0.0172355846F,    -0.0593613386F,
      -0.104262605F,    0.0409941971F,    0.071112819F,     0.36617884F,
      0.0637643337F,    0.0132427616F,    -0.00468376419F,  -0.0144954538F,
      -0.00795692299F,  -0.00380031276F,  -0.0109547926F,   -0.00474993652F,
      -0.0793086812F,   -0.110384963F,    -0.0480620228F,   -0.0163693987F,
      0.153704628F,     -0.0937696F,      -0.0139620034F,   0.416752875F,
      0.722611785F,     0.0342164226F,    -0.0773636103F,   0.00889589265F,
      0.0144887511F,    0.418624789F,     -0.0119753601F,   -0.127686426F,
      -0.0101589775F,   -0.00776752317F,  -0.00961431675F,  -0.0736380294F,
      0.0288770553F,    -0.0132888285F,   0.00885107275F,   -0.0268159211F,
      0.263293415F,     0.250152856F,     0.0830385238F,    0.0972204432F,
      -0.0365683734F,   -0.00587386684F,  -0.0246803537F,   -0.0586887263F,
      -0.095669128F,    -0.00480661402F,  -0.00789836608F,  0.000704655133F,
      -0.0382250845F,   -0.0166534483F,   0.00939095393F,   0.0903625265F,
      -0.0115730297F,   -0.0452841967F,   0.0827765465F,    0.523999631F,
      0.00434648432F,   -0.0166718457F,   0.0339753553F,    -0.118243098F,
      0.0404559113F,    0.492325574F,     -0.120889172F,    -0.0107010789F,
      0.265992761F,     -0.04542505F,     0.0109028993F,    0.301596105F,
      -0.00443721842F,  -0.122450545F,    -0.139987811F,    0.357207626F,
      0.0510883182F,    0.0579275973F,    -0.0412288979F,   0.0606975108F,
      0.150135338F,     0.381698608F,     0.0367107652F,    0.469349772F,
      -0.0141403805F,   0.032794863F,     -0.0460635312F,   0.147044F,
      0.0959422812F,    0.0312647894F,    -0.249248147F,    0.0628826693F,
      -0.0103476131F,   0.0742849708F,    -0.0693471283F,   0.130184576F,
      0.078622587F,     0.0216433145F,    -0.0665056556F,   0.0207510907F,
      0.23025541F,      -0.148327023F,    0.129426718F,     0.191337675F,
      -0.0471329354F,   -0.0106358146F,   -0.102448806F,    -0.00317291194F,
      -0.034465231F,    0.151228547F,     -0.103946887F,    -0.00899890717F,
      -0.10113623F,     0.397739977F,     0.512560606F,     0.0914835632F,
      0.412108809F,     0.030700827F,     0.0488445386F,    -0.00737302564F,
      -0.0231864098F,   -0.0766957477F,   0.251425683F,     -0.109545179F,
      -0.0495891571F,   -0.0155712133F,   0.230269864F,     0.00462165475F,
      0.0331156775F,    0.477865875F,     0.00762163289F,   -0.00968254823F,
      0.225438982F,     -0.0872306675F,   -0.0792241246F,   -0.00698296959F,
      -0.00931393821F,  -0.022192359F,    -0.0108787837F,   -0.0438768044F,
      -0.0567868277F,   0.188465178F,     -0.0201599803F,   -0.0846565291F,
      -0.00859695673F,  0.0591766611F,    -0.0149433454F,   -0.0460478254F,
      -0.0176172946F,   0.0572195798F,    0.0869732648F,    0.567443848F,
      0.256777555F,     -0.0154463146F,   -0.00906626508F,  -0.13413614F,
      -0.0500208139F,   0.0172350649F,    0.361697912F,     -0.208958402F,
      0.0107144266F,    -0.000959377328F, 0.0506614521F,    -0.168409213F,
      -0.013234132F,    -0.112398073F,    0.249949574F,     0.118653804F,
      0.228273883F,     0.212504819F,     0.00129642629F,   0.429377198F,
      0.299322754F,     -0.155806348F,    -0.0102901962F,   0.00585562503F,
      -0.0518908948F,   0.375540584F,     -0.0564836189F,   -0.000590350537F,
      -0.0478869155F,   -0.0771334916F,   -0.0358733349F,   -0.0108895367F,
      -0.0343225114F,   0.0303632952F,    -0.0100556063F,   0.560964525F,
      -0.107427053F,    0.184388548F,     -0.114607304F,    0.0177188963F,
      0.0245260987F,    -0.00763982674F,  0.0385509F,       0.0181276146F,
      0.364186198F,     0.145636708F,     0.0318635814F,    0.0649603307F,
      0.0015670557F,    -0.000491900486F, 0.00206480874F,   0.000527956F,
      -0.00347332307F,  0.00619664323F,   1.9346694E-6F,    0.00255062F,
      0.000390214234F,  0.00783109572F,   0.00323239225F,   -5.43692186E-5F,
      -0.000656523F,    -0.00208791671F,  -0.00192542362F,  -1.03025641E-7F,
      -0.00253225863F,  1.99342903E-5F,   0.00193865818F,   0.00319271465F,
      0.00829333533F,   -0.000426116982F, 0.000250386161F,  0.00268674805F,
      -0.00103025348F,  -0.0013166992F,   9.17012244E-7F,   -4.01703403E-7F,
      -4.44056786E-6F,  0.0106699048F,    -0.0104405787F,   0.001032606F,
      0.000906868896F,  -1.16863214E-6F,  -0.000251858524F, 0.00860883202F,
      -0.00156241388F,  0.0206448343F,    -2.17185197E-6F,  0.000759243849F,
      -1.10375584E-6F,  -0.00739497598F,  -0.00262292963F,  0.00202870881F,
      -0.00407969579F,  -0.000712825393F, 0.00688602F,      -1.27217845E-6F,
      -0.00171502808F,  -9.28759709E-6F,  0.00220331643F,   0.0242758244F,
      0.00283253193F,   -0.011922339F,    8.99723E-7F,      -0.00418044953F,
      0.0088965809F,    0.00311947474F,   1.43592797E-6F,   -0.000699428841F,
      0.000352429808F,  0.0153107122F,    0.0171983223F,    -0.00022590172F,
      0.00332366396F,   -0.00139835465F,  -0.00135878357F,  -0.0058655045F,
      0.00153498352F,   0.00022296996F,   0.00163640594F,   0.00210652663F,
      0.00116655615F,   -0.000207456862F, 0.00208966737F,   -1.09736302E-5F,
      -0.000622693391F, -0.000455466215F, 0.000771205639F,  0.000215564942F,
      0.00103966345F,   -0.00934647303F,  0.00532963919F,   -8.73920726E-7F,
      0.013539874F,     0.00231527397F,   -2.74357035E-8F,  -0.00139320735F,
      -7.67174643E-5F,  -0.00445354F,     -1.5142507E-6F,   -0.00146758975F,
      0.00128289452F,   0.00274310494F,   -1.08340755E-5F,  -0.000678791082F,
      -6.44986926E-7F,  -1.83665293E-6F,  -0.0025627336F,   0.000476054091F,
      -2.73222668E-6F,  0.00463892147F,   5.88777766E-5F,   -2.06726327E-6F,
      -0.000910086907F, -7.84345843E-7F,  -0.000447331899F, -0.00153241761F,
      0.00187445886F,   0.00142471655F,   0.00524844835F,   0.00354207354F,
      -3.5437879E-6F,   -2.14720353E-6F,  -0.00145132013F,  1.89947968E-6F,
      0.0013130029F,    0.000133109235F,  -0.000288251787F, -2.30162527E-6F,
      0.000337032659F,  -1.33356107E-5F,  -8.30090539E-6F,  0.0031926916F,
      -4.11225483E-6F,  0.00190020027F,   -1.55605248E-5F,  -1.28360216E-6F,
      0.0217927694F,    0.0028241598F,    0.0032376044F,    -2.58969862E-6F,
      -0.000367349508F, -0.00055938639F,  -0.00222117547F,  -5.0915321E-7F,
      -0.00183895533F,  0.00127982441F,   -1.44018522E-5F,  -0.00295297126F,
      -0.000171268475F, 0.000571919896F,  -0.00275362283F,  0.00322666671F,
      -0.00114304188F,  -0.000404826133F, 0.000598113053F,  0.00349349761F,
      -0.000307178823F, 0.0140298037F,    -0.00408656429F,  -0.000168744242F,
      -5.93309596E-5F,  0.000113367343F,  0.00374631304F,   0.000776573841F,
      0.00100207143F,   -0.000798427092F, 0.00116132142F,   -0.00437482866F,
      0.0091088526F,    -0.000870188465F, -0.000825489115F, -0.00173787668F,
      0.000849407457F,  -4.92215077E-5F,  -1.51057739E-5F,  -0.000467973208F,
      -0.00120734703F,  0.00303753186F,   -0.00041381194F,  -0.00193975435F,
      -0.00277709914F,  -0.000837375934F, 0.000192182692F,  -0.00425429316F,
      0.00077369978F,   -0.00263329013F,  -0.00185405195F,  0.00508258585F,
      -2.28726458E-6F,  0.00553905871F,   0.00352324476F,   -0.0010207434F,
      0.000236326858F,  0.000365518237F,  -1.54447332E-6F,  0.00187986461F,
      -9.91202796E-7F,  -1.12727741E-6F,  -0.000814620347F, -0.00145703775F,
      -0.00699948752F,  -0.000268953823F, 0.000201573494F,  -1.06569214E-5F,
      1.42253924E-8F,   0.00191731018F,   -0.00185988471F,  0.000652808405F,
      0.00300813466F,   -2.00183422E-6F,  0.00184998906F,   -0.000838216394F,
      0.000270661432F,  -0.00498941308F,  -0.00625763368F,  0.000639285601F,
      0.00237735128F,   -0.000435831171F, -2.00479299E-5F,  -0.00154126168F,
      -1.0898998E-6F,   -4.83610056E-5F,  1.34584695E-6F,   -0.002057045F,
      0.00134783378F,   0.00123783771F,   -0.000562787813F, 4.22450576E-5F,
      0.00131243747F,   0.00209962926F,   3.77575598E-5F,   -0.00110685395F,
      0.000831259F,     1.70175663E-5F,   0.00102327124F,   0.00305416854F,
      -0.00139499246F,  0.000494928914F,  -2.47853222E-6F,  0.00510413386F,
      -1.25290444E-5F,  -1.48262779E-6F,  -1.21583969E-6F,  -0.00360961421F,
      0.00076153077F,   0.00130498223F,   0.00258717616F,   7.75496082E-5F,
      -0.000910931325F, 0.000408267282F,  -0.000931080605F, 0.00136167218F,
      -4.28643434E-5F,  -1.65444945E-7F,  -0.00244306168F,  0.00155449973F,
      0.000260489061F,  -1.16736078E-6F,  -6.98052872E-7F,  -0.00167858147F,
      0.00316105061F,   0.00650888495F,   0.00141938752F,   0.00151573657F,
      -2.95065774E-6F,  -0.000892627519F, 0.000579258252F,  0.00196357071F,
      0.000866675749F,  -1.03183766E-5F,  -0.00659571355F,  0.000151249507F,
      0.0038042895F,    0.00493836775F,   0.00152063568F,   -3.14269244E-7F,
      0.013915861F,     0.000470196712F,  0.00661856635F,   -0.00153705128F,
      -0.00026284621F,  0.00038843503F,   0.00155080785F,   0.00123198749F,
      0.0040589734F,    0.00370213878F,   -0.00341949868F,  -0.00194202177F,
      -0.000295586622F, 0.00243868842F,   -0.00608910527F,  0.000692643167F,
      -6.61508375E-6F,  0.00299357437F,   -0.00111194572F,  0.00855217688F,
      0.000201953037F,  0.00115221122F,   0.012706521F,     -0.00934774149F,
      -2.09119798E-6F,  -0.00102580374F,  0.000423086807F,  -0.00659403531F,
      0.00103307422F,   0.00282283057F,   0.00215433748F,   -0.00189935649F,
      -0.00215430814F,  0.00549162645F,   0.0325922966F,    -0.00123563607F,
      0.000295192178F,  -0.00128712982F,  0.00100867206F,   0.00480262842F,
      0.00231952802F,   -0.00369357248F,  0.000288295094F,  2.90356525E-6F,
      0.000609542069F,  0.000152774228F,  -0.000864418806F, 0.0035854429F,
      -0.00149346772F,  0.00608528173F,   -0.000850876619F, 2.7575777E-7F,
      -0.00174636277F,  0.000954365707F,  0.000901147083F,  -0.000190361592F,
      -0.000658345758F, -3.5046E-5F,      0.000728622952F,  0.00328348111F,
      -0.00312546897F,  -0.000978450873F, -0.000314399949F, 0.00515536591F,
      0.00208983966F,   0.0040256842F,    0.00575056672F,   -2.40179111E-6F,
      -1.40220573E-6F,  9.43601335E-5F,   -2.80956442E-6F,  0.000630011898F,
      0.000783043331F,  -0.000378269469F, -0.000236965454F, 0.00284280465F,
      -2.38215094E-7F,  0.00241825287F,   -2.30052428E-5F,  0.000100378646F,
      -0.00189141382F,  -0.00931069721F,  0.0170625784F,    0.000155758928F,
      -0.000240989117F, -0.00238868664F,  3.96654627E-7F,   0.00194671075F,
      -0.00125923858F,  -0.00135845109F,  -0.000307055627F, 0.00207355595F,
      0.00141085021F,   0.0032933068F,    -0.000846064708F, 0.0011934978F,
      -9.97421557E-6F,  0.00369740766F,   -0.00415496901F,  0.000138599455F,
      0.000944148283F,  0.00121342728F,   0.000302554312F,  -7.85055527E-5F,
      -9.51910079E-6F,  -0.000919593032F, 2.28257164E-7F,   0.00247629802F,
      -0.00374120893F,  -0.00208349014F,  -0.000224950156F, 0.0054850569F,
      -0.000105456827F, -0.00283438689F,  6.58484496E-5F,   2.8071068E-7F,
      -0.000754812558F, -0.00498598907F,  -9.49574314E-7F,  0.00123998674F,
      0.00188997656F,   -0.0119424239F,   0.0016618626F,    1.8566021E-5F,
      -0.000227985889F, -3.16274986E-6F,  -0.0037369905F,   0.00354333455F,
      -4.04351886E-5F,  0.0010192832F,    -0.00127255416F,  -0.000487081765F,
      0.00055255997F,   2.20040019E-5F,   0.00128958316F,   -5.93525074E-5F,
      -0.000235151965F, 0.000314539822F,  4.38174425E-7F,   -0.000896605954F,
      -0.000733406574F, -0.00568159856F,  0.00133299618F,   0.00191073027F,
      6.00173735E-5F,   0.000394104864F,  -0.00107753905F,  2.2504512E-8F,
      0.000299734442F,  5.43168824E-7F,   0.000170020823F,  -0.000309802155F,
      0.00101892138F,   -0.000198004025F, -0.00307136914F,  0.00221555983F,
      -0.000933651929F, -0.00156553369F,  3.65028718E-8F,   -2.26879582E-9F,
      1.49054156E-8F,   -0.00648378627F,  -0.00158240634F,  0.000305733614F,
      6.26420951E-5F,   8.45661745E-8F,   0.000448702747F,  0.00129768706F,
      -0.000208508412F, 0.0221498944F,    2.23699637E-8F,   0.00013834484F,
      -1.12169101E-8F,  -0.00197838084F,  -0.00170650415F,  0.000119185F,
      -0.00158154103F,  0.000435638387F,  0.00108745741F,   1.35463381E-7F,
      0.000878801F,     4.55343887E-8F,   0.00602995465F,   0.00908282865F,
      0.00056613324F,   -0.00498746475F,  6.82143693E-8F,   -0.00169059739F,
      0.00043140573F,   0.00011163504F,   9.57343644E-8F,   -0.000431163615F,
      -0.00119223481F,  0.00240552449F,   0.00233749324F,   -0.000220960166F,
      0.000404203107F,  0.00019657478F,   -7.67844904E-5F,  -0.000222377173F,
      9.35829084E-5F,   0.000150072563F,  0.000585372618F,  -0.0151614649F,
      0.000140463992F,  0.00425454602F,   0.00103743339F,   1.11946044E-8F,
      -0.000389668101F, 0.00532033807F,   0.000557232124F,  -1.77303846E-5F,
      0.00165427988F,   -0.00198082882F,  0.000551891F,     1.17283685E-8F,
      0.00256587937F,   0.000638653059F,  -1.17465344E-8F,  -0.000592091354F,
      -5.73577454E-5F,  -0.00291438657F,  1.16874501E-8F,   -0.00134767417F,
      0.00178857613F,   -0.00024645892F,  -5.94887872E-9F,  0.000270035875F,
      1.55091186E-7F,   4.13549941E-8F,   -1.68495408E-5F,  0.000531349F,
      -3.5881591E-8F,   0.00181303499F,   -0.000117864765F, 2.62257458E-7F,
      -0.00753959408F,  1.97948822E-8F,   -0.00140254549F,  -0.00225905329F,
      0.0012692312F,    0.00127056893F,   4.83488329E-5F,   0.000524965755F,
      2.16307043E-8F,   1.37537448E-8F,   0.000402486825F,  4.52544313E-8F,
      0.000792267616F,  0.0015156134F,    0.00310990983F,   -3.30578622E-8F,
      0.00350990868F,   -1.22900232E-8F,  1.42644282E-7F,   -0.000314389821F,
      4.85866885E-8F,   0.000895503326F,  8.51732E-8F,      1.81005674E-7F,
      0.000339189079F,  0.00153243181F,   -6.07786278E-5F,  -6.15789375E-8F,
      -0.000416544499F, -0.000979548437F, -0.000574707519F, -5.57286617E-9F,
      -0.000475824287F, 0.000924485154F,  -2.0552573E-8F,   -0.00245873886F,
      -4.04880775E-5F,  -0.00027429004F,  -0.0160328075F,   0.00121956225F,
      -0.00148157927F,  -8.57465129E-5F,  -7.82507268E-5F,  0.00100520952F,
      0.00205911393F,   -0.0013249527F,   -0.0111046592F,   -0.00022159185F,
      2.17438782E-7F,   -6.47843335E-5F,  0.00294922898F,   0.000987111F,
      0.000432483095F,  -0.00270808418F,  0.000212751678F,  1.92126408E-5F,
      0.00549183274F,   -0.000286318827F, 0.00258628931F,   0.00326205604F,
      0.00825105328F,   -1.35468972E-5F,  -0.000690770219F, 0.0039903312F,
      -0.000429341133F, 0.000323923538F,  0.00420771493F,   -0.000816735032F,
      -0.000723833859F, -0.000191920568F, -0.00017797781F,  5.72652534E-5F,
      0.00083209842F,   -0.000875118654F, -0.000336799858F, -0.000186685313F,
      2.65329874E-8F,   0.00217362377F,   0.000124480081F,  -0.000407991203F,
      0.000672981783F,  -0.000462034863F, 1.61409247E-7F,   -0.000424687576F,
      -5.32613285E-7F,  1.29027642E-8F,   0.00133604405F,   0.00266438397F,
      -0.00251798751F,  -0.000370315072F, 0.000306713744F,  1.2186975E-8F,
      -5.40316E-9F,     0.0012836284F,    -0.00203353493F,  -0.00121669704F,
      0.00096515019F,   -1.65724458E-7F,  -0.000419955206F, -0.000314477016F,
      -2.70958699E-5F,  -0.0017315388F,   -0.000253272679F, 0.00222422415F,
      -0.000579891785F, -0.000152448119F, 5.06324454E-7F,   -0.000199321541F,
      -6.97168856E-9F,  -0.00879620947F,  1.02865449E-9F,   0.00838805921F,
      4.8779777E-5F,    0.000322887441F,  -0.000535893603F, -2.37086795E-9F,
      0.00107089337F,   0.000145168F,     -0.000198062247F, 0.00127573893F,
      -0.00334286713F,  -0.000831209065F, 0.000381909078F,  0.000571821176F,
      -0.00113252422F,  0.00430691335F,   -2.62387147E-7F,  0.000733117107F,
      -1.12276922E-7F,  2.87637967E-8F,   8.55294E-8F,      0.000266868155F,
      -0.000109079941F, -0.00350511284F,  0.001492218F,     -5.86991564E-6F,
      -0.00019566511F,  -0.00242876844F,  -0.00340239075F,  0.0022269208F,
      -6.71925372E-5F,  -7.44146078E-10F, -0.000414639886F, -3.06659203E-5F,
      -0.000103808285F, 9.63177538E-9F,   6.73012801E-9F,   -0.000939296791F,
      -0.00171915197F,  -0.0126915136F,   0.00128542818F,   -0.00217068195F,
      5.33409406E-9F,   -0.000744423189F, -0.00149445829F,  -0.0124418521F,
      -0.000256723F,    -1.12451701E-8F,  -0.00165228336F,  0.00045933947F,
      0.000450846041F,  0.000696767878F,  0.000237654735F,  1.73196968E-9F,
      0.00209340337F,   -4.95344138E-5F,  0.000803658215F,  -0.00478431815F,
      0.000519544876F,  0.000427116058F,  0.000107609507F,  -9.63279163E-5F,
      -0.000398131931F, -0.000719892501F, -0.000646649511F, -0.00112627621F,
      0.00224375608F,   0.000967278436F,  -0.000937907957F, -0.00419429271F,
      -1.39407547E-7F,  -5.39444954E-5F,  -0.000961592188F, 0.00509272795F,
      0.000962723512F,  0.000985623919F,  0.00311103882F,   -0.000723334902F,
      -2.38929569E-8F,  -0.00159013062F,  7.82273855E-5F,   0.000819475F,
      0.003018755F,     4.85028067E-7F,   0.000332970725F,  0.000459018804F,
      0.0030131354F,    0.000903537381F,  -0.000767625519F, -0.00201382721F,
      0.000967310916F,  0.000221992843F,  0.000253046455F,  0.00192629325F,
      0.000850584824F,  -0.000811695354F, -0.000478961272F, 8.04450107E-9F,
      -0.000440801F,    -0.00535053108F,  -4.22098929E-5F,  0.0015328197F,
      -0.00128061895F,  0.00144495314F,   8.10305137E-5F,   1.78430454E-7F,
      -0.000621330459F, 6.07181064E-5F,   -0.00115029735F,  0.000694865F,
      -7.99191475E-5F,  1.14684497E-6F,   0.00311308098F,   0.00122191687F,
      -0.00133955572F,  -0.00326373894F,  7.3857831E-5F,    -0.00286983163F,
      0.00209625764F,   0.000229302212F,  0.000834667764F,  -1.66070517E-8F,
      7.21058058E-8F,   -0.000116586729F, -8.06394862E-9F,  2.08889869E-5F,
      -6.52746821E-5F,  0.000300745625F,  0.00285408227F,   0.000968818F,
      -2.49442845E-11F, -0.000795372063F, 1.54446198E-8F,   -0.000154882713F,
      0.00350787281F,   0.0113273012F,    -0.00248680497F,  -0.00346380356F,
      0.00212084968F,   -0.000830100558F, 1.22827686E-7F,   0.000201024115F,
      -0.00179525849F,  0.000178682298F,  0.00102031638F,   0.000890244613F,
      0.000505926146F,  0.00273105432F,   0.000554617145F,  0.000422690966F,
      -1.14686713E-8F,  -0.000540394685F, -0.00190045813F,  0.0024198303F,
      -0.00184852735F,  0.00184927427F,   3.8680515E-5F,    0.000821588619F,
      0.00408970658F,   0.000900536368F,  1.16080727E-7F,   0.00169516413F,
      -0.0005134354F,   -0.000256902218F, -0.000385206484F, 0.00122188113F,
      0.000135550174F,  -0.000512254948F, -7.18263254E-5F,  -2.52908805E-9F,
      -0.000524728501F, -0.00260431017F,  -5.40568257E-9F,  -0.00377346319F,
      0.000555808365F,  0.000368458015F,  0.00102313561F,   0.000778603135F,
      0.000498884532F,  -2.14368523E-8F,  -0.00207782444F,  0.000734112691F,
      -0.00409793667F,  -0.00129304465F,  0.00106567936F,   0.000265357026F,
      0.0018537899F,    -0.000121525532F, 0.0101191364F,    -0.000704685925F,
      -0.00210478529F,  0.00868003443F,   1.28899364E-6F,   0.000747799815F,
      -0.000103250139F, 0.0139562F,       0.00345321768F,   0.000700572389F,
      -0.00179982244F,  -0.00208392832F,  -0.00215822062F,  -1.30439702E-7F,
      -0.00237715361F,  5.51285757E-6F,   0.000777726F,     0.00237889239F,
      -0.0011820686F,   -7.92689316E-5F,  4.30285727E-5F,   -0.00104324648F,
      9.56719305E-5F,   -0.00388220279F,  8.98294957E-7F,   -5.03495926E-7F,
      -4.50439302E-6F,  0.00328829675F,   -0.00362863718F,  9.82187776E-5F,
      0.000649373047F,  -1.19350432E-6F,  0.000215431588F,  0.00534297386F,
      -0.00513468497F,  0.00139319093F,   -2.16427202E-6F,  0.000395458803F,
      -1.14486613E-6F,  -0.0150906211F,   -0.00804119464F,  0.00188423507F,
      -0.00353616127F,  -0.000156963302F, 0.0073074149F,    -1.21489893E-6F,
      -0.000340031169F, -1.00104917E-5F,  0.00269926F,      -9.37778314E-5F,
      0.00175459357F,   -0.00414663134F,  9.37252764E-7F,   0.00139841076F,
      0.00597788952F,   0.00322709908F,   1.52005987E-6F,   -0.000735041511F,
      -0.00447733235F,  0.00294774771F,   0.0193830766F,    0.000293634774F,
      0.00283747353F,   -0.00200884766F,  -0.00135368737F,  -0.0045040315F,
      0.00158766471F,   -0.000689010776F, -0.0016463038F,   0.0019978669F,
      0.000703342957F,  -0.000440224714F, 0.00286643743F,   -1.10364444E-5F,
      0.000489369617F,  0.000835920044F,  0.00256235013F,   0.00248836447F,
      0.00103502697F,   -0.00570605276F,  0.00733674F,      -1.10394058E-6F,
      0.0136617227F,    0.00389556889F,   -8.63177405E-8F,  -0.00322125922F,
      -0.000156680078F, -0.00960386172F,  -1.53558608E-6F,  -0.00129738904F,
      0.0162362047F,    0.00272091548F,   -1.11821828E-5F,  -0.00244134804F,
      -6.08256187E-7F,  -1.9531592E-6F,   -0.00316065433F,  0.00114938407F,
      -2.73492606E-6F,  0.00574770477F,   -0.00013851907F,  -2.12252326E-6F,
      -0.00495993765F,  -5.66137601E-7F,  -0.00131122908F,  -0.00383419567F,
      0.00463436171F,   0.00794693734F,   0.00692229299F,   0.00338051678F,
      -3.65206279E-6F,  -2.02873753E-6F,  -0.00123401F,     2.19130834E-6F,
      0.00547955278F,   0.00149048143F,   0.00187136512F,   -2.44465764E-6F,
      0.0118311942F,    -1.32225414E-5F,  -7.40707492E-6F,  0.00295849354F,
      -3.6876479E-6F,   0.00223152782F,   -1.55933976E-5F,  -1.2701106E-6F,
      0.0056982683F,    0.0129709654F,    0.00030213545F,   -2.80980362E-6F,
      -0.000123062186F, -0.00148677803F,  -0.00148036645F,  -4.82847554E-7F,
      -0.000362838327F, 0.00475785602F,   -1.46523616E-5F,  -0.00300695468F,
      -0.00175445352F,  -0.000115444724F, -0.00807785522F,  0.0109057818F,
      -0.00237149303F,  -0.0018834623F,   -0.000263287686F, 0.00505635142F,
      0.0117033785F,    0.0104471315F,    -0.0165906288F,   -0.00286040874F,
      -5.58466563E-5F,  -5.33336242E-6F,  0.00387001038F,   0.00173576933F,
      0.00087385677F,   -0.00554093765F,  0.00132782047F,   -0.00316331442F,
      0.000312773831F,  -0.00126289483F,  0.00130414485F,   0.0021730056F,
      0.00165789563F,   -0.000445396407F, -0.00213065115F,  0.00104201888F,
      -0.00237383088F,  0.00299814367F,   0.000487589F,     -0.00135645398F,
      -0.00509909587F,  -7.68209502E-5F,  0.000109182423F,  -0.0028005871F,
      0.00460196845F,   -0.00490482198F,  -0.00074026F,     0.00790649559F,
      -2.3090663E-6F,   0.000661785249F,  0.00475036679F,   -0.000474118773F,
      0.00469178939F,   -0.000488299818F, -1.57031093E-6F,  0.00328777055F,
      -2.25377698E-6F,  -1.16619594E-6F,  -0.000960254925F, -0.000536519627F,
      -0.00447376911F,  -0.00277029094F,  0.000905901368F,  -1.0861655E-5F,
      -2.39941471E-8F,  0.00324610947F,   -0.000950105838F, -0.00103024719F,
      0.00820626412F,   -2.49686354E-6F,  0.00165034086F,   -0.00110705569F,
      0.000110540495F,  -0.00282856F,     -0.00506977271F,  0.000579773157F,
      0.00306463125F,   -0.000398716336F, -1.91214895E-5F,  -0.00123496226F,
      -1.15680598E-6F,  -0.0116446847F,   1.35209257E-6F,   0.0154269449F,
      0.0013732122F,    0.00175669463F,   -0.00805277F,     4.49907311E-5F,
      0.0020701834F,    0.00216328888F,   0.000192997482F,  -0.00142005016F,
      -0.00372802094F,  -0.000460836862F, 0.0016272061F,    0.00181180006F,
      -0.00174265716F,  -8.94058539E-5F,  -2.24792984E-6F,  0.00389488135F,
      -1.32030627E-5F,  -1.24159601E-6F,  -1.1683627E-6F,   -0.00325507205F,
      -0.000583154208F, -0.0401082896F,   0.00944636669F,   7.22973127E-5F,
      0.000876044796F,  -0.00164773175F,  -0.00201182417F,  0.0050464333F,
      -9.38832527E-5F,  -1.93678233E-7F,  -0.00771169551F,  0.00108483783F,
      -8.0435595E-5F,   -1.5769017E-6F,   -7.34909E-7F,     -0.00174351456F,
      -0.00150844769F,  0.0102385879F,    0.00530199753F,   8.88689374E-6F,
      -2.71572185E-6F,  -0.00202608109F,  -0.00264016562F,  -0.00194573915F,
      0.000222187722F,  -9.47571243E-6F,  -0.00563396187F,  0.000630301423F,
      0.00176250748F,   0.0061320588F,    0.00214234227F,   -3.15420976E-7F,
      0.00431864662F,   0.000333646487F,  0.0104078427F,    -0.00454858F,
      -0.00143996836F,  0.00346858823F,   0.00164382195F,   0.000925151922F,
      0.00410118373F,   0.00148778607F,   -0.00516896741F,  -0.0053940909F,
      0.0030887106F,    0.00192956056F,   -0.00424034754F,  -0.000123899255F,
      -6.38757137E-6F,  0.00387661811F,   -0.00253324164F,  0.00197383738F,
      -0.000548747776F, 0.00244216598F,   0.00273738895F,   -0.00536405807F,
      -2.15163709E-6F,  -0.00568829104F,  0.000455613103F,  0.00439922186F,
      0.00156941568F,   0.00233321683F,   -0.00130860542F,  -0.000932293537F,
      0.00428919727F,   0.00565545401F,   0.00815972872F,   0.000344916392F,
      0.00511381729F,   -0.000816424843F, 0.00135166256F,   0.0086927535F,
      0.00471998565F,   -0.0120912874F,   -0.000160153417F, 2.75457774E-6F,
      8.15529493E-5F,   -0.00449094828F,  -0.00321841869F,  0.00413978472F,
      -0.00250476948F,  0.00919340551F,   0.00172741129F,   7.46565888E-7F,
      -0.00166853855F,  0.00305227283F,   0.00138871791F,   0.000647124834F,
      -0.00288718403F,  -3.24397406E-5F,  0.00556022394F,   0.0104965428F,
      -0.00406423444F,  -0.0050509274F,   0.000107805499F,  0.000471867505F,
      0.00207663956F,   0.00359010114F,   0.0056815343F,    -2.43123577E-6F,
      -1.25007455E-6F,  -3.81828359E-5F,  -2.72877423E-6F,  0.000552076381F,
      0.000563133101F,  -0.000621883897F, 0.00107270572F,   0.0073453621F,
      -2.85090493E-7F,  0.00115057023F,   -2.28501303E-5F,  9.81413759E-5F,
      0.00476647401F,   0.0014167995F,    0.00394328218F,   -0.00117331801F,
      0.00188498746F,   -0.00146810047F,  4.50683E-7F,      0.00145724404F,
      -0.00787477288F,  -0.00108971458F,  0.00137221732F,   0.00555165717F,
      0.00395899732F,   0.00904021F,      0.00039658256F,   0.00305516F,
      -9.74981594E-6F,  0.0034269928F,    0.00193566689F,   0.00331847928F,
      -0.000429700653F, 0.0028244406F,    -0.000962291087F, 0.00200138334F,
      -0.000564099464F, -0.000120560115F, 3.57787485E-7F,   0.0107595697F,
      -0.00480370689F,  -0.00168188068F,  -0.000875954167F, 0.00804183353F,
      0.000329056289F,  -0.00518230256F,  -1.52346493E-5F,  2.4328881E-7F,
      -0.00766102504F,  -0.00415832177F,  -8.72975875E-7F,  -0.000663719897F,
      0.00462072808F,   -0.00210738019F,  0.00757238502F,   0.00118128851F,
      0.000353429583F,  -3.240566E-6F,    -0.00658666901F,  0.00361638144F,
      -0.000270380493F, -0.00130670622F,  0.00104360399F,   -0.000267012161F,
      -0.01582231F,     -0.0287004393F,   0.00725119794F,   -0.0283764601F,
      0.0285079051F,    0.044722531F,     -0.00356037263F,  -0.0531911589F,
      -0.0012618152F,   -0.0199747961F,   0.0321562737F,    0.0229104273F,
      -0.0081299413F,   0.0399918407F,    -0.0317557193F,   -0.00115209923F,
      0.0146394977F,    -0.00315762521F,  0.0564600788F,    0.0488817766F,
      -0.0767524391F,   0.0778929442F,    0.0666793287F,    -0.014769488F,
      0.0305004194F,    0.0410555489F,    -0.00141921046F,  -0.0012939244F,
      -0.00175113685F,  0.087632075F,     0.0609523095F,    0.0379608981F,
      -0.0129310014F,   -0.00260858121F,  0.0476662666F,    -0.125579402F,
      0.0219285153F,    0.00186504726F,   -0.00119358813F,  -0.00769193843F,
      -0.0010394518F,   0.0482654199F,    -0.0738261268F,   -0.00644078106F,
      0.0799218267F,    -0.00389609812F,  0.0208502691F,    -0.00181361509F,
      -0.0660661459F,   -0.00409718649F,  0.058876805F,     -0.0180407166F,
      -0.00879048835F,  -0.0573014282F,   -0.00215207622F,  -0.0684870109F,
      0.0786364526F,    -0.0359295905F,   -0.00185034273F,  0.0461590774F,
      0.00911614951F,   -0.00527866231F,  -0.0790543109F,   0.0559230559F,
      -0.0132191F,      0.058070533F,     -0.00894173F,     0.0480822921F,
      0.0127524305F,    -0.000205311939F, -0.038407851F,    0.0188630205F,
      -0.020132266F,    -0.040883828F,    -0.0288711078F,   -0.00314508844F,
      0.0065937792F,    0.102446906F,     -0.00545325736F,  -0.0648303628F,
      0.0655681938F,    -0.00130953651F,  0.0489972271F,    -0.00197505252F,
      -4.55525369E-5F,  -0.0153238466F,   -0.0012923691F,   -0.004291947F,
      -0.00422333973F,  0.0525630787F,    -0.000822998874F, 0.0691921189F,
      -0.00412065722F,  0.027325362F,     -0.00238044211F,  0.0749870762F,
      -0.00183526415F,  -0.00255069789F,  0.132010296F,     0.0561313815F,
      -0.0014194767F,   -0.0254597254F,   -0.00434921402F,  -0.00210443675F,
      -0.0634295121F,   -0.00381643185F,  0.0580710731F,    0.0367889442F,
      0.0838253945F,    0.0176746249F,    0.00926579349F,   -0.00640525203F,
      -0.00133598759F,  -0.00229717349F,  -0.01605919F,     -0.000909442431F,
      -0.0139889959F,   0.0182157047F,    0.0247501377F,    -0.000875935366F,
      0.00166230835F,   -0.00283951568F,  -0.000556892424F, 0.0393904559F,
      -0.00106494199F,  -0.0555834509F,   -0.00424992852F,  -0.00136661041F,
      -0.0244784039F,   0.0474498644F,    -0.00352295837F,  -0.00600730861F,
      0.0317507833F,    -0.0780127496F,   -0.00724651618F,  -0.00264064083F,
      0.0396284871F,    0.0314164646F,    -0.00297418679F,  0.0200587325F,
      -0.011654295F,    0.0455406271F,    0.00470884703F,   0.0389103629F,
      -0.000217549707F, 0.0308642201F,    0.0002224739F,    -0.00795765594F,
      0.0171669815F,    0.274304926F,     -0.0179748051F,   -0.0131686386F,
      -0.000407817977F, -0.0100224968F,   0.0376421735F,    0.0464675389F,
      0.0556081608F,    0.0247956719F,    0.0375051424F,    0.124092519F,
      0.0108047333F,    0.035287831F,     -0.0399465449F,   0.0182617716F,
      0.0396860875F,    0.00217906688F,   -0.0144831268F,   0.0249882899F,
      -0.0614155456F,   -0.0045847022F,   0.0321005285F,    0.0466346592F,
      -0.0114253964F,   -0.0144078266F,   -0.00783495232F,  0.00364051387F,
      0.0421726257F,    0.0670871735F,    -0.00396800181F,  0.0191763863F,
      -0.00304391049F,  0.0214059372F,    0.0874320939F,    0.014474662F,
      0.0934601575F,    -0.0152735179F,   -0.00158773386F,  -0.00903168321F,
      -0.00304019405F,  -0.00147657958F,  -0.0295195729F,   0.0168860145F,
      -0.0593881048F,   -0.00878075697F,  0.0283580888F,    -0.00190772908F,
      -0.00163569767F,  -0.0421005972F,   0.0335885175F,    0.0926869735F,
      0.0181579255F,    -0.00532234041F,  -0.0388960093F,   -0.0216952618F,
      0.0133205811F,    0.00799317F,      -0.0680928305F,   0.0758019164F,
      -0.0267995726F,   -0.0111312838F,   -0.00115827296F,  -0.033350829F,
      -0.00168457534F,  0.0303059816F,    -0.00181977893F,  0.0409559272F,
      -0.00501511851F,  -0.00850649457F,  0.00309231202F,   -0.00430251472F,
      0.0108394353F,    -0.00842494331F,  0.0250739865F,    -0.0314767919F,
      -0.0475602672F,   0.00196920475F,   0.0503065251F,    0.0226658806F,
      -0.0673483089F,   0.0526186936F,    -0.00173550937F,  0.0719005913F,
      -0.000765604782F, -0.00148326915F,  -0.00180914684F,  -0.0120448228F,
      -0.00328640058F,  0.104265161F,     0.0440061279F,    -0.00328833214F,
      0.0389621854F,    0.0210280642F,    0.000945989857F,  0.01369101F,
      -0.0041781296F,   -0.00106797006F,  0.0402006283F,    -0.00726093212F,
      -0.00988211203F,  -0.00010855648F,  -0.000560770452F, -0.0432202406F,
      0.0684985667F,    0.0362675227F,    0.0217412915F,    0.0519560538F,
      -0.00250359648F,  0.0418184F,       0.00173178467F,   0.0994046256F,
      0.0145221306F,    -0.00267645065F,  -0.048989404F,    0.0227749981F,
      0.0716335401F,    0.0493851751F,    0.000827576092F,  -0.00151263131F,
      0.0291760378F,    0.00327685126F,   0.0379194431F,    0.0293714423F,
      -0.0122691253F,   0.0145458477F,    -0.00508512929F,  0.0598135069F,
      0.0178293139F,    0.0425313041F,    0.00799499173F,   0.0148614449F,
      0.0134382993F,    0.0344943255F,    0.0308643878F,    0.028231306F,
      -0.00264963F,     -0.0139258225F,   -0.0237027593F,   0.112708628F,
      -0.00706538372F,  -0.00836548302F,  -0.0180336F,      0.0138338646F,
      -0.000549578515F, 0.0304927621F,    -0.0127392905F,   -0.0220573209F,
      0.0443756133F,    0.0252277255F,    0.0943917111F,    0.178685412F,
      0.00387763465F,   0.0701134875F,    -0.043132592F,    0.129976079F,
      -0.0223677754F,   -0.0411878303F,   0.0408616662F,    -0.0544865914F,
      0.0515354089F,    -0.181619212F,    -0.00801019091F,  -0.00303067057F,
      -0.020674644F,    0.0303463042F,    0.0130057121F,    -0.00887624454F,
      0.048058629F,     -0.0629340261F,   -0.00663971F,     -0.000808569486F,
      -0.0161143392F,   0.0952689797F,    0.0785281882F,    0.00825148448F,
      0.00565170543F,   -0.00224436913F,  0.0215930827F,    0.0583951734F,
      -0.13235119F,     0.0695542395F,    -0.0387319066F,   0.0469833687F,
      0.01588111F,      -0.00253329449F,  0.0455938131F,    -0.000579603249F,
      -0.00130513695F,  -0.00501787569F,  -0.00363638229F,  -0.00613769609F,
      -0.00430330215F,  -0.00581010617F,  0.0133900763F,    0.0279216338F,
      -0.000705181912F, 0.0318153948F,    -0.00220047403F,  -0.00400752854F,
      0.000683765684F,  0.0628010482F,    -0.0690103918F,   0.0647906736F,
      0.0239002798F,    -0.00738921249F,  -0.00149966904F,  0.00443803333F,
      0.00797752198F,   -0.0260138661F,   0.0432294682F,    0.0128725208F,
      0.0185271595F,    -0.0574015453F,   -0.000338527898F, 0.00100014464F,
      -0.00240237801F,  -0.0120029375F,   -0.0760626271F,   0.0109908748F,
      0.0195193198F,    0.0277291574F,    -0.011139458F,    0.0996627957F,
      0.0589297749F,    0.0134508433F,    -0.00160265167F,  -0.00456246501F,
      0.0703182444F,    0.0483947285F,    -0.00686494168F,  -0.00528599788F,
      0.0030769466F,    -0.00496322149F,  -0.00424038386F,  -0.00183651748F,
      0.0168884806F,    -0.0644621775F,   -0.00220085168F,  0.111001953F,
      0.0388000123F,    -0.0694656819F,   0.0369515829F,    0.0307016689F,
      0.00328622689F,   -0.000846526411F, 0.0029758825F,    0.0214720685F,
      0.0191635191F,    -0.0236045793F,   0.094518505F,     0.0239615198F,
      -0.000589584757F, 0.00893691275F,   0.0104832426F,    -9.29690286E-5F,
      0.00346133462F,   0.0108377393F,    -1.40071988E-5F,  0.0270220209F,
      -0.0250215307F,   -0.0258353967F,   -0.00493613165F,  0.00199575583F,
      0.00147697108F,   0.0053867097F,    -0.00319898152F,  -1.47433957E-6F,
      0.0129740322F,    -7.34615833E-6F,  -0.0410162359F,   -0.063496F,
      0.0191647168F,    0.0131334299F,    -0.0157983918F,   0.0184226613F,
      -0.0479094461F,   -0.109260276F,    -1.17864499E-6F,  3.68679906E-7F,
      3.22227925E-6F,   -0.0815102533F,   0.0408668928F,    -0.00249218591F,
      -0.00327833625F,  3.89096158E-6F,   0.0177217331F,    0.0149485702F,
      -0.00107648107F,  0.379923135F,     -7.17992464E-7F,  -0.0017069733F,
      -1.69377756E-9F,  -0.067132391F,    -0.0194732621F,   -0.00190711324F,
      -0.0136927254F,   0.00211431342F,   0.0873862F,       2.64377263E-6F,
      -0.0206709281F,   1.29203272E-7F,   -0.0347241387F,   -0.0113121849F,
      0.00295824767F,   -0.0811045393F,   -2.14788906E-6F,  0.0282992013F,
      0.005710667F,     0.0102033755F,    -4.69179577E-6F,  -0.0172506534F,
      0.0164334811F,    -0.00136152073F,  0.0135238497F,    0.00151964289F,
      -0.00573427556F,  -0.00697166426F,  -0.00486354623F,  0.0266474113F,
      0.0026398988F,    -0.00775564974F,  -0.102566555F,    0.138491035F,
      -0.00253300299F,  -0.00125947257F,  -0.027189875F,    2.68329813E-7F,
      -0.0212719124F,   -0.0224777088F,   -0.00983801764F,  0.0109752733F,
      0.0397920795F,    -0.0468471646F,   0.00810967758F,   3.52439156E-6F,
      0.0365553759F,    -0.0195738804F,   -4.35918E-7F,     6.22986336E-5F,
      -0.000844082213F, -0.0506469496F,   2.33789183E-6F,   0.00463824533F,
      -0.0238536131F,   -0.020117566F,    -2.10495546E-6F,  0.0268317927F,
      1.5080592E-6F,    3.245887E-6F,     -0.0260677282F,   -0.00353728933F,
      2.27002124E-6F,   0.00056680711F,   -0.00143486413F,  4.70600162E-6F,
      -0.062718071F,    1.39722863E-6F,   0.0059358906F,    -0.00472017936F,
      -0.0292565748F,   -0.0436785854F,   0.0101906704F,    -0.00296704401F,
      -6.33565492E-7F,  1.43928901E-6F,   0.00127966935F,   1.51376335E-5F,
      0.0168609265F,    -0.0121106049F,   0.0084601948F,    2.16716876E-6F,
      0.0332501493F,    -2.90194566E-6F,  2.78327025E-5F,   0.00499550905F,
      3.01881801E-6F,   -0.0406361036F,   -1.34095444E-5F,  2.05981632E-6F,
      0.0145854466F,    0.00053467229F,   -0.014817602F,    -9.33886895E-7F,
      -0.0111909211F,   -0.00965734F,     0.0050587263F,    3.58533782E-7F,
      -0.0427981764F,   0.00495598745F,   8.63602736E-7F,   0.00317396154F,
      -0.0197268315F,   0.00530636311F,   -0.323213369F,    0.00658057723F,
      0.000905556139F,  0.00199623941F,   0.00346324593F,   -0.0101475511F,
      0.0257139783F,    -0.0239760187F,   -0.170935288F,    0.00292816386F,
      9.57714074E-5F,   -0.00219025114F,  -0.0433754586F,   0.00849156734F,
      0.0123513546F,    0.019453045F,     -0.0453062057F,   -0.0304306522F,
      0.0830208138F,    -0.00510918628F,  0.000435123744F,  0.0587968938F,
      0.301578045F,     -0.0101021761F,   -0.0195419863F,   0.033872813F,
      0.00226519234F,   -0.00306569226F,  -0.00398503616F,  -0.0136659862F,
      0.015339165F,     -0.00819152687F,  -0.00244731945F,  0.0167228896F,
      0.00632898F,      0.00623551104F,   0.00353770819F,   0.0164333172F,
      7.01842453E-7F,   0.0251707342F,    -0.0508494861F,   0.00747641595F,
      0.000982953119F,  -0.000677193457F, 1.69585883E-6F,   -0.0150917778F,
      -2.25994791E-5F,  1.07562255E-6F,   0.0331991948F,    0.0417147614F,
      -0.0349975899F,   -0.00677210139F,  0.0155373514F,    -7.91491129E-6F,
      -3.38983853E-7F,  -0.05078578F,     -0.0150866127F,   0.128997311F,
      0.0037933758F,    -2.52349782E-5F,  -0.0267229117F,   0.000920439546F,
      0.00520997168F,   -0.0110146319F,   0.0193966236F,    -0.0924841538F,
      0.00951539F,      -9.43571831E-6F,  1.89017956E-5F,   0.00343812024F,
      3.2425379E-7F,    -0.0278098583F,   1.29636294E-6F,   -0.0484896712F,
      -0.00113802461F,  -0.000127872045F, 0.014858393F,     -6.36002151E-5F,
      0.00569956191F,   -0.00238059252F,  -0.0228769723F,   0.0272141416F,
      -0.00201006071F,  -0.0488664694F,   -0.00786457863F,  -0.0476430394F,
      -0.0567964464F,   -0.0130834235F,   -1.71026909E-6F,  -0.00986653287F,
      -1.59553929E-5F,  -2.22924473E-6F,  1.99192459E-6F,   0.000708791486F,
      -0.0126290517F,   -0.120622344F,    0.00670968415F,   -0.000285585F,
      0.00626239972F,   -0.0297878664F,   -0.0245117191F,   0.00491698924F,
      -0.00076913106F,  -1.93982646E-6F,  -0.00072876527F,  -0.00344258943F,
      -0.00271427142F,  4.72978263E-6F,   -8.82672225E-7F,  -0.0118068159F,
      -0.0237856172F,   0.0861621872F,    0.00585959246F,   0.0236543603F,
      2.98243322E-6F,   -0.00909107551F,  0.0232995879F,    -0.0658230409F,
      -0.00188625F,     1.30016497E-5F,   -0.0304695945F,   -0.00138676912F,
      -0.0670745894F,   -0.0431497954F,   -0.00152746704F,  1.9474632E-7F,
      0.0479970314F,    -0.000226356045F, 0.0145413177F,    -0.0102166655F,
      -0.033919584F,    0.00323690567F,   -0.00160079845F,  0.0246783663F,
      -0.00046118864F,  -0.0534322336F,   -0.0223141331F,   0.0280177724F,
      -0.0161484946F,   -0.0627389625F,   0.00196670252F,   0.00337465014F,
      7.8668545E-6F,    0.00352554652F,   -0.0243514553F,   0.00457609026F,
      -0.0105510047F,   -0.000720900425F, 0.00424044207F,   0.0589866675F,
      2.47265802E-7F,   0.0228121243F,    -0.00131369801F,  0.00439295825F,
      0.134464502F,     -0.0201280322F,   -0.0103469053F,   -0.00152058119F,
      0.0271003693F,    -0.0016663773F,   -0.011220267F,    0.0328036733F,
      0.0046659247F,    0.0157624111F,    0.00147905131F,   0.0750177354F,
      -0.0122960778F,   -0.130073428F,    -0.000272145553F, -1.70429269E-6F,
      0.00790347F,      0.0236860681F,    -0.0112243658F,   -0.00194538F,
      0.0323372483F,    -0.0190258864F,   0.00922692847F,   1.78347327E-5F,
      0.00214647735F,   0.0137622925F,    0.0743262395F,    -0.00854079332F,
      0.00341131655F,   9.97497409E-6F,   0.0228160638F,    0.00521910517F,
      -0.0732101053F,   -0.0165111367F,   -0.000576478429F, 0.040866673F,
      -0.0239682123F,   -0.00312150712F,  0.00444748066F,   -5.98025963E-7F,
      4.20615925E-6F,   -0.0013119377F,   -3.39536115E-7F,  -0.000885664311F,
      -0.00180473062F,  0.0259469412F,    -0.0202888865F,   -0.0197135806F,
      1.41004898E-6F,   0.000405176077F,  -6.65757034E-6F,  -0.000955286F,
      -0.0979917F,      0.20100908F,      0.00405183621F,   -0.00922788121F,
      0.0342134796F,    0.000563930487F,  8.73218539E-7F,   -0.00145151361F,
      -0.00843377132F,  0.0138834929F,    -0.00491838576F,  0.00243080058F,
      0.0675691217F,    -0.0216296F,      -0.0226348471F,   0.000442733086F,
      2.85689566E-6F,   -0.036891643F,    0.0209177341F,    -0.0563200153F,
      -0.0369549207F,   0.0519519225F,    -0.00450052647F,  0.01660024F,
      0.0330222212F,    0.0296827219F,    1.34954973E-6F,   -0.0187856518F,
      -0.00210601022F,  -0.0267155524F,   0.0129132746F,    0.012797731F,
      -0.000357178855F, 0.00216351729F,   -0.000988447573F, 6.22921959E-7F,
      0.00207237387F,   -0.0486233532F,   7.43597639E-7F,   -0.0157124624F,
      0.00249684951F,   0.0392691977F,    0.00480624102F,   0.0114861606F,
      -0.0020355063F,   -4.49374693E-6F,  0.0262243189F,    -0.00608779164F,
      0.0622117147F,    -0.0461144671F,   0.00114901282F,   -0.00350975641F,
      0.00355112157F,   -0.0108019989F,   -0.0102171116F,   -0.0310056545F,
      -0.0480823666F,   -0.0208679438F,   -0.00384050445F,  0.0249867979F,
      0.0129834134F,    0.0134788156F,    0.0286888108F,    0.0227460898F,
      -0.0201881193F,   0.0812201202F,    0.0158775747F,    -0.00113696407F,
      0.00967662781F,   -0.00274336967F,  -0.00130538316F,  0.048363924F,
      -0.00244512549F,  -0.0335507207F,   0.0185392834F,    0.0267639421F,
      -0.043456316F,    0.0455972403F,    -0.00137581327F,  -0.00130006485F,
      -0.00174932578F,  -0.0500497855F,   0.0173188876F,    0.0618226714F,
      -0.0104014268F,   -0.00257684616F,  0.0575764F,       0.0646848157F,
      -0.0352045596F,   0.0375043862F,    -0.00113091397F,  -0.0133103179F,
      -0.00102704135F,  -0.0606794395F,   -0.0362584F,      -0.0146886501F,
      -0.0380125F,      -0.00139838364F,  -0.0290056281F,   -0.00172230904F,
      0.00945110712F,   -0.00420811772F,  0.0530228056F,    0.0708765909F,
      -0.00362727651F,  0.0785443634F,    -0.00175460149F,  0.0740075633F,
      0.0183359124F,    0.00637001405F,   -0.00178076176F,  0.0230751559F,
      0.00493821921F,   -0.0312062651F,   -0.00130973489F,  -0.0496183485F,
      -0.0296276379F,   -0.046442695F,    -0.0231450684F,   0.0703424588F,
      0.0137215285F,    -0.0418072753F,   -0.027034238F,    0.0210936219F,
      -0.00268356246F,  0.0517989285F,    0.0136489533F,    -0.00318922522F,
      -0.00274785259F,  0.0561746135F,    -0.0961518958F,   -0.0227618273F,
      0.000336703F,     -0.0541660339F,   -0.0188029483F,   -0.00212394726F,
      0.0282991938F,    0.0671310052F,    -0.0012602167F,   -0.0368665792F,
      -0.00421355106F,  0.108994044F,     -0.000796196342F, -0.0368533023F,
      0.0458264649F,    0.0244493727F,    -0.00241069309F,  0.0187234674F,
      -0.0018678013F,   -0.00257671F,     0.119400546F,     0.0417238027F,
      -0.00142648409F,  -0.0300027691F,   -0.00462245F,     -0.00205969089F,
      -0.0427272692F,   -0.0037462567F,   0.0492763482F,    0.0651779845F,
      -0.0294837616F,   0.00311665796F,   0.0450610183F,    -0.0127619542F,
      -0.0013001794F,   -0.00225719507F,  -0.0167567302F,   -0.000896017F,
      0.0584303364F,    0.0154693797F,    0.0288685188F,    -0.000845260394F,
      0.081467F,        -0.00281907804F,  -0.000497219793F, 0.0353484079F,
      -0.00104628422F,  -0.0169035606F,   -0.00423301104F,  -0.00137247646F,
      0.11482133F,      -0.0177360047F,   0.00683914917F,   -0.0059493524F,
      -0.070822224F,    0.0592009574F,    0.00652889302F,   -0.00261278939F,
      0.0203440879F,    -0.00568089169F,  -0.00297998497F,  0.0171950068F,
      -0.00695372513F,  0.00685691135F,   -0.0669449046F,   -0.00400739349F,
      -0.105197489F,    -0.0239231065F,   -0.0162809081F,   0.0193502661F,
      0.0487478822F,    -0.0802705362F,   -0.184625611F,    -0.0147575522F,
      -0.000396864576F, -0.0167003945F,   0.0408960506F,    -0.0095227F,
      0.0336552747F,    0.0185196381F,    -0.0635825098F,   -0.081758067F,
      0.0624655373F,    0.00497926725F,   0.031571012F,     0.0471132919F,
      -0.0993776619F,   0.00484596519F,   0.0101978909F,    0.0332904868F,
      0.0134671954F,    -0.0244512148F,   -0.00797651149F,  -0.0772525668F,
      0.0365381502F,    0.0194521081F,    -0.00557486573F,  0.00985082332F,
      0.0519374348F,    -0.0431382544F,   0.0141089344F,    -0.031351462F,
      -0.00304801855F,  0.0319538116F,    -0.0494938828F,   0.0151333231F,
      0.0630953759F,    -0.0273174F,      -0.00156462716F,  0.0208395738F,
      -0.00266572251F,  -0.00146794785F,  -0.00749519514F,  -0.00807816908F,
      0.0119876191F,    -0.0346851796F,   0.0198776163F,    -0.00191216194F,
      -0.00158399099F,  0.0365009382F,    0.00292704697F,   0.114938356F,
      -0.0191104393F,   -0.00529980613F,  0.0275428221F,    -0.007068804F,
      -0.00386327365F,  0.0116436752F,    0.0223946F,       0.0851996616F,
      -0.0148494337F,   -0.0132945646F,   -0.00117241498F,  -0.013713086F,
      -0.00163041626F,  -0.0989770666F,   -0.00177033129F,  -0.0473467372F,
      -0.00715593714F,  -0.0136505635F,   -0.00307443715F,  -0.00374391489F,
      0.0146373687F,    -0.0157792754F,   -0.0631584153F,   0.0888482854F,
      0.123982452F,     -0.0615311861F,   -0.0436972119F,   0.0220846608F,
      0.0731418952F,    0.0709072351F,    -0.00134719233F,  -0.0133924019F,
      -0.000799825415F, -0.00148548849F,  -0.0017968585F,   -0.0661563724F,
      -0.000172219385F, -0.0804899707F,   0.0611199401F,    -0.0030917204F,
      0.0233139098F,    0.0267760549F,    -0.0170754883F,   0.011560169F,
      -0.00469155191F,  -0.00103068457F,  -0.0475496054F,   -0.0228492562F,
      -0.0302229859F,   -0.000134145012F, -0.000543030736F, 0.018178558F,
      -0.051758945F,    -0.0266723689F,   0.0303132851F,    -0.0282736477F,
      -0.00246393355F,  -0.0506447181F,   0.0517255515F,    0.108328529F,
      -0.00623640278F,  -0.00266044331F,  -0.0120310774F,   -0.0195778161F,
      -0.0188391209F,   0.0822780132F,    -0.0202917624F,   -0.00145379372F,
      -0.0468175262F,   -0.023090804F,    0.0499331653F,    0.0376620144F,
      -0.00702901045F,  -0.0103540383F,   -0.012330926F,    0.0622581206F,
      -0.0320944041F,   0.0102999937F,    -0.0198310018F,   0.0196531843F,
      0.0472286679F,    0.023093665F,     0.0545649789F,    0.0326568857F,
      -0.00252061058F,  -0.0176179316F,   -0.0320429541F,   0.0231039617F,
      -0.0421662405F,   -0.0233562235F,   0.00958892F,      0.0167011507F,
      -0.00051730877F,  0.0261259917F,    -0.010831506F,    0.0466293655F,
      -0.0303484537F,   -0.00543384161F,  -0.0138183078F,   0.0279228259F,
      0.0244113319F,    0.0098525621F,    0.041274514F,     0.0417552069F,
      0.0573944524F,    0.0356116034F,    -0.0418411419F,   0.0443232208F,
      -0.0771622211F,   -0.104988568F,    0.000822681759F,  -0.00293934F,
      -0.0109441467F,   0.0397457331F,    0.0699320883F,    -0.00304403F,
      0.0788946748F,    0.00184108515F,   -0.0147476569F,   -0.000809819147F,
      -0.00980152749F,  -0.019164186F,    0.112534732F,     0.0569827929F,
      0.0197490212F,    -0.0018826084F,   0.0483917966F,    -0.0157653764F,
      0.0894356817F,    0.068449758F,     -0.0170480106F,   -0.000946109765F,
      -0.0347467288F,   -0.0321623385F,   -0.0171690229F,   -0.000551813166F,
      -0.00132775377F,  -0.00504965F,     -0.00362896407F,  -0.00799620338F,
      -0.00742631499F,  0.131872788F,     0.0129222544F,    -0.0157947075F,
      -0.000655006326F, 0.0301803127F,    -0.00221745204F,  -0.00690545607F,
      0.0725130737F,    -0.0177280698F,   0.108203039F,     0.0529132F,
      0.0417112112F,    -0.0115678255F,   -0.00147747714F,  -0.0213237684F,
      -0.0392146781F,   0.00365302595F,   0.0879361108F,    -0.0244729426F,
      0.0431293175F,    0.0422127284F,    0.00672616344F,   -0.0124159073F,
      -0.0024139497F,   0.083947F,        0.0246199016F,    0.0212184712F,
      -0.0313696079F,   0.0470634699F,    -0.0598811321F,   0.108256772F,
      0.0715977699F,    0.0393535979F,    -0.00159596454F,  0.0351680443F,
      -0.0360950902F,   0.0736809447F,    0.0255722217F,    0.0486051068F,
      -0.0260426402F,   -0.029233573F,    -0.00466763508F,  -0.00184622F,
      -0.0140317762F,   0.0679096282F,    -0.00216284045F,  0.163375452F,
      -0.00892794598F,  0.0273119919F,    0.0117469151F,    0.0631673709F,
      -0.00881080888F,  -0.000818065833F, -0.0560691878F,   -0.0106426692F,
      0.113866642F,     -0.0202239752F,   0.0169721786F,    0.0424020439F,
      -0.0297096558F,   -0.0268071201F,   0.0189807583F,    0.0251221806F,
      0.0545623824F,    0.0144772818F,    -0.0127108656F,   -0.0495901F,
      -0.0153823858F,   0.0067509287F,    -0.0427157432F,   0.0237624962F,
      -0.0192871019F,   -0.0268861912F,   -0.0286210869F,   -0.00706223305F,
      0.022736581F,     -0.0119580161F,   -0.0588420704F,   0.0194762591F,
      -0.0409000441F,   0.0487214513F,    -0.0208989568F,   0.0284583662F,
      0.0192512702F,    0.00188271527F,   -0.0100740837F,   -0.00720998133F,
      -0.0117190992F,   -0.0501889177F,   0.0069696093F,    0.0274151824F,
      -0.0203812756F,   -0.0127046546F,   -0.0205196049F,   -0.0215400904F,
      0.0254759267F,    -0.0398181342F,   -0.00731122773F,  0.00653083064F,
      -0.00858307537F,  0.058844205F,     -0.0463381447F,   -0.0163259096F,
      0.0387271047F,    0.00398180587F,   0.057373967F,     -0.0117671546F,
      -0.00293431105F,  -0.0140656168F,   0.00237639388F,   0.000549427234F,
      0.0287136324F,    0.125932768F,     -0.0142554538F,   -0.00738134F,
      0.0379263796F,    0.0111304689F,    -0.0115914289F,   -0.0396976024F,
      0.0550646484F,    -0.00599859655F,  0.0146794962F,    0.054516837F,
      -0.017064007F,    0.0577590913F,    -0.017774513F,    -0.0600957386F,
      -0.0261919256F,   0.016812481F,     0.0384257846F,    0.0512373969F,
      -0.00173328922F,  0.00472813565F,   0.0400664061F,    -0.0119352778F,
      -0.0223338176F,   -0.0134936543F,   0.0136331394F,    -0.0246505458F,
      -0.00900732074F,  -0.0319334306F,   0.0257482845F,    -0.0117784925F,
      0.00988988392F,   0.061685238F,     -0.00774465827F,  0.0204366967F,
      -0.0119807413F,   0.0593180247F,    -0.00661149807F,  0.00846510567F,
      -0.0306866374F,   0.00942573417F,   -0.0116086416F,   -0.0187553857F,
      -0.0118677747F,   -0.0127623994F,   -0.0351831F,      -0.0101494864F,
      -0.00769943604F,  -0.0133481855F,   -0.0141632995F,   -0.0126417046F,
      0.0299653672F,    -0.0126245525F,   -0.000933249481F, 0.0295579825F,
      0.0414028428F,    0.0272044726F,    -0.014282261F,    -0.0113403657F,
      -0.00770430081F,  -0.0102131087F,   -0.0272974502F,   -0.00897747837F,
      -0.0412685126F,   0.0197970867F,    0.029543113F,     -0.00645290129F,
      0.0194625854F,    -0.0123593481F,   -0.00611265469F,  0.0220923051F,
      -0.00779804913F,  0.0126151294F,    -0.0145832654F,   -0.0108983452F,
      -0.0449628271F,   0.0369585864F,    0.00786517281F,   -0.0123957815F,
      -0.023220228F,    -0.0286995564F,   -0.00972666405F,  -0.0110067213F,
      0.0422746353F,    0.0311530232F,    -0.00975717418F,  0.019754732F,
      0.00827631F,      -0.0343109816F,   0.0240775F,       0.0366068184F,
      0.0210962929F,    0.0301090591F,    0.0399036147F,    -0.0212250724F,
      0.000330805371F,  0.0729932114F,    0.0141046513F,    0.0254366F,
      -0.00488362834F,  0.0087024048F,    0.0509760268F,    0.0362745486F,
      -0.0495896786F,   0.0565815903F,    -0.0442158766F,   0.00808798335F,
      -0.0393603F,      -0.036075566F,    -0.00607850449F,  -0.0120146144F,
      0.0471201241F,    0.00302473269F,   -0.0218002517F,   0.0465840548F,
      0.0234896317F,    -0.0173309129F,   -0.00590145495F,  0.0622944087F,
      0.0455245674F,    -0.0063608177F,   -0.0160632655F,   -0.0412878878F,
      0.0103504667F,    0.0471799187F,    -0.0172474682F,   0.00063134497F,
      -0.0136223929F,   -0.00609091949F,  -0.0380159F,      -0.045444604F,
      -0.00884261075F,  0.0131968101F,    -0.01059468F,     -0.0383919477F,
      -0.0104148751F,   -0.00853923429F,  -0.0131146628F,   0.00648287823F,
      -0.019313952F,    0.0169557706F,    -0.0380561575F,   -0.010188031F,
      -0.00809758529F,  -0.0164730959F,   -0.019882435F,    -0.0162401926F,
      -0.0407199524F,   -0.0134023372F,   -0.0179344229F,   -0.0245225504F,
      -0.0102323741F,   -0.0151045369F,   -0.0315599F,      0.00973508228F,
      -0.0416062884F,   -0.0145393154F,   -0.00625571236F,  -0.02380866F,
      -0.00812988542F,  0.0028607524F,    -0.00924598798F,  0.0513422228F,
      -0.015740823F,    -0.021860024F,    0.0369562805F,    -0.0140366983F,
      0.0449056774F,    -0.0208698604F,   -0.026139304F,    0.00631059473F,
      0.0277055707F,    -0.0184402484F,   0.0441544056F,    -0.0333084837F,
      0.00298322621F,   -0.00687530497F,  -0.0174906943F,   0.0888282433F,
      -0.00838752743F,  -0.00914822146F,  -0.0096341325F,   -0.0122102173F,
      -0.0218366701F,   -0.000882165099F, 0.0201764926F,    -0.0122620277F,
      0.00368884346F,   0.0359605663F,    0.00319235399F,   0.044247631F,
      -0.0126071498F,   -0.00655898592F,  0.0349305272F,    -0.0167546049F,
      -0.0168814529F,   -0.00425646035F,  -0.00622038F,     -0.035559494F,
      -0.0332097895F,   0.0558079481F,    0.00288939406F,   -0.000413430826F,
      -0.0115131754F,   -0.0288572367F,   -0.0435183495F,   -0.00546702323F,
      -0.0088474378F,   -0.0149039039F,   0.053301841F,     -0.0473271869F,
      -0.0458514914F,   -0.000498051289F, -0.0284163617F,   -0.00948978588F,
      -0.0597204752F,   0.0113366814F,    -0.0114882495F,   0.0198812149F,
      0.0253671929F,    0.0291437823F,    -0.00375973899F,  0.0201071985F,
      0.00369841466F,   0.00905094296F,   -2.74412268E-5F,  0.0106736207F,
      -0.0294993091F,   0.0151646975F,    0.0109702945F,    0.0334350318F,
      -0.0158781F,      -0.0241319221F,   -0.0374051183F,   0.0538242199F,
      0.0284259357F,    0.00501875626F,   0.035221763F,     -0.0318497159F,
      -0.00714289024F,  0.0736915469F,    -0.0203269944F,   0.0398160033F,
      -0.0093785543F,   -0.000446256832F, 0.0585002452F,    -0.0251353588F,
      -0.0143840602F,   -0.0500991791F,   0.0457313955F,    0.0182122402F,
      0.00395075046F,   -0.0243734196F,   -0.0525542051F,   -0.0166750662F,
      0.0387767069F,    0.0300821178F,    0.00444802223F,   -0.00979807787F,
      -0.0186233427F,   0.0248044971F,    0.00413235463F,   -0.0543601178F,
      0.0545087084F,    0.0549846292F,    -0.011809892F,    -0.00815200526F,
      -0.0206679404F,   0.0160458535F,    0.0119403573F,    0.00935169775F,
      -0.0269433167F,   -0.00980866887F,  -0.0400355309F,   0.0441334695F,
      -0.0500081442F,   0.00173301611F,   -0.0157243405F,   0.0400859639F,
      -0.0117588574F,   -0.0117031503F,   0.0325591192F,    -0.00777006615F,
      -0.00920082815F,  -0.0122254901F,   -0.0112433033F,   -0.0174921416F,
      -0.0126933102F,   -0.0369930752F,   0.033077281F,     0.0366230942F,
      -0.00726767862F,  0.0146712949F,    -0.0133106476F,   0.0212733243F,
      0.0432893895F,    0.0897009671F,    -0.0406007171F,   0.0227166265F,
      -0.00198934879F,  0.0338462107F,    -0.0103985174F,   0.0287891906F,
      0.00661861897F,   -0.0217898376F,   -0.00895878486F,  0.033601366F,
      -0.0235373713F,   -0.0244303569F,   0.0409624055F,    0.00962335523F,
      -0.0122288316F,   0.0336661264F,    0.00545696635F,   0.0282370914F,
      0.0173197649F,    0.014520769F,     0.0125492206F,    -0.0141924294F,
      0.0193246529F,    -0.00103180297F,  -0.0114114415F,   0.0368497111F,
      0.0397175F,       0.0359883904F,    -0.0540497378F,   -0.0276872572F,
      -0.0265373811F,   0.0201100111F,    -0.0121215554F,   -0.0100929644F,
      0.0179163348F,    -0.0299593676F,   -0.0102109443F,   0.0146176862F,
      -0.0219398551F,   -0.0311114118F,   0.0353755206F,    0.0430780873F,
      -0.00273062987F,  -0.00691262539F,  0.00666781748F,   0.0154537363F,
      0.0360285267F,    -0.0771250352F,   0.0560412034F,    -8.96748097E-5F,
      -0.00427107094F,  0.0148245245F,    0.0305863339F,    0.00368237938F,
      0.0111178104F,    0.00780211156F,   -2.73557689E-5F,  0.0467184708F,
      -0.0262522399F,   -0.0250487607F,   -0.112734303F,    0.0357375257F,
      0.0185484719F,    0.00152931176F,   -0.0374227613F,   -5.67008E-6F,
      0.0247160718F,    -1.40155671E-5F,  -0.0130443731F,   -0.0126061467F,
      -0.0376788862F,   0.00464565912F,   0.0219239835F,    0.0389339663F,
      -0.024960652F,    0.00118955807F,   -9.85605493E-6F,  1.6205604E-7F,
      5.46223328E-6F,   0.00422799308F,   -0.00566248596F,  0.0111728525F,
      -0.00745809218F,  3.25522524E-6F,   0.00635543838F,   -0.0094936816F,
      0.00853728876F,   0.0676630139F,    -2.15902901E-6F,  -0.00554627925F,
      2.74650097E-6F,   0.00515242F,      -0.0525629856F,   0.000905183377F,
      -0.0482864417F,   0.00397875113F,   -0.0393307693F,   -3.97248914E-6F,
      0.0113205202F,    -8.55566E-7F,     0.0118111661F,    -0.00314722257F,
      0.0202181414F,    -0.0135244094F,   -6.06253843E-6F,  0.00867311936F,
      0.019633159F,     0.0244696774F,    -9.91218E-6F,     -0.0309447721F,
      0.0276397448F,    0.0213478319F,    0.0355545208F,    0.00464404374F,
      0.00162049441F,   0.00266026543F,   -0.00562869059F,  0.0184273757F,
      0.00560498284F,   -0.00920083188F,  0.000483974232F,  0.0136149321F,
      0.00164201786F,   0.0225164574F,    -0.0336606614F,   9.12553605E-6F,
      -0.0106299678F,   -0.00807812624F,  -0.0201971028F,   0.0071536405F,
      -0.0319058299F,   0.018974334F,     -0.00130808819F,  1.33868843E-5F,
      0.0331715085F,    0.0158894F,       9.77921388E-8F,   0.00288276747F,
      4.53629837E-5F,   -0.0131841265F,   -1.1176553E-6F,   0.0294488538F,
      -0.00522716343F,  -0.0289187841F,   8.07626748E-6F,   0.031993553F,
      -5.37524102E-6F,  6.33560785E-6F,   -0.0514462776F,   0.040845748F,
      2.7360702E-6F,    -0.021193495F,    -1.46239836E-6F,  -8.09158155E-6F,
      0.0505549572F,    2.55218606E-6F,   -0.0199588016F,   -0.0384861939F,
      -0.0526392795F,   -0.0284383185F,   -0.0201580022F,   0.00568955624F,
      -3.33757862E-6F,  3.51705739E-6F,   -0.00981574319F,  2.3002005E-5F,
      0.0195248984F,    -0.00514646294F,  -0.0607740171F,   6.03234821E-6F,
      0.0451088697F,    3.16943187E-5F,   2.29615107E-5F,   -0.00153890043F,
      1.25327733E-5F,   0.0635715276F,    3.84991199E-6F,   -1.18916696E-5F,
      0.0178663731F,    0.0205538068F,    0.0152553357F,    5.34363699E-6F,
      0.00162311667F,   -0.0266657304F,   0.00840959325F,   5.5584278E-6F,
      0.000859094434F,  -0.0223610029F,   -8.42248664E-6F,  0.0354331173F,
      -0.0361383446F,   0.0119316271F,    0.047667753F,     0.0171613339F,
      -0.00669854134F,  0.00502124336F,   0.0108426446F,    -0.0450960547F,
      0.0229623094F,    -0.00471480237F,  -0.158209786F,    -0.0331556909F,
      3.86167558E-5F,   -0.000692564761F, -0.00219009793F,  0.0260297526F,
      0.0194799956F,    0.0338216387F,    -0.0259746332F,   -0.0517628528F,
      -0.0521172956F,   -0.0134450831F,   0.0224878453F,    -0.0154694356F,
      0.116307728F,     -0.0274389014F,   -0.0213363543F,   -0.0429947674F,
      -0.014280621F,    0.00160414667F,   0.00804404356F,   -0.0277469289F,
      -0.0419935696F,   -0.00704900315F,  -0.00207816064F,  0.0158222411F,
      0.0134883896F,    -0.00826013F,     -0.0120557928F,   0.0196600817F,
      2.28317072E-6F,   -0.00933769811F,  -0.0470930822F,   0.0109239863F,
      0.00845124852F,   0.00282454F,      -9.11637926E-6F,  -0.0217621475F,
      -1.54542377E-5F,  9.84946269E-7F,   -0.00189035176F,  0.00228920532F,
      -0.044356171F,    -0.0146326749F,   0.0227694958F,    1.46422844E-5F,
      1.75984019E-6F,   -0.0830963403F,   -0.0316152722F,   -0.0247525144F,
      0.0328471512F,    -3.46440211E-5F,  -0.0218306668F,   -0.00330043095F,
      0.0114671346F,    -0.0158963799F,   0.0132795852F,    -0.00383587228F,
      0.00788557716F,   0.000648478512F,  -0.000116624477F, -0.0106729483F,
      5.51638504E-6F,   -0.0302392934F,   -4.21885545E-7F,  -0.0215420704F,
      0.000567197276F,  -0.0076769162F,   0.0116287712F,    -1.41954638E-6F,
      0.014468058F,     0.00128872937F,   0.0155418599F,    0.0183614921F,
      0.0362356566F,    -0.00852539949F,  0.0234165546F,    -0.0297201388F,
      -0.010796641F,    0.00553941308F,   -1.02980612E-5F,  0.0317900665F,
      2.07212452E-5F,   -1.23981727E-5F,  -2.6729308E-6F,   0.00389727787F,
      -0.0231795423F,   -0.000201173549F, 0.0364156514F,    6.39446953E-5F,
      0.0173909459F,    -0.00390233542F,  -0.0403195582F,   -0.00730873086F,
      0.000384315557F,  -2.51157257E-6F,  0.0101751843F,    0.000893422635F,
      0.000413954811F,  -1.2445691E-5F,   -2.31418585E-6F,  -0.0328991897F,
      0.0162784811F,    0.00204159925F,   0.0208615717F,    -0.0457052775F,
      7.87303088E-6F,   -0.0314050168F,   0.0583909824F,    0.0461695567F,
      -0.000347957364F, 5.21870497E-5F,   -0.00665609F,     0.0216142349F,
      -0.0210198723F,   0.0511726215F,    0.0039615524F,    2.13096769E-7F,
      0.0545982681F,    0.000660198799F,  0.0174514893F,    0.0325322077F,
      -0.0024252627F,   0.00781360175F,   0.000199808681F,  0.0209000781F,
      -0.024328433F,    -0.0376033559F,   -0.0430424735F,   -0.0510734096F,
      0.0269728191F,    -0.00347455288F,  0.0128451511F,    0.0236979146F,
      2.6185111E-5F,    -0.00500857038F,  -0.049663078F,    -0.0338608623F,
      0.00786058F,      0.0209381F,       0.0423145778F,    0.0289212521F,
      3.00655688E-6F,   0.0317455716F,    0.00105007901F,   -0.0539662242F,
      0.0445026606F,    -0.0346788429F,   0.0161745511F,    0.00667760102F,
      0.0220162403F,    -0.0322795697F,   -0.0617779493F,   -0.0127842007F,
      0.0297541395F,    0.0152418511F,    0.0279539023F,    -0.0842928663F,
      0.033503F,        -0.0290803257F,   0.000908391958F,  -5.73966736E-6F,
      -0.0434616469F,   0.0349428616F,    0.0211322159F,    0.0237767827F,
      0.00517949369F,   -0.0724968F,      0.0152982222F,    9.78513071E-6F,
      0.0078561157F,    0.0260787085F,    -0.0424907319F,   -0.0073898253F,
      -0.0419329815F,   -7.14498747E-5F,  0.0318792462F,    -0.000787976664F,
      -0.0358980745F,   0.0260961857F,    -0.0010436239F,   -0.0776411891F,
      0.023997765F,     -0.000115794996F, 0.0276843607F,    2.63691106E-7F,
      1.95750522E-6F,   -0.000416743365F, 6.69809936E-7F,   0.00104295113F,
      -0.000892317912F, 0.0162713714F,    0.0368907042F,    0.0295835156F,
      -1.30934461E-6F,  -0.0553404763F,   5.01135728E-5F,   -0.000513643958F,
      0.0276200827F,    -0.0661517158F,   -0.0217658337F,   0.0315607823F,
      -0.0151797934F,   -0.00582548557F,  -1.05729205E-5F,  0.00415755715F,
      0.0214737728F,    0.016521804F,     0.0181548242F,    0.0185633637F,
      -0.073886618F,    -0.0201359615F,   -0.0426131263F,   0.00742587494F,
      2.24754112E-5F,   0.0396182425F,    0.0189616829F,    -0.041569788F,
      -0.0352662839F,   -0.0177086499F,   0.0111122383F,    0.0182185955F,
      0.0394889154F,    -0.0351139456F,   -8.33420563E-6F,  -0.0176399518F,
      0.0112165F,       0.00351547333F,   0.0190319605F,    -0.0069454927F,
      -0.00183206541F,  0.00362712983F,   0.000181515337F,  7.56416398E-7F,
      0.0108727822F,    0.00354344514F,   2.18416449E-6F,   0.0303544179F,
      0.000904534652F,  -0.0456424244F,   0.0231431965F,    0.00951726828F,
      0.0172368735F,    3.25674E-6F,      0.0173720084F,    -0.0462327376F,
      -0.0156111307F,   0.0425605886F,    -0.0553913973F,   -0.0136273196F,
      0.0147248171F,    0.0273493417F,    0.0470154248F,    -0.0190013554F,
      -0.0170768276F,   -0.0269547626F,   -0.0124709522F,   -0.0311127454F,
      -0.00202795211F,  0.080832012F,     0.0758664533F,    0.0203326587F,
      -0.014185397F,    -0.081000492F,    0.0331892F,       -0.00693821767F,
      0.0453115292F,    -0.0105975391F,   0.032321129F,     -0.00877525564F,
      0.0561634041F,    -0.0251771193F,   -0.00751876645F,  0.00012830575F,
      -0.0236935969F,   0.0102076475F,    -0.00993688311F,  -0.00710086245F,
      -0.011701691F,    0.0102298902F,    0.0557322614F,    -0.00459421799F,
      -0.0143428901F,   -0.012562003F,    -0.0231925137F,   0.0221381783F,
      -0.0321022794F,   0.042856887F,     -0.00704805786F,  -0.0187671352F,
      -0.00852184184F,  0.0278486703F,    0.088831529F,     -0.0175734702F,
      0.0556754246F,    0.000237101194F,  0.00164780219F,   -0.0112850703F,
      0.012682871F,     -0.0142585179F,   0.0430906F,       0.0429851301F,
      -0.00551070599F,  0.0758550912F,    -0.0123606957F,   -0.00881892815F,
      0.0518393815F,    -0.0226569045F,   -0.011227686F,    0.0282959547F,
      0.0463788062F,    0.0164454747F,    -0.0334104374F,   -0.0697701424F,
      -0.00592960184F,  -0.0335532352F,   -0.0205935631F,   0.00834812876F,
      -0.00586732151F,  -0.0275980774F,   -0.0070243068F,   0.037821155F,
      -0.0192084424F,   -0.00584543683F,  0.0232851245F,    -0.0118857762F,
      -0.030023573F,    0.0301384386F,    -0.0425682403F,   -0.0101794712F,
      -0.0085963F,      -0.0227692F,      -0.0355834588F,   -0.012286731F,
      -0.0296780858F,   -0.0384082161F,   -0.00755566033F,  -0.027369855F,
      -0.0125084044F,   0.0140278535F,    -0.00649056071F,  0.0227792505F,
      -0.0632398278F,   0.00523147872F,   -0.0116778994F,   0.0902435258F,
      -0.0118648261F,   -0.0128689036F,   0.0326019228F,    0.0208492503F,
      -0.00756317284F,  -0.00657383306F,  -0.0103989057F,   -0.0123049179F,
      0.0146141881F,    -0.0124131404F,   0.0350446254F,    0.00173010491F,
      0.0322238654F,    0.0213715807F,    0.0131003521F,    -0.0145171788F,
      -0.00749858702F,  -0.00998834427F,  -0.0350201391F,   -0.00897678F,
      0.0426237099F,    0.0122101894F,    0.00894728117F,   -0.00637844112F,
      0.0504515767F,    -0.0121561121F,   -0.00603850465F,  0.0162973683F,
      -0.00765452813F,  -0.00258681434F,  -0.0143804308F,   -0.0107976627F,
      0.00221593399F,   0.0398837365F,    -0.0367087089F,   -0.0121738166F,
      -0.023351F,       0.0590623952F,    0.00725125708F,   -0.010893615F,
      -0.0723854452F,   -0.00264844857F,  -0.00954887178F,  0.0270966906F,
      -0.00713901781F,  0.0310180262F,    0.0144216474F,    0.000868906151F,
      0.0433442928F,    -0.0241962299F,   -0.0291212946F,   0.0101559386F,
      0.0414487794F,    -0.0447329767F,   0.0247579925F,    -0.0118945008F,
      -0.00486247335F,  -0.0338641442F,   0.0497189052F,    0.00780026289F,
      0.0145804333F,    0.0614235625F,    0.0246847495F,    0.031079419F,
      0.0601593181F,    0.00672119F,      -0.00374318822F,  0.0102170818F,
      0.0230937134F,    -0.0111713745F,   -0.0192703586F,   0.0249039363F,
      -0.0346930958F,   -0.022448495F,    -0.0147763612F,   -0.02348575F,
      -0.0322924666F,   -0.0284307F,      -0.0120152431F,   0.0268995129F,
      0.0224513095F,    -0.0197315924F,   -0.0250773448F,   -0.020020552F,
      -0.0136226695F,   0.0317632854F,    0.0307215154F,    0.0141069954F,
      0.0103056794F,    -0.025522761F,    -0.0103844274F,   -0.00724181626F,
      -0.0102583803F,   -0.00847307127F,  -0.00208369247F,  0.00783194136F,
      0.0502345785F,    -0.0349734537F,   0.0221364982F,    -0.0100433994F,
      -0.00785746891F,  0.0510026291F,    -0.0364645943F,   0.0100373672F,
      0.0350060202F,    -0.0133564416F,   0.00577700092F,   -0.0162685011F,
      -0.00100587902F,  0.00346232532F,   0.0497298837F,    0.0302012339F,
      -0.0414747F,      -0.0294686779F,   -0.00619876571F,  -0.0228029471F,
      -0.00792440306F,  -0.00374667463F,  -0.00913170539F,  -0.00268723816F,
      -0.0183485132F,   -0.0218786988F,   -0.0216385759F,   -0.0106378207F,
      0.0413437076F,    -0.0174927879F,   -0.0165753663F,   0.0243116356F,
      0.0582478F,       -0.00800731406F,  -0.00183338847F,  0.0183069538F,
      -0.0274371579F,   -0.00489225797F,  -0.00822487846F,  0.0127068507F,
      -0.00828675739F,  -0.00900674798F,  -0.00950903725F,  -0.0141119091F,
      -0.0152381966F,   0.0323082916F,    0.0228408501F,    -0.0112049952F,
      0.0199420284F,    0.0404392891F,    0.00183305133F,   0.0359234661F,
      -0.0140012689F,   -0.00647484884F,  -0.0376299545F,   -0.0135745434F,
      -0.0211845804F,   -0.00416069292F,  -0.00607034564F,  0.0220565852F,
      0.0445080101F,    0.0690112486F,    0.013556188F,     -0.0329088084F,
      -0.0113320062F,   0.0075222184F,    0.00661419285F,   0.0503773019F,
      -0.054654751F,    -0.0137153836F,   0.0140015269F,    0.0529279485F,
      -0.00726099825F,  0.0247083269F,    -0.034146715F,    -0.00915639289F,
      0.00123850419F,   -0.0430198F,      0.0189846307F,    0.0300564319F,
      0.00584081141F,   -0.0349436104F,   -0.0187453255F,   0.0307678208F,
      -0.0192992557F,   -0.0119370241F,   0.00362689956F,   0.00959064346F,
      0.0250909012F,    -0.0102721136F,   0.0245690439F,    -0.0391074903F,
      -0.0129292272F,   -0.027893519F,    0.0518823378F,    0.0200854447F,
      -0.0257263519F,   0.0225710403F,    0.0061801332F,    0.0771250278F,
      -0.00696410472F,  0.077955775F,     -0.00801561121F,  -0.0141287828F,
      -0.103187047F,    -0.0122100152F,   -0.016549034F,    0.0817610547F,
      0.0118320417F,    0.0376483127F,    0.0558114201F,    0.0259414874F,
      0.102897324F,     0.0201720688F,    0.0542902537F,    0.0187164489F,
      0.022566339F,     0.029099308F,     -0.0194486752F,   -0.00964343082F,
      0.0299061071F,    0.0543889292F,    -0.035020072F,    0.0185216349F,
      0.0447166078F,    -0.0302100871F,   -0.0142882206F,   -0.00802076049F,
      0.0146637456F,    -0.00559721049F,  0.00405133329F,   -0.0496227741F,
      0.0577043369F,    -0.00954372808F,  0.022233041F,     -0.0728721172F,
      0.0453911275F,    -0.00766037963F,  -0.0267032757F,   -0.0118980231F,
      0.043772608F,     -0.0294570904F,   0.0389488935F,    -0.0076763304F,
      -0.00923639163F,  -0.011495742F,    -0.0110838693F,   -0.0171143301F,
      -0.0140931448F,   -0.0110603757F,   -0.0507691316F,   0.00805583876F,
      -0.00698314421F,  -0.0716523752F,   -0.0129872719F,   -0.0314447582F,
      0.0670274496F,    -0.053295061F,    0.0280280281F,    -0.0218421724F,
      -0.00341394939F,  -0.0327938125F,   -0.0102081718F,   -0.0178149119F,
      -0.0248908438F,   0.0209904239F,    -0.0235965531F,   -0.00420278078F,
      -0.0101436079F,   0.0272721313F,    0.00342948828F,   -0.0192364361F,
      -0.0121224653F,   -0.0194059797F,   0.0303228237F,    -0.0302049983F,
      -0.0220972225F,   -0.0080778F,      -0.00935157202F,  -0.0177143943F,
      0.0170801692F,    -0.00185237452F,  -0.0112389205F,   0.0346277915F,
      -0.0359713957F,   -0.0157881547F,   0.0613877624F,    0.0246337708F,
      -0.0179908462F,   -0.0302534588F,   -0.0123571064F,   -0.0100309765F,
      -0.0112519162F,   0.0532266572F,    -0.00997944549F,  -0.0166908056F,
      -0.0191064533F,   0.0401815F,       0.00765112275F,   -0.037145827F,
      0.0127267744F,    -0.00678184116F,  0.0463400483F,    -0.00085990876F,
      -0.00597392535F,  -0.0160345845F,   0.0390500911F,    -0.0474681705F};
  static const float inputStateWeights[2000] = {
      -0.017228296F,    -0.0143124517F,   -0.0759488866F,   0.00335895829F,
      -0.00266829901F,  0.0131151369F,    0.000516367494F,  -0.507948279F,
      -0.623748839F,    0.191770479F,     0.00534543348F,   2.09699488F,
      0.00105590175F,   0.00553190568F,   -0.000235783169F, -6.74030671E-5F,
      -0.0052968259F,   0.000116125004F,  0.0147402966F,    2.03650713F,
      -1.01230872F,     -0.00475941785F,  -0.29862F,        0.00431194622F,
      -0.655097485F,    -1.66570413F,     -2.15235668E-5F,  -2.81842531E-5F,
      -2.3988694E-5F,   -1.08896768F,     0.0120904827F,    1.90523088F,
      0.00228068396F,   5.03349156E-7F,   -0.00374165294F,  0.0713002533F,
      -0.00306271249F,  -1.4130702F,      7.83469659E-5F,   -0.00272808457F,
      -0.000140273172F, 0.00208311947F,   0.00357825356F,   6.00714775E-5F,
      -0.0267250091F,   -0.00474208826F,  -2.35380411F,     -0.000184846285F,
      0.874016047F,     0.000205243705F,  -2.52581573F,     0.00811112951F,
      0.0017076045F,    0.0209885389F,    -0.000252033147F, 1.19334519F,
      0.00644337479F,   -0.00567982066F,  -0.000137848707F, 0.0392979123F,
      0.00592508074F,   -0.0145633882F,   -0.0051028328F,   0.0199772045F,
      -0.000144639809F, -0.000401583238F, -0.00784409326F,  -0.0258164946F,
      0.0126438672F,    -0.0035437739F,   -0.159942284F,    -1.32349169F,
      -0.00375185022F,  -2.10819793F,     -0.000924985448F, 0.000158819093F,
      0.0623984039F,    -1.94678855F,     0.00819524936F,   0.0217606388F,
      -0.93029362F,     -0.763839245F,    -0.0455162F,      -1.67304588E-5F,
      0.00530210882F,   0.00631910237F,   2.05136821E-5F,   0.0035460256F,
      -0.000636879879F, -0.0680464804F,   -5.32904887E-5F,  -0.0510464385F,
      -0.0482371636F,   0.0206578895F,    0.000220101923F,  -0.0110263508F,
      -0.000220307571F, 2.39901401E-5F,   -0.0117228106F,   0.105953515F,
      -7.2187795E-5F,   0.017194353F,     -0.000486750534F, -0.000329658331F,
      1.61233521F,      -0.000159048577F, 1.67241716F,      1.23831856F,
      -0.00281172781F,  -0.0341398567F,   -0.0245793648F,   -0.00466423528F,
      7.40626201E-5F,   -5.4683187E-5F,   -0.0347525515F,   0.000115926981F,
      -0.00334989349F,  -0.00298411492F,  -0.224196434F,    6.6981811E-5F,
      0.185593486F,     0.000166725295F,  -0.000149491156F, -0.0254324283F,
      -0.000216325454F, 2.52832127F,      -0.000261592475F, 0.000218405534F,
      0.00162665872F,   -0.00279652164F,  0.00175118248F,   0.000103268045F,
      -0.00134757371F,  0.00422910368F,   -0.00315819192F,  0.000159220086F,
      -0.00383203034F,  -0.00195172918F,  6.19137863E-5F,   -0.133732706F,
      -0.00450676121F,  -0.00610564416F,  -0.351620317F,    -0.00247935439F,
      -0.00321634975F,  -0.00168221444F,  -0.00234437687F,  0.218633696F,
      -0.0829724818F,   -0.0332540758F,   0.13875553F,      -0.00373775326F,
      0.000623723725F,  0.00134498475F,   2.37912083F,      -0.00285607344F,
      -0.0278983153F,   -0.00656310935F,  0.0252091661F,    -0.00305998838F,
      0.877263963F,     0.00994153228F,   1.62708628F,      0.962477803F,
      1.39531457F,      -0.0901392F,      0.00131375063F,   0.317694873F,
      0.0081328582F,    -0.00348298415F,  -1.37284923F,     -0.012922165F,
      0.00746707758F,   0.00865637F,      0.0021653818F,    -0.0230510663F,
      0.000782404852F,  0.00189877395F,   -0.00576830376F,  -0.0121015674F,
      -0.000173651846F, -0.0141737647F,   0.00185747759F,   0.0110963313F,
      -0.0103130303F,   0.00359406718F,   0.000175043373F,  -0.0157871917F,
      -0.000485562166F, -4.41690245E-5F,  -2.59075856F,     -1.67899394F,
      0.002555124F,     -0.00331473816F,  0.00278062862F,   0.000141679498F,
      5.4514E-5F,       -0.784875274F,    -0.239146039F,    1.71431482F,
      -0.00690249167F,  -0.000293180696F, 0.0271768589F,    0.00670426199F,
      0.00305488217F,   0.00437412644F,   0.0107186399F,    -2.11166191F,
      0.0209269859F,    -0.00285358843F,  -0.000918085279F, -0.0066573564F,
      3.87763794E-5F,   -1.54785168F,     -0.000239897025F, -0.0334522203F,
      -0.000214160289F, 0.00543743186F,   0.0042245849F,    -3.14620847E-5F,
      0.00409685401F,   -0.0020226636F,   0.00762975914F,   -1.88489151F,
      -2.42906094F,     0.0215893406F,    0.00233548344F,   -0.0103032524F,
      0.0163176917F,    -2.50905752F,     -3.87109612E-5F,  0.00263241539F,
      1.4389052E-5F,    -1.23740519E-5F,  -3.50440932E-5F,  0.0044675F,
      0.0431334414F,    1.81689095F,      0.00248331437F,   -0.000345927925F,
      1.69129813F,      -1.16083038F,     -0.0793492272F,   0.0590611026F,
      0.0002475121F,    8.24332819E-5F,   0.00297749019F,   0.00169487484F,
      -0.00167391764F,  8.03580333E-5F,   5.51805861E-5F,   0.00344833F,
      0.00687671779F,   0.169109285F,     -0.00363152521F,  0.914873064F,
      7.38308954E-5F,   -0.00304671563F,  0.278661072F,     -2.14854884F,
      -0.0100161452F,   0.000135152193F,  0.00195150962F,   0.00805449486F,
      -0.0202950016F,   3.37787223F,      -0.00163447869F,  3.90694731E-5F,
      1.42155111F,      -0.00033249415F,  0.00650779344F,   -1.29299259F,
      -0.000565786613F, -0.00243546348F,  -0.00218917639F,  -2.01454473F,
      0.0167304575F,    0.179776564F,     0.000984585145F,  0.0051656235F,
      0.82019639F,      -2.2454927F,      0.00626860699F,   -1.7811799F,
      9.49459354E-6F,   -0.0181649551F,   -0.0018586379F,   0.0783129F,
      -0.00499946065F,  0.0308536477F,    -0.00527456962F,  0.00847834442F,
      4.52436E-5F,      -0.00983515754F,  -0.00138311833F,  -0.00209798384F,
      1.09270239F,      -0.0256292485F,   -0.00209107157F,  -0.0139098372F,
      0.174598858F,     0.000795561413F,  0.00721774437F,   0.638733387F,
      -0.0119503411F,   -0.013887166F,    0.00213547656F,   -0.0031804454F,
      -0.0023113112F,   -0.00546579435F,  0.00190119515F,   -0.000154437672F,
      -0.00236384687F,  1.7431289F,       2.37425852F,      -0.063463293F,
      -1.71058214F,     0.00629329355F,   0.00769945141F,   -9.87343519E-5F,
      -0.00380802341F,  0.000631839328F,  1.16322F,         -0.00547473738F,
      -0.00180583063F,  -0.000392755144F, -0.195263237F,    0.000610650459F,
      -0.000813533785F, 2.47123837F,      -0.00927603152F,  0.23621507F,
      0.962452173F,     -0.00414617499F,  -0.00107613485F,  -4.35620568E-5F,
      -2.20294696E-5F,  -0.000252694F,    0.000285812508F,  -0.000270089979F,
      -0.000359898084F, -0.837606192F,    0.228668839F,     0.00356120476F,
      -1.95714329E-5F,  -0.037628F,       -0.000107492931F, 0.00107714289F,
      -0.31182605F,     -0.400433898F,    0.0622330271F,    2.4689219F,
      -1.25725091F,     -0.000705090293F, -0.000100687059F, 0.000570617267F,
      0.104041852F,     0.00667336863F,   1.80001593F,      -0.00124655454F,
      -0.00302263559F,  4.51812193E-5F,   0.0729289949F,    -0.000441310229F,
      4.19342905E-5F,   0.000395378942F,  1.32307303F,      -0.212621599F,
      1.11891377F,      0.664476752F,     0.00287785102F,   1.82807386F,
      -1.79042745F,     0.00269145356F,   0.000181701413F,  -0.016813796F,
      0.00279162847F,   1.92069638F,      0.0172407459F,    0.0044814432F,
      0.00762482453F,   -0.00835646875F,  0.000372460694F,  4.83439471E-5F,
      0.00189414667F,   -0.000892372045F, -0.000118227923F, 2.22116852F,
      0.00215726951F,   0.576705515F,     -0.00233765342F,  0.0159768667F,
      0.0146938683F,    3.47700334E-5F,   0.00911216717F,   0.0138174286F,
      -1.78352106F,     -0.341430277F,    0.0362804644F,    0.000634006923F,
      0.0248695016F,    -0.0267957486F,   0.0428781435F,    0.0569010563F,
      -0.204359859F,    -0.00251360424F,  -4.20628858E-5F,  0.00604343973F,
      -0.000915386481F, -0.058218088F,    -0.0238785818F,   -0.00155777589F,
      0.148343861F,     0.0373887196F,    -0.0698045492F,   -1.63809855E-5F,
      -0.223404884F,    0.000318977545F,  0.00539386366F,   2.84853468E-5F,
      0.00268402183F,   0.179642662F,     0.0151351979F,    0.672430694F,
      0.0177666172F,    -0.00388243515F,  4.04312595E-6F,   5.20300528E-5F,
      -8.64713875E-5F,  0.0208145957F,    0.244430259F,     0.00620810827F,
      -0.0282164682F,   8.53637248E-5F,   -0.0477043726F,   -0.0255346112F,
      -0.245560169F,    0.000417877367F,  6.37557459E-5F,   0.0245101657F,
      -5.58431E-6F,     0.244046628F,     0.178259343F,     0.0208552517F,
      0.086651884F,     0.0613152348F,    -0.00405670889F,  -6.11218056E-5F,
      0.00502581568F,   6.9013804E-6F,    0.000999384F,     -0.0875847861F,
      0.305164248F,     -0.12050245F,     -5.06489505E-6F,  -0.00989371818F,
      -0.115327291F,    0.134460777F,     -5.35395E-5F,     -0.0170862041F,
      0.253723681F,     -0.0520788357F,   -0.894794464F,    -0.00282755727F,
      0.0604263768F,    -0.140678599F,    0.0265946053F,    -0.0460526682F,
      0.00289700041F,   0.0127952714F,    0.0228250585F,    0.0206002016F,
      0.0404672436F,    0.00232062023F,   -0.27950567F,     4.22912126E-6F,
      -0.00879349466F,  0.00160308194F,   -0.0572345741F,   -0.00653994177F,
      0.0133736255F,    0.00740323681F,   0.00526090758F,   -1.23838008E-5F,
      0.897564173F,     -0.124650471F,    -2.34083527E-5F,  0.0474699549F,
      -0.00384279573F,  -0.0388897F,      -5.41812806E-5F,  -0.0198358484F,
      0.0578777194F,    0.00416333601F,   1.6752987E-5F,    -0.231376991F,
      -5.25644646E-5F,  7.33611669E-5F,   0.00175188761F,   -0.00146101369F,
      -0.000125531093F, -0.0614668764F,   0.00467609102F,   -7.16199283E-5F,
      0.00845608953F,   1.46840202E-5F,   0.000999465585F,  0.00834896695F,
      -0.428908139F,    0.0788819268F,    -0.00143441895F,  0.0791680887F,
      7.16934665E-5F,   2.52155951E-5F,   0.00767501025F,   3.34269134E-6F,
      0.0190570131F,    0.145327032F,     -0.00421466F,     6.75825067E-5F,
      -0.0772615F,      2.05592314E-5F,   -0.000115912146F, -0.00531478086F,
      -2.73724581E-5F,  -0.000745619647F, 1.96437522E-5F,   6.38246056E-5F,
      0.0303054601F,    -0.475774169F,    0.542424798F,     9.52859864E-6F,
      -0.282568663F,    -0.0364260189F,   -0.0886083245F,   -2.3458233E-5F,
      -0.0285301805F,   -0.32590884F,     -0.000132048619F, -0.0103492634F,
      0.134463459F,     -0.0285627786F,   -0.0033652233F,   -0.422756732F,
      -0.338206291F,    0.174470842F,     -0.214724302F,    -0.0152442772F,
      0.0845963359F,    -0.000852037512F, 0.00302544562F,   -0.125015393F,
      0.000205559554F,  0.00713219726F,   -0.000562603818F, -0.468123317F,
      -0.007876263F,    -0.241378441F,    -0.0343863182F,   -0.379785776F,
      0.00630487688F,   -0.0153032178F,   -0.0110343F,      -0.019457655F,
      0.00279664295F,   -0.039082896F,    0.321198851F,     0.0105167199F,
      0.0796010792F,    0.0620658509F,    0.0143000083F,    0.0480823964F,
      0.193195686F,     -0.0104302568F,   -0.00766378315F,  -0.0611981452F,
      -0.0467785038F,   0.269287765F,     -0.0395631343F,   -0.00123931176F,
      5.36924199E-5F,   -0.0432540029F,   0.581393301F,     0.0197075531F,
      0.0488620624F,    0.00411896547F,   5.52297242E-5F,   -0.00183520326F,
      -0.000705966493F, -5.42796151E-5F,  -0.000175305351F, 0.00159419503F,
      0.251797199F,     -0.120511107F,    0.115116291F,     -3.00480406E-5F,
      -2.64907248E-5F,  0.0142534506F,    -0.0221358724F,   0.00566668948F,
      0.0926585F,       9.49079549E-5F,   0.00371898036F,   0.0761082917F,
      0.0797732472F,    0.089695476F,     0.0868666321F,    0.000558270374F,
      -0.00343465433F,  0.00378027186F,   -0.000207216639F, -0.0366178676F,
      -4.86922982E-5F,  -0.0080217775F,   2.59911576E-5F,   0.101427697F,
      0.0168342423F,    -0.116631486F,    0.186470136F,     -0.00189838756F,
      -0.146421969F,    0.0321134217F,    -0.13605465F,     -0.00222341553F,
      -0.00168396241F,  -0.0768081844F,   0.378313243F,     0.102829501F,
      -0.0597521551F,   0.000880475796F,  -2.94989695E-6F,  0.509539843F,
      -7.66310222E-6F,  6.94405608E-5F,   -7.35177673E-5F,  0.331015766F,
      0.00820127316F,   0.00476833107F,   0.122987345F,     0.00140956126F,
      -0.00280677644F,  -0.00816793926F,  0.0279990323F,    -0.0394929163F,
      0.00731733069F,   5.97881899E-6F,   0.256330729F,     -0.019806074F,
      -0.16105929F,     -7.36636321E-6F,  -3.40754777E-5F,  -0.034797471F,
      -0.209230095F,    -0.0664962083F,   -0.0453649946F,   -0.00087184069F,
      -2.64348128E-5F,  0.10908369F,      0.0246327184F,    -0.00626725797F,
      0.003836666F,     -3.51388371E-5F,  0.246459872F,     -0.037887685F,
      -0.00622788491F,  -0.00117332407F,  -0.258985579F,    2.37293916E-5F,
      0.00445833337F,   -0.00225205417F,  -0.0256240107F,   -0.00614486123F,
      -0.267540306F,    -0.293100148F,    0.0605867021F,    0.00202602101F,
      0.00282770721F,   0.00443569943F,   0.201414332F,     0.173346311F,
      -0.0143313622F,   0.000738740491F,  0.101145439F,     -0.0110587897F,
      -1.13503311E-5F,  0.00420414843F,   0.129508063F,     -0.114230417F,
      0.0865362659F,    -0.0570130758F,   -0.8909567F,      0.281938314F,
      7.75301305E-5F,   -0.262630522F,    0.00952391F,      -0.315809816F,
      -0.00232272409F,  -0.00560942199F,  -0.540085614F,    0.0325480253F,
      -0.084217146F,    0.288351089F,     0.0110478131F,    0.0100294007F,
      0.069848381F,     -0.0538392514F,   -0.166875467F,    0.0476853698F,
      -0.438585103F,    -0.0184500068F,   0.0345374569F,    1.42071531E-6F,
      -0.155703187F,    0.0107430462F,    -0.00269977027F,  0.0441819243F,
      -0.00406717323F,  -0.184636638F,    0.110751912F,     -7.11380344E-5F,
      -0.174307659F,    -0.294936538F,    -0.00632351777F,  -0.004526F,
      -0.0500969738F,   -0.000209798425F, 0.0885017F,       0.240001395F,
      0.0414791889F,    0.00209303945F,   0.00598016055F,   0.0828496441F,
      -0.00638929848F,  0.0744656324F,    -0.370273143F,    -1.76212288E-5F,
      2.60290071E-5F,   0.00475198F,      -4.73346772E-5F,  -0.00849548541F,
      -0.0206792671F,   -0.000184232864F, 0.00126441498F,   0.534666419F,
      1.44971307E-6F,   0.0392164402F,    -9.79027E-6F,     0.00569258F,
      -0.000170219952F, -0.0168234073F,   -0.0212854147F,   0.00235302723F,
      0.0110253012F,    0.174606875F,     -5.00470524E-5F,  0.0627790689F,
      0.0052469112F,    0.0898197293F,    -0.00308358739F,  -0.315932035F,
      0.0311087072F,    -0.0774056166F,   -0.0257371236F,   0.189412504F,
      8.36174513E-6F,   -0.252375364F,    0.0180475097F,    0.0322107F,
      6.91442547E-5F,   -0.0122940782F,   0.125659689F,     -0.0130471066F,
      -0.00100744725F,  0.355382591F,     4.8102429E-5F,    0.167370901F,
      0.243147194F,     0.000495972636F,  0.0269301571F,    -0.0166766215F,
      -0.0070443796F,   -0.0966518447F,   0.00681393873F,   -2.64300506E-5F,
      0.173964605F,     0.0609444976F,    4.12534682E-5F,   0.00257651974F,
      0.283864528F,     0.0123050697F,    -0.423871696F,    0.0184997302F,
      -0.0810294896F,   9.42150145E-5F,   0.134932891F,     -0.0299984664F,
      -0.00614259858F,  -0.00923785102F,  0.00555178383F,   0.148673862F,
      0.00915636215F,   0.000845130882F,  0.0179882701F,    0.00666598F,
      0.00459144171F,   -0.0240672324F,   -3.35508776E-6F,  0.0200532507F,
      -0.0321704708F,   0.10084457F,      -0.0134664988F,   0.065470919F,
      0.0145702427F,    -0.00414247578F,  0.00185159F,      -9.43475607E-5F,
      -1.56957958E-5F,  0.000397794676F,  0.0143083381F,    0.0150350928F,
      0.062981911F,     -0.00379477721F,  0.0458239764F,    -0.0577156581F,
      -0.0405059978F,   0.17982015F,      -0.000213266525F, 0.000203394244F,
      -1.87052592E-5F,  -0.046486672F,    0.000378625758F,  0.0270078089F,
      0.0054262816F,    0.000385423249F,  -0.00247067329F,  -0.0475572683F,
      0.0072939354F,    -0.13887538F,     0.000101353158F,  -0.00689880596F,
      0.000170501953F,  -0.00342107727F,  -0.00667586364F,  -0.00266147382F,
      -0.0112588853F,   -0.00233298773F,  -0.0598549694F,   -4.94289525E-5F,
      -0.0586749129F,   -0.000585111149F, -0.124597475F,    -0.0993422195F,
      -0.0143440375F,   0.00471694069F,   0.000110480258F,  0.123546802F,
      -0.00827891845F,  -0.00426892797F,  -0.000264538F,    -0.00493840128F,
      -0.00602638861F,  -0.0635757744F,   0.072645396F,     0.00709426682F,
      -0.00610319385F,  0.00619113352F,   -0.00423260406F,  0.0181504432F,
      0.00694941729F,   -0.00439429376F,  -0.0140770916F,   -0.19893752F,
      -0.00467211101F,  -0.0138824154F,   0.00960645825F,   -0.000510001F,
      0.0187275689F,    -0.134185925F,    -0.0095749218F,   -0.0194397978F,
      0.0468761437F,    -0.0484999903F,   0.0322936699F,    -0.000368328445F,
      -0.0776672214F,   0.00303611858F,   -0.000160475262F, -0.00339923846F,
      0.00112102239F,   0.018772427F,     -6.90404049E-5F,  0.0303263105F,
      0.0105626332F,    0.0132667329F,    -0.000407275598F, -0.00315281912F,
      -8.06851676E-5F,  0.000225236348F,  -0.0198654588F,   0.0120847328F,
      1.94306776E-5F,   -0.0038969086F,   -0.0024606674F,   1.09300963E-5F,
      -0.184926644F,    0.000411093F,     0.00155639299F,   -0.148550227F,
      0.0216131564F,    0.004802763F,     -0.039096605F,    -0.0052660685F,
      0.000162614466F,  0.000365741435F,  0.00126282615F,   -0.000235732179F,
      0.0119442753F,    0.00870273542F,   0.0219260957F,    -3.22836604E-5F,
      -0.011360798F,    -0.000555745035F, -1.31463203E-5F,  -0.00902978145F,
      -4.01189209E-6F,  -0.0117577733F,   0.000599035295F,  0.000187062571F,
      0.0535088889F,    0.0252410155F,    -0.022422947F,    -0.000351318624F,
      0.0164589919F,    0.0105542811F,    0.00272922288F,   -0.000357485842F,
      0.012570193F,     0.0132541824F,    -0.000218104396F, 0.0549748689F,
      -0.00673338398F,  0.000214291038F,  -0.024378486F,    0.0215749145F,
      0.0154335946F,    -0.00360415666F,  0.0088945888F,    -0.0328174792F,
      0.00314869545F,   -0.0730372369F,   0.0194639713F,    -0.00563614722F,
      -0.000217154506F, -0.00262132706F,  0.133121803F,     0.0273314249F,
      -0.014403333F,    0.00454094261F,   0.00114320044F,   0.0228739437F,
      0.0990139544F,    0.000105775311F,  0.0759897903F,    -0.0432905555F,
      0.164950266F,     -0.00888067298F,  -0.00960946828F,  -0.00572112529F,
      0.00190504524F,   -0.00137148728F,  -0.0502253808F,   -0.000896951475F,
      5.73145517E-5F,   0.0079574585F,    0.00434624776F,   0.0100489575F,
      -0.00268437481F,  -0.0114258137F,   0.00246379361F,   -0.033305347F,
      0.000304207351F,  -0.0316235609F,   -0.0265281126F,   -0.00828643329F,
      0.0125380568F,    0.00324362423F,   0.000179631534F,  -0.0140756089F,
      0.000142147735F,  -0.00010789861F,  0.0530867279F,    -0.00868650153F,
      -0.00363008445F,  -0.00383737427F,  0.00484402757F,   -0.000423002493F,
      -0.000160764F,    -0.0182930082F,   0.0411215834F,    -0.00197506137F,
      0.00960615464F,   0.000863987778F,  0.013178437F,     -0.000279789267F,
      0.00363773806F,   -0.00177641376F,  -0.00405167742F,  -0.101065665F,
      -0.0268844627F,   0.00260342844F,   -0.000295204954F, 0.00244873646F,
      -0.000191739193F, 0.134588376F,     0.000159437652F,  0.0371778384F,
      -0.0021022791F,   0.00322897756F,   -0.00217939611F,  -0.000223115261F,
      -0.0113373604F,   -0.00180357136F,  0.00544214528F,   -0.152510032F,
      0.0701508299F,    0.00274594245F,   -0.0184582192F,   -9.7257187E-5F,
      0.00828837231F,   -0.028145181F,    6.21117351E-6F,   -0.029577F,
      -0.000389588502F, 0.000353069481F,  -0.000297749677F, -0.0144083565F,
      0.00832194462F,   -0.216997027F,    0.0121907825F,    -0.000309202849F,
      0.0791860074F,    0.0444005877F,    -0.0529624224F,   -0.0132141514F,
      -0.00167891511F,  2.06891145E-5F,   -0.00973469485F,  0.00400424283F,
      0.00714078266F,   -9.19312442E-5F,  -0.000135307084F, 0.00591925345F,
      0.00919990893F,   0.122650273F,     -0.0131713161F,   0.0465233289F,
      -0.000385459221F, -0.00418442534F,  -0.0539037324F,   0.222225115F,
      -0.00101009523F,  0.000556343293F,  -0.0103789605F,   -0.00101854955F,
      -0.0212651212F,   0.00517206546F,   0.00909641758F,   7.00364399E-5F,
      -0.0464879945F,   0.00281759468F,   -0.0171709154F,   0.134215459F,
      0.0077199121F,    0.00881363917F,   -0.00404923689F,  -0.115788855F,
      0.0256663319F,    0.0283614788F,    -0.00697652297F,  -0.000873171957F,
      -0.0270112939F,   0.0185588449F,    -0.00405300595F,  0.0971634611F,
      0.000292719895F,  0.0233178828F,    -0.00333046028F,  -0.0107096797F,
      0.00343688414F,   0.00142352504F,   0.0727548525F,    -0.0057850522F,
      -3.80711426E-5F,  0.0014868906F,    -0.0021101844F,   0.0126680043F,
      0.00595075F,      -0.0212164167F,   0.0238161F,       0.0125854816F,
      -0.00294314162F,  -0.012099145F,    0.12539421F,      0.0226198733F,
      0.0100052375F,    -0.00100408134F,  0.00196462823F,   0.0208376069F,
      0.021954963F,     -0.0736247376F,   0.000862294226F,  0.000262101181F,
      0.00800185F,      -0.0775927231F,   0.0235250425F,    0.0175930187F,
      0.0376117453F,    0.0016945306F,    0.0081063807F,    -0.000124280981F,
      0.00523873512F,   0.00770704029F,   0.139378965F,     -0.00247515086F,
      0.00220275833F,   0.000193324668F,  0.0225445628F,    -0.00733505609F,
      -0.0162772126F,   -0.0743998736F,   0.00246435939F,   0.00578878028F,
      0.0792651549F,    0.000231364465F,  0.0166494735F,    4.61272648E-5F,
      2.28007157E-5F,   -0.00144081505F,  -0.000135199865F, 0.00305989641F,
      0.00236601848F,   -0.0840582773F,   0.00117021741F,   -0.0288019963F,
      0.000149977772F,  0.00708411122F,   0.000595260935F,  -0.00263558887F,
      0.0648229644F,    -0.0300103165F,   0.0751722082F,    -0.161730021F,
      -0.00577988476F,  -2.18818204E-5F,  -0.000301492255F, -0.00826273113F,
      -0.0339881666F,   0.000573015946F,  0.0660277382F,    0.0147162545F,
      0.0106721483F,    -0.00770295737F,  0.000969462F,     -0.00632461719F,
      -0.000478386064F, 0.00793627277F,   -0.0619293712F,   0.0193288215F,
      -0.0322675779F,   -0.0277007669F,   0.00816565473F,   -0.015974002F,
      -0.122948498F,    -0.0261999257F,   0.000294982281F,  0.00791466702F,
      -0.0102648502F,   -0.101343058F,    -0.00568580767F,  -0.0164922383F,
      0.0025895834F,    -0.00231610704F,  -0.00220693089F,  -0.000204991913F,
      -0.00799447671F,  -0.0114658512F,   0.000262335612F,  -0.053095188F,
      -0.00641770801F,  -0.0947265923F,   0.0237850901F,    0.00371056842F,
      0.00170329551F,   0.000167826161F,  0.0010425119F,    -0.0145603307F,
      0.0627261922F,    0.00477361027F,   -0.0157844853F,   0.0072506736F,
      -0.0094679324F,   0.00071570609F,   0.0102716228F,    0.0242039394F,
      0.0084075937F,    0.00798236113F,   0.000418130774F,  0.100263096F,
      -0.00035018631F,  -0.00455124909F,  -0.0800981149F,   0.0123768924F,
      0.113705762F,     -0.050619293F,    -0.0142906085F,   -0.000108014166F,
      0.0404606685F,    0.000958988F,     0.0407794341F,    0.000210087586F,
      -0.0749723092F,   0.0508473106F,    0.0424951129F,    0.0580640174F,
      -0.00925416593F,  -0.0145933805F,   -4.10371558E-6F,  0.000159772389F,
      -3.44746659E-5F,  -0.0472744666F,   -0.0194334406F,   0.00543935085F,
      0.00796295889F,   0.000187898971F,  0.0601502471F,    0.136013061F,
      0.00775895081F,   -0.0566930883F,   0.00010447696F,   -0.00704184175F,
      -0.00016298375F,  0.111826733F,     -0.0344406068F,   -0.0111206556F,
      0.101096399F,     0.00018109451F,   -0.00820079166F,  -0.000100582729F,
      0.0203805249F,    0.000635024626F,  0.00277488353F,   -0.0271723028F,
      0.0530349799F,    0.0719622076F,    3.00616E-5F,      -0.0707298592F,
      -0.253456116F,    0.0651109666F,    -0.000103659615F, 0.0210429244F,
      0.0164161474F,    -0.00176247756F,  -0.0501596667F,   0.0297756772F,
      -0.034503974F,    0.0397365689F,    -0.0104398942F,   0.0869979337F,
      0.00252089F,      -0.034483131F,    -0.0593846105F,   -0.0017212918F,
      0.00344563695F,   0.00458014477F,   -0.0371922255F,   0.000705213461F,
      0.0140989246F,    0.0067325444F,    0.0127114505F,    -0.0073574F,
      -0.00805178098F,  0.117417574F,     0.00188364729F,   0.000212694882F,
      0.082677F,        -0.0222934F,      0.000126754618F,  -0.00052982592F,
      -8.91237942E-5F,  0.112973846F,     0.000140433651F,  -0.0859003589F,
      -0.0199502427F,   0.000730199448F,  0.000478044822F,  0.0451243892F,
      -0.000114328512F, 0.00033208885F,   -0.0937033594F,   -0.055209484F,
      -0.000140070348F, -0.00899235625F,  0.000514173589F,  -0.000187341648F,
      0.0135792671F,    2.09851205E-5F,   -0.00118056859F,  0.00310264505F,
      -0.0643228441F,   -0.0148677975F,   0.0247340593F,    -0.0321088023F,
      -2.50322373E-5F,  -0.000259780412F, 0.0740456209F,    -0.000333529722F,
      -0.0220643F,      0.054506056F,     0.0834250078F,    0.000309762341F,
      -0.00661441963F,  0.000682043668F,  0.000587631599F,  -0.00311767F,
      0.000205908029F,  0.00337195047F,   -0.0010154323F,   9.19235172E-5F,
      -0.19721958F,     -0.0752439126F,   0.0355418399F,    -0.000653843803F,
      0.00672046468F,   -0.0299409572F,   0.000328850292F,  0.000157654198F,
      0.123591982F,     -0.0570277348F,   -0.000677498349F, -0.0797257125F,
      0.0341402516F,    0.0155406017F,    0.190199316F,     -0.0766453817F,
      0.0816046447F,    0.0494039096F,    0.0330018476F,    0.0151191736F,
      -0.0228857268F,   0.192646429F,     -0.689602196F,    0.0101947412F,
      -0.0010808059F,   -0.00792878307F,  -0.00128539151F,  -0.0610067807F,
      -0.00397403352F,  -0.00879427325F,  -0.0136820087F,   0.0821299478F,
      0.0425548963F,    -0.0125375446F,   -0.00442775944F,  -0.00364774326F,
      0.0323808864F,    -0.00583940698F,  0.0299534F,       0.0291075911F,
      -0.0260667428F,   -0.0363942534F,   -0.0238885842F,   -0.0681887567F,
      -0.0130451769F,   -0.00929129217F,  -0.00367996795F,  -0.0279227F,
      -0.00351062045F,  0.0482594594F,    -0.0309049338F,   0.0141462926F,
      0.000145586208F,  -0.000580184453F, -0.0695323348F,   0.0228208061F,
      0.0648530275F,    -0.00344328163F,  0.00011000532F,   -0.00522100087F,
      -0.000265109644F, -0.000120750745F, -0.00455342606F,  -0.0237350687F,
      0.00107596652F,   -0.049621772F,    -0.00934015773F,  0.00080676272F,
      8.87033093E-5F,   0.00749580655F,   -0.0266669709F,   -0.0114651872F,
      -0.0165077914F,   -0.0011360764F,   -0.000926164386F, -0.00427623466F,
      -0.0392927527F,   0.0397204608F,    0.0453650281F,    0.00567648467F,
      0.00751903374F,   -0.0251178481F,   -0.000348184025F, -0.00634645F,
      0.000211227714F,  0.00333858258F,   5.7091147E-5F,    0.0157941915F,
      -0.00472597359F,  -0.0202344395F,   -0.0108961454F,   -0.000718646101F,
      -0.106582F,       -0.0195416752F,   0.0041850959F,    0.00943250768F,
      -0.0074538039F,   0.0427332371F,    -0.015656637F,    -0.05963834F,
      -0.0262057967F,   -0.00106490322F,  0.000281663146F,  -0.0117078125F,
      0.000719142321F,  0.000201167451F,  -0.000145789381F, -0.0292255227F,
      0.0130828964F,    2.21885239E-6F,   -0.0089313658F,   0.000803014496F,
      0.011591061F,     0.00699192286F,   -0.0695165768F,   -0.0216976795F,
      0.000591328135F,  0.00015307487F,   -0.00520827F,     0.00783816166F,
      0.0336762853F,    -0.000840829336F, -1.73002791E-5F,  -0.00618918054F,
      -0.0892350152F,   0.00924816541F,   -0.00708034402F,  0.017810598F,
      0.000246236159F,  0.0396063887F,    0.021612037F,     -0.000874095946F,
      -0.000238215987F, -0.000566835399F, 0.00516915368F,   -9.20825842E-5F,
      -0.0399645045F,   0.000773683249F,  -0.0197630282F,   9.02765896E-5F,
      0.039298486F,     0.0103524532F,    0.0378282033F,    -0.00897298753F,
      -0.0525910854F,   -0.0714482591F,   -0.0186014101F,   0.00274592428F,
      -0.0158598889F,   0.00196941779F,   -0.00604019174F,  -0.0183125585F,
      -0.00350941345F,  -5.58242245E-5F,  -0.111184426F,    -0.000921152881F,
      -0.000377752061F, 0.00505940802F,   -0.00818356872F,  0.00378650636F,
      0.0684810802F,    -0.00381084811F,  -0.0832495838F,   -0.0125471698F,
      0.000155649948F,  0.0149661172F,    -0.00591755332F,  -0.0376516394F,
      0.0216345228F,    0.0111402059F,    0.0578372255F,    0.0929348F,
      0.00138736959F,   0.0884754211F,    -0.322116256F,    -0.0354002044F,
      0.0368127972F,    0.0233717915F,    -0.0423295535F,   -0.0106512727F,
      0.042443648F,     0.344174922F,     -0.00374060846F,  -0.000372614712F,
      0.0247540064F,    0.00510250637F,   3.75749696E-5F,   0.0270525292F,
      0.0194191597F,    0.0433324575F,    0.0330074541F,    0.000359963393F,
      -0.00383029319F,  -0.0619073622F,   -0.0807790384F,   0.000975206262F,
      -0.0739034787F,   -0.000288788666F, 0.00512715895F,   0.0425767787F,
      0.0339732654F,    0.00335731148F,   -0.0116960397F,   0.000844796596F,
      -0.0174674932F,   -0.0474402942F,   -0.0616501458F,   -8.36727559E-5F,
      -9.4418865E-5F,   -0.000147212631F, -0.000171482534F, -0.00196796958F,
      0.00763827702F,   0.182539195F,     0.019738419F,     -0.0161729809F,
      -0.000174310582F, -0.0210239161F,   -0.000887070782F, -0.0052556931F,
      0.040858753F,     -0.114013173F,    -0.152881339F,    0.00734240236F,
      0.00419261586F,   0.0504745282F,    3.55719249E-6F,   -0.0217983276F,
      0.0537779592F,    -0.0043101497F,   0.00400305586F,   -0.0437675677F,
      -0.0235295091F,   0.0241614394F,    -0.00587518932F,  0.0374299F,
      0.00050612F,      -0.0796936527F,   -0.00654183654F,  -0.00378943281F,
      0.00344137778F,   -0.00852964912F,  0.0593136139F,    -0.0102809314F,
      0.00926246773F,   0.0252890047F,    -9.40306782E-5F,  -0.00504336227F,
      -0.00583421F,     0.010166375F,     0.0188759435F,    0.0255174674F,
      -0.0115950201F,   -0.00982432254F,  0.000196550405F,  5.18470151E-5F,
      0.0260918774F,    0.0149794538F,    -5.39551475E-5F,  -0.00272528687F,
      0.0682076737F,    0.0344715081F,    -0.0628691763F,   0.0335082673F,
      -0.0251318906F,   -0.000171332926F, -0.0415441021F,   0.0452642851F,
      0.0319252722F,    0.00489469944F,   -0.046567034F,    0.0667435303F,
      0.0130509799F,    -0.0342041217F,   -0.00592000922F,  0.0244337153F,
      0.0661869273F,    -0.0283863675F,   0.000382069178F,  0.043264173F,
      0.000149444022F,  0.00202760845F,   0.0492590815F,    -0.00341957249F,
      0.0290576592F,    0.0316739418F,    0.0103895217F,    1.28835572E-5F,
      0.00901756901F,   9.9751167E-5F,    -0.0452158973F,   0.000379569887F,
      0.0100973472F,    -0.0369889215F,   -0.0366453305F,   -0.00436789729F,
      -9.36692959E-5F,  -0.00114774844F,  -8.33643498E-5F,  5.13501391E-7F,
      -6.59008074E-5F,  -0.00102577568F,  0.00281977467F,   -0.00246715569F,
      0.00891376752F,   0.000120625416F,  -0.0674540177F,   -0.0369195528F,
      0.00280123227F,   -0.00407401426F,  -8.37406187E-5F,  -0.000348288071F,
      8.35731771E-5F,   -0.0801542252F,   -0.00481596123F,  -0.016297929F,
      -0.041313082F,    -0.00271926704F,  -0.00190755702F,  -0.000115223207F,
      -0.00784032885F,  -6.66458873E-5F,  0.000415498769F,  0.00283790892F,
      -0.00378638273F,  0.0390145294F,    -0.000185488505F, 0.00501758279F,
      0.0280413534F,    -0.0141087072F,   -0.00011358397F,  -0.0456722677F,
      -0.00690530147F,  0.00211696606F,   0.00754856691F,   0.0150790988F,
      -0.0336531401F,   -0.00652521756F,  -0.00207925588F,  0.0120634874F,
      0.00820291787F,   0.00791481324F,   0.006431852F,     -0.000234816165F,
      -0.00904700812F,  -9.79719334E-5F,  -0.00358822127F,  -0.000177511043F,
      -0.00449770223F,  0.000752104854F,  0.0746508613F,    -0.00879884325F,
      0.000106145118F,  -0.0084766075F,   0.0433826707F,    -0.000228317207F,
      -0.00787925627F,  -0.0299503021F,   -8.3238243E-5F,   0.0121057201F,
      0.000436642702F,  -0.0224644355F,   0.0001064237F,    -0.00356278894F,
      0.0189556926F,    -0.000271853438F, -0.000126070081F, 0.0107446676F,
      -0.000134457849F, 4.3391763E-5F,    0.0415545069F,    0.0230405759F,
      0.000129178457F,  -0.0184517F,      -0.00187607796F,  -0.000159687173F,
      0.00303436164F,   0.000111293557F,  -0.00103437505F,  0.00117334037F,
      -0.000324770779F, -0.00345724029F,  -0.00117057585F,  -0.0176935047F,
      0.000128762476F,  0.000182632211F,  0.0300189331F,    -9.02648171E-5F,
      0.0125269713F,    -0.0124466671F,   0.111033782F,     -0.000110376146F,
      0.00432873284F,   -0.000217713372F, 0.000295149715F,  0.000749338884F,
      7.65112491E-5F,   -0.000683505205F, 0.000104971637F,  0.000145839876F,
      -0.0520650744F,   0.00312365126F,   0.00560741499F,   0.000205809309F,
      0.0106600337F,    -0.0383878F,      -0.0108977593F,   -0.000127603576F,
      0.00745290564F,   0.0010654846F,    -4.28221E-5F,     -0.00245886901F,
      0.00595554058F,   -0.00021034159F,  0.0633311793F,    0.00397796277F,
      -0.00321701542F,  0.00138552883F,   0.00640907F,      -0.0226873811F,
      0.000983226229F,  0.0634377077F,    -0.0430357233F,   -0.0165864341F,
      -0.00079991942F,  0.00528950291F,   2.21045248E-5F,   0.00308160647F,
      -0.0752764195F,   0.00619027624F,   -0.0133841373F,   -0.0051787016F,
      0.00502771605F,   0.000777157838F,  0.000420549884F,  -0.00139657F,
      -0.00857251F,     -0.00268492731F,  -0.000516377098F, -0.0230303966F,
      0.0124814771F,    -0.0464109294F,   0.00304573728F,   -0.0302105136F,
      -0.00115078955F,  0.00198309543F,   0.00287796138F,   -0.0570336543F,
      -0.083783105F,    -0.0687481835F,   0.0192042775F,    9.59656682E-5F,
      0.000163659337F,  -0.00441442803F,  0.0211268291F,    0.036760997F,
      -0.0588972606F,   0.0116103385F,    0.00011726479F,   -0.00642648246F,
      5.22320079E-5F,   1.24307462E-5F,   0.00112470973F,   0.00612852024F,
      -0.0406096093F,   0.0203488395F,    0.0169536564F,    -0.000213441337F,
      -8.17485E-5F,     -0.00175251509F,  0.00167563953F,   -0.000374075607F,
      0.0292649418F,    -0.000277239043F, 0.00361801614F,   0.00165253272F,
      0.0304466244F,    0.0131945983F,    0.0384444371F,    -0.00161177374F,
      -0.0284949075F,   0.00747787906F,   0.000610045507F,  0.00455713784F,
      -0.00013795901F,  -0.0031277067F,   3.97277399E-5F,   0.00347492867F,
      -0.00682767341F,  -0.010394915F,    0.000533242594F,  1.02526892E-5F,
      -0.0347891152F,   -0.0259621833F,   -0.000520800648F, -0.00083362346F,
      0.000380625424F,  0.00700698374F,   -5.56422638E-5F,  0.0558554903F,
      -0.000687931082F, 0.000652779185F,  -0.000139753392F, -0.00150321831F,
      -0.000217778608F, 1.92374591E-5F,   -9.38647E-5F,     -0.0465425327F,
      0.000978154F,     0.00326774432F,   -0.025382271F,    -0.000230546502F,
      -0.0033662729F,   -0.000246777461F, 0.0358196609F,    -0.0294541046F,
      -0.00146603363F,  1.02779541E-6F,   -0.00549146F,     0.0178820509F,
      0.0185745377F,    -8.96925776E-5F,  3.04961013E-5F,   -0.00699198246F,
      0.0923331231F,    0.00157942064F,   0.00988438446F,   0.0579553246F,
      -0.000158055F,    -0.0305607896F,   -0.00299988058F,  -0.00162938109F,
      -0.0244339649F,   0.000420672906F,  0.00882415567F,   -0.00313023478F,
      0.0653533787F,    -0.000474738074F, 0.000742325559F,  -1.64993089E-5F,
      0.00522875553F,   0.0108084716F,    -0.104273386F,    -0.00242579682F,
      0.000713829882F,  0.0145898527F,    -0.005885405F,    -0.000315216923F,
      0.0011073862F,    0.000429866777F,  0.0124619808F,    0.0108965179F,
      -0.00136103714F,  5.43421847E-5F,   0.00739236F,      -0.000929350615F,
      0.000244089737F,  0.0265940949F,    -0.00168831181F,  0.00768708112F,
      -0.0681171417F,   -0.00795634836F,  0.00685873441F,   -0.0181055237F,
      -0.000105519597F, 0.00893623196F,   -0.000562848756F, 0.0583529584F,
      0.0866990909F,    -0.00029254664F,  -0.0178802833F,   -0.0597255528F,
      -0.00341229723F,  0.0191356875F,    0.0349882394F,    0.00440349F,
      -0.0151492571F,   -0.0258767866F,   -0.00653330376F,  0.00721690571F,
      -0.00205113343F,  -0.00843790453F,  0.0123681715F,    -4.52220083E-5F,
      0.0325626954F,    0.00377550861F,   0.000345655833F,  0.00963459257F,
      -0.000281985733F, -0.0581235774F,   0.0359652489F,    0.000171131731F,
      0.034895286F,     0.047338441F,     0.01022017F,      0.0137088345F,
      0.0389877521F,    0.000220028218F,  0.00302689173F,   0.0227826536F,
      0.0304069426F,    1.6246624E-5F,    0.0120620076F,    0.00483185658F,
      0.0029624009F,    -0.0553958267F,   0.00333106029F,   -2.11354127E-5F,
      -0.000120156546F, -0.00121749414F,  2.94914225E-5F,   0.0057138307F,
      0.00399444206F,   -0.0348476171F,   -0.036416132F,    -0.00225152494F,
      7.27951119E-5F,   -0.06358511F,     0.000312932185F,  0.00119493436F,
      0.101410076F,     0.00686568487F,   -0.0520089082F,   0.000308522925F,
      0.000375144533F,  -0.0374163277F,   -0.000155652306F, -0.00318137417F,
      0.00269334554F,   0.000137379058F,  -0.00113520492F,  0.00300807483F,
      -0.0375016F,      -0.0320737176F,   0.00191411807F,   -0.00386904576F,
      -0.000234974694F, 0.0463883877F,    0.00235222047F,   0.0182945672F,
      0.000244493887F,  6.02823202E-5F,   -0.00506036356F,  0.000113300208F,
      0.000581364788F,  0.00166642293F,   0.000198094029F,  -0.0120663838F,
      -0.00614206633F,  -0.00186200964F,  0.0596399531F,    -0.0279817507F,
      -0.00180132792F,  -0.0102190766F,   -0.00164264336F,  -9.12075484E-5F,
      0.00469598221F,   0.00233996776F,   0.000100474674F,  0.000906076F,
      0.0255648829F,    -0.00868891738F,  0.00363564235F,   -0.0214001276F,
      -0.00729103852F,  6.70343215E-5F,   0.0183316357F,    0.00203129346F,
      -0.00134154269F,  0.0552013665F,    -0.0355449319F,   -0.0202650428F};
  static const float A[1200] = {
      0.103950061F,     0.00734369271F,   0.0262814537F,    -0.0557999164F,
      -0.0479476601F,   0.104288913F,     0.0432228222F,    -0.169411153F,
      0.0716855079F,    0.101892442F,     0.0283510815F,    -0.0850463137F,
      0.112276912F,     -0.0652057603F,   -0.031675417F,    -0.198218F,
      0.118733644F,     0.105433568F,     -0.00995934103F,  -0.0104854498F,
      -0.0106302248F,   -0.171435729F,    -0.16915679F,     0.39721185F,
      -0.294906557F,    -0.0715059116F,   0.281553686F,     0.454058021F,
      -0.128969342F,    -0.252322108F,    -0.113749541F,    0.0508877076F,
      0.0383499339F,    0.72216177F,      0.181102633F,     -0.913035393F,
      0.143244341F,     0.10037677F,      -0.0799570754F,   0.0269017871F,
      -0.0628965F,      -0.0421391912F,   0.0644734874F,    -0.000103919359F,
      0.0096252542F,    -0.000160703683F, 6.87823422E-6F,   -1.09368675E-5F,
      -0.059205547F,    -0.08611653F,     0.0146178342F,    -0.00814706087F,
      -0.00840780325F,  -0.00823901501F,  0.0159298889F,    -0.113981016F,
      -0.0740325153F,   0.768522441F,     0.42954728F,      -1.14502418F,
      -0.341273457F,    -0.246455401F,    0.418795258F,     -0.105297543F,
      -0.0639277F,      0.0924852341F,    -0.0615345463F,   -0.207222953F,
      0.258761883F,     -0.479516536F,    -0.0565092452F,   0.277285308F,
      -0.163398147F,    -0.105240777F,    0.447222799F,     0.060720738F,
      -0.55768621F,     0.473753065F,     -5.25530813E-5F,  4.08596752E-5F,
      5.50485383E-5F,   2.35396947E-5F,   -1.55391645E-5F,  6.69373403E-5F,
      4.35508809E-5F,   0.00015226347F,   -2.04697244E-5F,  -0.564911664F,
      -0.0513980836F,   0.537216246F,     -0.0513829291F,   0.142363563F,
      -0.189708486F,    1.01331246F,      -0.00883915182F,  -0.725215137F,
      0.0912196562F,    0.00821464136F,   -0.0163992848F,   4.07803491E-5F,
      -0.000154984868F, 3.27846319E-5F,   -0.0432228521F,   0.0648913532F,
      0.0797243F,       -0.15793632F,     0.183934808F,     -0.0669266F,
      0.0614202805F,    -0.0206786785F,   -0.0508414581F,   -0.909219265F,
      0.290515512F,     0.289321393F,     9.2570066E-5F,    -5.79943153E-5F,
      -3.74702722E-7F,  -0.0785864219F,   0.0195426457F,    0.0419312119F,
      -6.91819E-5F,     4.23120619E-5F,   -9.8869541E-6F,   -0.225584954F,
      0.163431779F,     0.0858263373F,    -0.166739449F,    0.0216433872F,
      0.014953332F,     -0.031217536F,    0.0322434306F,    0.0387390256F,
      -0.153756529F,    -0.0252619181F,   0.0821005553F,    0.107326172F,
      0.08973369F,      0.0946201533F,    -1.09160244F,     -0.426862627F,
      1.62809396F,      -0.00013422F,     0.000132975416F,  -1.79633244E-5F,
      0.206889063F,     0.229652256F,     -0.499073118F,    0.000191172076F,
      4.41220473E-5F,   -0.000203557662F, -1.19711316F,     -0.389341146F,
      1.42432737F,      -0.575002074F,    0.264873624F,     0.182434514F,
      0.00484199263F,   -0.169008225F,    0.183803365F,     0.227140769F,
      -0.0836994201F,   -0.161747754F,    0.000937988341F,  0.00110275054F,
      0.00105293666F,   0.484631151F,     0.198807403F,     -0.65151614F,
      -0.167325139F,    -0.00564204622F,  -0.0376888812F,   0.0447322801F,
      -0.125112325F,    0.123905152F,     -0.00240024109F,  -0.00206731958F,
      -0.00223055668F,  0.0302489027F,    0.0302679017F,    -0.0920007F,
      -0.0802226067F,   0.152647913F,     -0.0626037344F,   -0.500090718F,
      0.208955526F,     0.175581411F,     0.614444733F,     0.106346495F,
      -0.37584132F,     0.0906402F,       -0.0108015873F,   -0.0745190457F,
      -0.100519463F,    0.0297473352F,    0.0286585353F,    0.137467608F,
      0.0361654572F,    -0.0058752927F,   -0.0696887672F,   -0.0289759412F,
      0.0223631766F,    -0.0607831776F,   -0.0825810134F,   0.0247240439F,
      0.0421635173F,    -0.0644553378F,   -0.0757154077F,   -0.075794585F,
      0.0236273259F,    0.022290796F,     -0.0806471556F,   -0.130658314F,
      0.263156116F,     -0.911146581F,    0.179970115F,     0.552748322F,
      -0.062545836F,    -0.0254768468F,   0.0323367082F,    -0.705323279F,
      -0.432335317F,    1.10922909F,      0.0317083336F,    0.0471627824F,
      -0.0979548246F,   0.000364007603F,  8.63220339E-5F,   -0.000149145708F,
      0.155267835F,     0.0239789207F,    -0.158185631F,    -0.915913284F,
      -0.180381492F,    0.962639451F,     -0.151787013F,    0.064871937F,
      0.0256956685F,    -0.120796666F,    0.0392629877F,    -0.0679309F,
      -0.123258621F,    -0.319535434F,    0.474084795F,     -0.432320774F,
      -0.112114556F,    0.389269769F,     0.167842016F,     -0.144932106F,
      0.0192271527F,    -2.46046329E-5F,  7.64150682E-5F,   -2.12274699E-7F,
      -0.650772393F,    -0.0255454928F,   0.422824055F,     -0.0179482363F,
      0.123052374F,     -0.0819587111F,   -1.03027642E-5F,  -2.988917E-5F,
      -5.7213997E-6F,   -0.0377055258F,   0.0283664241F,    -0.0247474536F,
      -0.00232383958F,  -0.0141469184F,   -0.0144922361F,   0.043562524F,
      -0.197547927F,    0.155662015F,     7.3062788E-6F,    -2.94073834E-5F,
      2.73513269E-5F,   -0.009938173F,    -0.108677052F,    0.048480235F,
      0.113628872F,     -0.183428854F,    0.0363441743F,    0.0300595853F,
      -0.116390333F,    -0.117204227F,    0.000159072908F,  1.7521219E-5F,
      -2.41563957E-5F,  0.00501468172F,   -0.093268685F,    0.163192421F,
      -0.000338964F,    -3.29275535E-5F,  -0.00019166409F,  -0.000509996898F,
      -0.000565041497F, -0.000362558494F, -0.118837416F,    0.0354411528F,
      -0.00139886979F,  0.275425464F,     0.0921239853F,    -0.219236195F,
      -0.000274711114F, 0.000199782851F,  0.00014573666F,   -0.101536803F,
      0.0627535F,       -0.0709619373F,   -0.0337640718F,   0.00587510597F,
      0.0112407748F,    -0.000225567506F, 0.000111007292F,  -8.09986E-5F,
      -0.0699755773F,   0.487385869F,     -0.599260151F,    1.69541054E-5F,
      -9.99477561E-5F,  -1.01006653E-5F,  0.279458761F,     0.253311038F,
      -0.633486271F,    -0.0161941759F,   0.411887676F,     -0.442691654F,
      0.0719557703F,    0.128515095F,     -0.191470921F,    0.0980446935F,
      -0.0867288411F,   0.0445138142F,    -0.232315734F,    0.109272122F,
      0.12707594F,      -0.0743248388F,   -0.0193453096F,   0.0335439108F,
      8.42845911E-5F,   -5.75464655E-5F,  2.20451548E-5F,   4.78998263E-5F,
      -7.70805491E-6F,  3.19703E-5F,      -0.0673882142F,   -0.066583693F,
      -0.0134030012F,   -0.000615104625F, -0.00055822474F,  -0.00077998417F,
      0.136076987F,     -0.041633565F,    -0.0396283343F,   0.171559349F,
      0.0365859121F,    0.0135113215F,    -0.0573136769F,   -0.160610557F,
      0.225842625F,     7.27488587E-5F,   -0.000136095754F, 8.22684233E-5F,
      -0.065647915F,    0.271750897F,     -0.177606612F,    0.000202763404F,
      -2.35790631E-5F,  -0.000149404659F, -4.85961064E-5F,  -7.23248959E-5F,
      7.24366182E-5F,   -0.159369096F,    0.00338009465F,   0.0464926884F,
      -0.000160908079F, 2.71056233E-5F,   0.000101976504F,  1.0208658F,
      0.507293463F,     -1.48954046F,     -0.0002718092F,   -4.33874848E-5F,
      0.000227710101F,  0.00015310751F,   -0.000165245132F, -8.32413843E-6F,
      0.220511541F,     -0.116840906F,    -0.124065533F,    0.0485185571F,
      0.206168264F,     -0.209704503F,    -0.142767042F,    0.0182799418F,
      0.23381792F,      -0.000209038801F, 8.98871222E-5F,   1.2386341E-5F,
      0.152185559F,     -0.0323139429F,   -0.131433591F,    0.0249381084F,
      -0.0320278518F,   -0.0240369234F,   -0.0193472374F,   -0.0956841558F,
      -0.0833461657F,   7.88875514E-6F,   9.76454885E-6F,   -2.86011127E-5F,
      0.026238054F,     -0.0770539F,      -0.0695849285F,   0.005959F,
      0.0551766641F,    -0.149897501F,    -8.56482729E-5F,  0.00013171717F,
      -9.24076812E-5F,  0.0714856535F,    -0.168731511F,    0.213759065F,
      -0.0812247396F,   -0.020856401F,    0.14949806F,      0.0136625217F,
      -0.0226836354F,   0.0588542558F,    -0.471398F,       -0.0589240827F,
      0.177222028F,     -0.0122421458F,   0.253333241F,     -0.190901667F,
      0.0978599861F,    -0.0145665118F,   -0.0597156659F,   -0.0270235185F,
      0.0349099413F,    0.145903856F,     0.0296728238F,    -0.0658036321F,
      -0.0608430207F,   -0.0463325307F,   0.177491307F,     -0.183498055F,
      0.174026489F,     -0.115129031F,    0.121043481F,     -0.371579051F,
      0.185285568F,     0.193622306F,     0.422356308F,     -0.133028075F,
      -0.318244845F,    -0.0890002623F,   -0.0582928173F,   0.0771105587F,
      6.5287E-5F,       0.000137892494F,  -0.000196389214F, -0.0232712328F,
      0.0455137864F,    0.0391237698F,    0.997205794F,     0.42126298F,
      -1.4099524F,      0.0458878241F,    0.0515760668F,    -0.230400771F,
      -0.175233021F,    -0.0195833482F,   0.125585601F,     0.071311377F,
      -0.136653081F,    0.0680407435F,    0.0112217152F,    0.0915150046F,
      -0.0897742137F,   0.0793073326F,    -0.0157290623F,   -0.201311335F,
      0.590258777F,     -0.337258577F,    -0.128436342F,    0.0143987993F,
      0.00272453693F,   0.00206198194F,   0.344847322F,     0.297907829F,
      -0.884224892F,    0.137744769F,     0.362787962F,     -0.419432074F,
      0.910792F,        -0.431209326F,    -0.229855254F,    -0.189634904F,
      -0.116240084F,    0.197091505F,     -0.0530310087F,   0.0133249778F,
      0.158646032F,     0.173810884F,     0.186834767F,     -0.295882791F,
      0.00155249378F,   0.0918606F,       -0.0914060324F,   -0.0680473521F,
      0.00571715739F,   0.0259278882F,    -0.44065991F,     -0.387744F,
      0.646733105F,     -0.0480026379F,   0.0371389426F,    0.0481956229F,
      -0.0130909383F,   0.153346822F,     -0.13346824F,     0.0804075F,
      -0.0209063943F,   -0.0366317853F,   0.0712410584F,    0.00511269784F,
      -0.0111545715F,   -0.0657224F,      -0.122844249F,    0.0640869F,
      -0.00734002423F,  0.0984239876F,    0.0961101726F,    -0.125822932F,
      -0.00585536472F,  0.0953662395F,    0.0621143766F,    0.0213823747F,
      0.0824142843F,    -0.188558489F,    0.115763865F,     0.0700410679F,
      -0.000236714579F, -0.000363192696F, -0.000263817055F, -0.277208865F,
      0.109096989F,     0.105467521F,     -0.136046901F,    -0.082545653F,
      0.23709476F,      -0.0180329178F,   0.0348222181F,    -0.0489683822F,
      0.176324636F,     -0.0425317064F,   0.0148279294F,    0.0275009982F,
      -0.00852992199F,  -0.0757648051F,   0.000166333761F,  -0.000143877434F,
      -3.49274478E-6F,  -0.109477311F,    0.0552251F,       0.075258553F,
      -0.00906048436F,  -0.0105280103F,   -0.0107580386F,   -2.1352982E-5F,
      7.33479828E-5F,   -3.50793562E-5F,  -0.746824F,       -0.587332308F,
      1.41323137F,      -0.62559253F,     -0.272756398F,    0.777595878F,
      -0.135766864F,    0.0475949869F,    -0.00160503632F,  -0.0218460113F,
      -0.0980287492F,   0.0677986294F,    0.00234143366F,   -0.0326512568F,
      -0.140395805F,    0.00019204944F,   1.10399355E-6F,   -0.000138165196F,
      5.66561403E-6F,   -5.24293409E-6F,  -2.51451766E-5F,  -0.206601664F,
      -0.228895873F,    0.503101945F,     -0.0308849F,      -0.145410478F,
      0.204348937F,     0.541231394F,     0.480764389F,     -0.913491428F,
      0.167837352F,     -0.0918240175F,   -0.0616188645F,   -1.67514791E-5F,
      -6.48117E-5F,     0.000228377408F,  0.154653028F,     0.00805677F,
      -0.0596188419F,   -0.0222422276F,   0.0728770792F,    -0.063912414F,
      0.0328335427F,    -0.0298877973F,   -0.092980653F,    -0.0264722593F,
      0.0991954058F,    0.0567739569F,    -0.0179058313F,   0.0822196454F,
      -0.0633664131F,   -1.08575737F,     0.283437699F,     0.526063323F,
      -0.0906157047F,   0.146656901F,     0.105280474F,     0.012258945F,
      0.0159429759F,    0.0162352063F,    0.00013543143F,   0.000190914172F,
      -0.000336341647F, 0.0554056056F,    -0.00217732065F,  0.0777115524F,
      5.1278028E-5F,    1.29298205E-5F,   -6.95003037E-5F,  -0.164324194F,
      -0.606229544F,    0.796804726F,     -2.19409467E-5F,  -3.50956252E-5F,
      2.51345664E-5F,   0.283691287F,     -0.162883848F,    -0.0934365243F,
      0.0121917995F,    0.0506675467F,    0.056101162F,     -0.00610011443F,
      0.0629210249F,    -0.0759435892F,   -0.116531461F,    0.077748321F,
      -0.0876583457F,   0.00921952818F,   0.0104341796F,    0.00910598319F,
      -0.0596979782F,   0.0448430628F,    0.060813386F,     -0.035338778F,
      0.0242544264F,    0.0458724275F,    0.0891363099F,    0.0912896544F,
      -0.112970084F,    -1.37209821F,     0.280170858F,     0.616035342F,
      0.18805483F,      -1.2732619F,      1.27655935F,      0.0832833052F,
      0.044189807F,     -0.105618834F,    -0.13830322F,     -0.131580517F,
      0.105569206F,     0.0496056974F,    -0.103237338F,    0.0738597438F,
      0.119003922F,     0.0306272767F,    -0.0908180252F,   -0.959586382F,
      -0.360346496F,    1.28647506F,      -0.000481339754F, -0.000987754436F,
      -0.00111700303F,  -0.129918352F,    -0.14139466F,     0.276832551F,
      0.000351968309F,  -0.000157251852F, -0.000120932447F, 5.21489674E-5F,
      9.30413626E-6F,   1.30885655E-5F,   -0.000144216508F, 0.000117161064F,
      -6.91326131E-5F,  -0.248174429F,    0.138799831F,     0.0525799543F,
      0.0905954391F,    0.0439900756F,    -0.116150193F,    -0.634852648F,
      0.906086624F,     -0.514079094F,    0.214946344F,     -0.0463874601F,
      -0.147847861F,    0.00255639665F,   0.00829686597F,   0.00835223403F,
      0.750495F,        -0.0974396691F,   -0.538738787F,    -0.240987837F,
      -0.335132092F,    0.408834368F,     -0.231804758F,    0.0450990498F,
      0.0988962352F,    -0.0536495186F,   0.154323354F,     -0.00351368566F,
      -0.0112703973F,   0.0115084033F,    0.0128479991F,    2.38305984E-5F,
      -0.000115479888F, -5.32234808E-5F,  -0.113834858F,    0.0081645865F,
      0.0672306269F,    0.0552982F,       -0.00609292416F,  -0.0322078578F,
      0.0819717795F,    -0.0521455593F,   -0.0410098098F,   -0.000317975559F,
      0.000350943155F,  -0.000117155731F, 3.17254535E-6F,   1.3584051E-5F,
      -6.02106811E-5F,  0.0511637591F,    -0.0195784513F,   -0.0271824803F,
      0.170463949F,     0.0454878062F,    -0.0681827664F,   0.586073041F,
      -0.219105244F,    -0.15142706F,     -0.130615801F,    0.0779161304F,
      0.106718883F,     0.450240821F,     0.12126334F,      -0.511251569F,
      -1.61378757E-5F,  2.63950351E-5F,   -6.76481432E-5F,  -0.128622174F,
      -0.0484201F,      0.00885085855F,   0.0396022536F,    0.198715687F,
      -0.192004085F,    0.439609081F,     -0.822448F,       0.64098525F,
      -0.0319335945F,   -0.0111290896F,   0.0843016356F,    0.00390603463F,
      0.0042066942F,    0.00439035846F,   -0.158053979F,    -0.0273755584F,
      0.0732742101F,    -0.0721122175F,   -0.00477148825F,  -0.0656576753F,
      -0.119354747F,    0.100390539F,     0.0225779116F,    1.46574032F,
      0.881842613F,     -2.67936873F,     0.0190899223F,    0.0746568739F,
      -0.0880171508F,   -2.46641721E-5F,  -8.13319784E-5F,  -2.61084151E-5F,
      0.632777214F,     0.0304040816F,    -0.67465961F,     0.0494946688F,
      -0.0186359379F,   -0.0262743365F,   -0.156351477F,    0.0989885703F,
      0.101778693F,     -0.0583439879F,   -0.39682287F,     0.470275849F,
      0.0646071211F,    0.0761092529F,    -0.145105466F,    -0.0359714292F,
      0.127209589F,     -0.0703585371F,   0.00189795881F,   0.0202495679F,
      0.0371556543F,    -1.13845515F,     0.13964048F,      0.468719155F,
      0.0943887F,       -0.114543185F,    -0.0781964511F,   0.236437768F,
      -0.0193857476F,   -0.223726749F,    -0.0847147107F,   0.0284167379F,
      0.134263068F,     -0.0425564572F,   0.0641875938F,    -0.0559034683F,
      0.127858326F,     0.318179756F,     -0.403466225F,    -0.767978191F,
      -0.408105135F,    1.2908864F,       -0.0655927807F,   0.0526101775F,
      -0.0063218656F,   -0.127603561F,    -0.54772532F,     0.716945052F,
      0.00548111787F,   0.00586621603F,   0.00606104F,      0.154334545F,
      -0.0619452521F,   0.0444755778F,    -0.112260751F,    -0.0555221327F,
      0.0077850977F,    -0.100460753F,    0.101279534F,     -0.140395507F,
      0.138551682F,     0.0449750461F,    0.0482935F,       0.0428839847F,
      0.0942372382F,    -0.090866372F,    0.505333066F,     0.25649628F,
      -0.511226535F,    -0.0439273044F,   0.17022337F,      -0.0271232445F,
      0.000104196843F,  -0.000139609008F, -3.39994876E-5F,  0.0230526198F,
      -0.115045786F,    0.08775758F,      -0.0292144939F,   0.00931934174F,
      0.0252897311F,    0.135042444F,     -0.161623284F,    -0.0577674881F,
      0.420903802F,     0.0642412752F,    -0.538320363F,    -0.110734276F,
      0.0504384115F,    0.0778721124F,    0.22248745F,      0.124297515F,
      -0.0776512325F,   0.0627292395F,    -0.089113377F,    -0.0279231537F,
      -0.0701349154F,   0.249388471F,     -0.0976512209F,   0.0412257463F,
      -0.193148851F,    0.208547413F,     0.470855683F,     -0.323218346F,
      -0.0322397687F,   0.265565485F,     0.155268267F,     -0.464801669F,
      0.124015614F,     -0.0614221394F,   0.0178649351F,    -0.0790970474F,
      -0.0836180076F,   0.101914339F,     0.030564012F,     0.0859528705F,
      -0.046628233F,    0.19650881F,      -0.107218824F,    -0.114330642F,
      0.0364495851F,    0.146817908F,     -0.241688F,       -0.343999892F,
      0.23219277F,      0.0874192193F,    0.0459548235F,    0.0397049412F,
      0.012008463F,     -7.2589E-5F,      -5.95586534E-5F,  9.28191E-5F,
      0.108298801F,     -0.0929845273F,   -0.0814310089F,   0.150237188F,
      0.514088631F,     -0.729061186F,    1.0934937F,       0.131326169F,
      -1.018309F,       0.108045973F,     -0.114873186F,    0.0757089183F,
      -0.201897949F,    -0.496240139F,    0.696268916F,     -0.0556604676F,
      0.12597461F,      -0.11218027F,     0.155010283F,     0.0870289877F,
      -0.123415366F,    -2.03480959E-5F,  -0.000157420407F, 7.54788562E-5F,
      0.0980491713F,    -0.111058757F,    -0.0420895927F,   0.0506336652F,
      0.133412674F,     -0.0368763469F,   0.427700162F,     0.090284951F,
      -0.451400191F,    -0.0738027394F,   0.00959227607F,   0.0598221533F,
      0.000197517656F,  -0.0513635501F,   0.00785141531F,   0.000229900877F,
      -0.00187555153F,  -0.00223234575F,  0.116395228F,     -0.253831208F,
      0.170235038F,     0.0101186913F,    -0.135629699F,    0.130620673F,
      -0.084017925F,    0.0873299167F,    0.0820581093F,    0.0253372714F,
      1.08072531F,      -1.28078699F,     0.0107822232F,    -0.0277801529F,
      0.0262509342F,    0.164149135F,     0.16353935F,      -0.341621667F,
      0.371884078F,     0.19476828F,      -0.463760853F,    -0.0161546282F,
      0.0455505513F,    0.0509711057F,    0.112271465F,     0.169801176F,
      -0.201767668F,    7.63639109E-5F,   7.57553207E-5F,   2.43331742E-5F,
      -3.84434279E-5F,  1.90926603E-5F,   -5.9994858E-5F,   0.0106408671F,
      0.0367506631F,    0.0394013114F,    1.61816188E-5F,   7.25116333E-5F,
      -7.1340859E-5F,   0.0195654705F,    -0.0285409261F,   -0.0330644697F,
      0.0616630055F,    0.0259685796F,    0.023782827F,     -0.459231704F,
      -0.0591454916F,   0.443071067F,     0.147094265F,     0.145166621F,
      -0.298365831F,    -0.0318904631F,   -0.20613113F,     0.269010842F,
      3.03431316E-5F,   1.95875946E-5F,   -4.63521937E-5F,  0.0797060281F,
      -0.0171123147F,   0.118302979F,     -0.000223804658F, -1.54400386E-5F,
      0.000183422555F,  -0.00347624975F,  0.0367028713F,    0.0341916233F,
      0.000525899464F,  -0.239917099F,    0.260130733F,     -0.526207209F,
      0.23159638F,      0.0562843829F,    0.375538856F,     -0.0975938737F,
      -0.0795339718F,   -0.0710736513F,   0.936098099F,     -1.01391399F,
      -0.239926443F,    -0.325900108F,    0.660947859F,     -0.00810242165F,
      0.073558569F,     0.089204751F,     -0.000137682902F, 0.000117450429F,
      1.31693196E-5F,   -0.0748511404F,   0.0421076603F,    0.0434705876F,
      -0.126882672F,    0.240647331F,     -0.148326531F,    0.00602492178F,
      0.040764045F,     -0.0498816F,      0.638508797F,     0.318327576F,
      -0.744624078F,    0.00387302693F,   0.0856631845F,    -0.149201766F,
      0.0718079135F,    -0.0942966F,      -0.065588586F,    -0.158936784F,
      0.0705622286F,    0.124271773F,     0.100462325F,     0.106196903F,
      -0.0320386514F,   -0.0394334644F,   -0.111422941F,    0.0112276031F,
      9.3926079E-5F,    4.008424E-5F,     -7.7521E-5F,      -0.0523824468F,
      0.188228115F,     -0.161704838F,    0.271738589F,     0.459604949F,
      -0.650409281F,    -0.0403083339F,   -0.189045534F,    0.148596078F,
      0.39766565F,      0.230108604F,     -0.48791334F,     0.0493537076F,
      0.266935349F,     -0.356081098F,    0.113825426F,     0.077830568F,
      -0.0528883897F,   0.758786F,        0.30103454F,      -0.910429597F,
      -0.671792448F,    -0.109719269F,    0.767840445F,     -0.251888365F,
      0.0598709099F,    0.159579098F,     0.00013613551F,   -0.000133389287F,
      -2.19373087E-5F,  0.0160134863F,    -0.173307374F,    -0.0610594116F,
      -0.112990327F,    0.0395809151F,    0.0997103751F,    0.20067659F,
      0.469129622F,     -0.601075232F,    -0.00257664244F,  0.0799988806F,
      -0.0902468488F,   -0.190593556F,    0.0831647888F,    0.0710814148F,
      0.0265800525F,    0.038183894F,     -0.0228032731F,   0.036399167F,
      -0.0894152F,      0.0946445838F,    -0.0187807586F,   0.0133299073F,
      0.0167449173F,    -1.04611354E-5F,  3.0545365E-5F,    -1.26920168E-5F,
      -0.0647638217F,   0.109966189F,     0.132272407F,     -0.0890524611F,
      0.0958272815F,    0.0986132249F,    1.0953363E-5F,    -1.0341385E-5F,
      4.59963303E-5F,   -0.0940848F,      0.964406371F,     -0.837642431F,
      0.062125843F,     -0.134356499F,    0.0956945F,       -0.00114669965F,
      0.352657139F,     -0.399106294F,    0.0213641617F,    0.0819113478F,
      -0.207178369F,    0.128309727F,     0.0628767312F,    -0.0465586558F,
      0.0454222485F,    0.0576236099F,    -0.0780217126F,   -3.41292944E-5F,
      -3.77731776E-5F,  7.48085149E-5F,   -0.0447241738F,   0.0797507316F,
      -0.0789242685F,   -0.135972485F,    0.047766868F,     -0.0254774F,
      -0.204781905F,    -0.443274587F,    0.751903117F,     -0.17092815F,
      -0.139937F,       0.309981912F,     -0.00741634425F,  0.105069652F,
      -0.0416082703F,   -0.0411486365F,   -0.0277209841F,   -0.135735452F};
  static const float gateBias[1200] = {
      0.0485089049F,   -0.0132532613F,  0.0564957634F,   0.0807064101F,
      0.042324353F,    -0.0135997757F,  -0.0957013294F,  0.171617046F,
      0.534579754F,    0.153490767F,    -0.100320041F,   0.823895633F,
      0.284972101F,    0.0415751301F,   0.0697738752F,   -0.748881698F,
      0.0691329166F,   -0.0688167214F,  -0.0584669411F,  0.874345183F,
      0.399896652F,    0.119495705F,    0.0810392797F,   0.672726F,
      0.384472251F,    0.653101921F,    -0.264104515F,   -0.999828696F,
      -0.217282042F,   0.279905409F,    0.0634807944F,   0.474246562F,
      0.237595335F,    -0.328697264F,   -0.0129830493F,  0.0647291541F,
      1.07961714F,     0.310078084F,    -0.777840197F,   0.141428873F,
      -0.719246447F,   0.0623470768F,   0.346941203F,    -0.000198322203F,
      0.0434226245F,   0.237606287F,    0.601145089F,    -0.425141424F,
      0.308247149F,    -0.30835F,       1.38394451F,     0.179139957F,
      0.310581684F,    0.0459885485F,   0.04862101F,     0.293175936F,
      0.0662086F,      0.0834033489F,   -0.249079823F,   0.0525803678F,
      0.300388455F,    0.0556258336F,   0.64816916F,     -0.0109962504F,
      0.131611764F,    0.146853715F,    -0.0137245022F,  0.0461515896F,
      -0.11192894F,    -0.0768019482F,  0.166922599F,    0.487024128F,
      -0.0271951333F,  0.50147897F,     0.416888803F,    -0.479652137F,
      0.174295053F,    0.925703228F,    -0.0236552805F,  -0.0660945848F,
      0.39768818F,     0.195220783F,    -0.00889957231F, -0.233498067F,
      0.581983209F,    0.194986835F,    -0.870699644F,   0.115554653F,
      -0.0740606487F,  0.104407199F,    -0.940324605F,   0.0579227954F,
      -0.0010689937F,  -0.134951577F,   -0.297227085F,   0.0994785652F,
      -0.268795222F,   -0.171186447F,   -0.180300713F,   0.200956732F,
      -0.824909925F,   -0.0457959622F,  -0.0868751779F,  -0.218622625F,
      0.659265757F,    -0.46366027F,    0.959357142F,    0.761466861F,
      0.614136517F,    -0.00139298907F, 0.0234402549F,   0.136622772F,
      -1.104388F,      -0.655141115F,   0.00822720397F,  -0.291939169F,
      0.00305016874F,  0.288340449F,    0.546499193F,    -0.854094565F,
      -0.147963449F,   -0.414322495F,   -0.941138F,      -0.0431247838F,
      -0.799809456F,   0.847940922F,    -0.228374213F,   -0.47852087F,
      0.125767201F,    1.29268563F,     0.310029387F,    -0.622247159F,
      0.272060513F,    0.0322729573F,   0.156487659F,    -0.51627636F,
      -0.0135325193F,  0.780625522F,    -0.680645823F,   0.0894408152F,
      0.101609938F,    -0.134462178F,   0.11267F,        1.0282104F,
      0.278899699F,    0.208665416F,    0.450681478F,    0.090394035F,
      -0.00125627825F, -0.0119640818F,  0.011195397F,    0.183718741F,
      -1.26614368F,    -0.142803237F,   1.22318864F,     1.4104346F,
      -0.0182325952F,  0.364851445F,    0.0315123089F,   0.311135918F,
      0.133096904F,    0.109097473F,    0.584972322F,    0.199911326F,
      0.41496259F,     0.0166444127F,   0.334068269F,    0.233908236F,
      0.00540201692F,  0.141917825F,    0.614079237F,    0.0122578321F,
      0.0917050168F,   0.00964578427F,  -0.0273620542F,  0.0385070257F,
      0.0877709314F,   0.348076552F,    0.0166955013F,   0.0293015391F,
      -0.0904542282F,  -0.0362331644F,  0.288344204F,    0.0133788167F,
      -0.178532377F,   -0.0207345597F,  -0.647810042F,   -0.0109890476F,
      -0.158555388F,   -0.523716331F,   0.645686448F,    0.701061487F,
      0.168605775F,    0.289818704F,    0.0487789176F,   -0.848264217F,
      -0.933315873F,   0.321708173F,    0.13247712F,     0.757165253F,
      0.104258947F,    -0.474254668F,   -0.0661252663F,  -0.0475438945F,
      0.0450593568F,   0.0221636F,      0.108908124F,    0.637560606F,
      -0.0329423063F,  -0.168921441F,   -1.12476051F,    -0.0167986825F,
      -1.10842156F,    0.527124524F,    -0.579568446F,   0.196202919F,
      0.267760366F,    0.146746501F,    0.38389644F,     -0.143076494F,
      0.32945931F,     0.0323232971F,   0.0649765F,      0.529958904F,
      0.727165282F,    0.0334593281F,   0.608992934F,    0.0260319691F,
      0.0663885102F,   0.898582876F,    -0.0611877516F,  0.397060633F,
      -0.839509904F,   -0.828519166F,   -0.675268054F,   0.216541171F,
      -0.0526012443F,  0.444385052F,    -0.0534450971F,  -0.222525313F,
      0.432036042F,    0.922735035F,    -0.128907904F,   0.0701855868F,
      -0.0314379074F,  -0.996799529F,   0.475866735F,    -0.00390461856F,
      0.119705319F,    -1.4515388F,     -0.80429095F,    0.0371901914F,
      0.579009175F,    0.101365879F,    -0.128178507F,   0.743194938F,
      -0.502844036F,   0.137256801F,    0.0960816666F,   0.633303344F,
      -0.0440498032F,  -0.119581625F,   0.174000189F,    0.156893104F,
      -0.135471627F,   1.30219924F,     0.662229538F,    -0.454908222F,
      0.245298F,       -0.108593009F,   0.0200621225F,   0.769951224F,
      0.134393021F,    0.6828143F,      0.439196676F,    0.80962348F,
      -0.0234037936F,  0.117744297F,    0.110280469F,    0.0959960446F,
      0.314964473F,    1.01168215F,     0.0574581511F,   0.893654287F,
      -0.0765643418F,  -0.0584438108F,  0.135156408F,    0.0632240251F,
      0.0715739802F,   0.00241424749F,  0.602269769F,    0.0833551064F,
      -0.939863F,      0.225073114F,    0.00359266112F,  0.158378631F,
      0.665593624F,    -0.105963968F,   0.507071674F,    -0.0603973307F,
      0.0315942019F,   0.307236791F,    0.0722221583F,   0.202523306F,
      -0.0478825718F,  0.00162604195F,  0.42890662F,     -0.0222598501F,
      0.40804407F,     -0.0113170668F,  0.141518041F,    -1.10830295F,
      0.0186374187F,   1.08839214F,     0.979299724F,    0.0664952248F,
      0.716297448F,    0.115552083F,    0.0451217555F,   -0.698837757F,
      -0.0171285197F,  0.296311677F,    0.397786111F,    0.0701922104F,
      -0.0467603505F,  -0.12636438F,    0.0703033581F,   0.195287362F,
      0.080715172F,    0.692171276F,    -0.0346653387F,  0.181614354F,
      0.370846897F,    0.240686551F,    0.286218137F,    -0.884845138F,
      -0.295581788F,   -0.0503830127F,  -0.55849F,       0.240460113F,
      -0.00131306809F, 0.214004412F,    0.73211211F,     1.04300427F,
      -0.843380749F,   0.0471922904F,   -0.336517811F,   -0.00548924599F,
      0.63856107F,     0.111581273F,    0.214092702F,    0.876736939F,
      0.647572279F,    0.084052071F,    -0.571656883F,   0.465423286F,
      0.401297122F,    0.0409160256F,   0.883732855F,    0.862526894F,
      -0.127949238F,   0.0121112745F,   0.00410172902F,  0.653042555F,
      -0.428388F,      0.188044146F,    0.21508342F,     0.0938377157F,
      1.12512636F,     0.366718501F,    0.211700603F,    0.516754866F,
      0.602777F,       0.336754292F,    -0.479280233F,   0.00687113544F,
      0.175669625F,    0.752151F,       0.0452718139F,   0.0204104036F,
      -0.0242727F,     0.0540817752F,   -0.10961698F,    -0.411160499F,
      0.649588287F,    0.0644248128F,   -0.53815496F,    0.605030656F,
      0.698782921F,    0.162657261F,    1.3044045F,      -0.116199888F,
      0.0374658406F,   -1.06712258F,    0.0998202637F,   -0.13825196F,
      0.501190484F,    0.264257282F,    0.0315690637F,   0.332818657F,
      1.11377561F,     1.0492959F,      1.08563793F,     1.14109731F,
      0.949551404F,    1.10488057F,     0.99582F,        0.984823525F,
      1.01909363F,     0.951281607F,    1.03306472F,     1.02384365F,
      1.13890171F,     1.15944684F,     0.916324317F,    0.907465458F,
      1.00596833F,     0.942750573F,    1.03457069F,     1.00108838F,
      1.00953233F,     1.07114792F,     1.03895295F,     1.11476576F,
      0.966724F,       1.16316926F,     0.937526762F,    0.815540671F,
      0.946951F,       1.10906494F,     1.05760396F,     1.01719081F,
      0.961600959F,    0.943211913F,    1.08831978F,     1.08667958F,
      0.824969351F,    1.06350183F,     0.81934F,        0.909644723F,
      0.849971831F,    0.750763476F,    0.97646F,        1.02510715F,
      0.996615827F,    1.07153583F,     1.01380062F,     0.85983789F,
      0.957914591F,    0.825297236F,    1.02841949F,     1.02129066F,
      1.18266308F,     1.07092893F,     0.950178325F,    1.00521374F,
      1.1266948F,      1.01761258F,     1.00162017F,     1.01688552F,
      0.917680502F,    0.963329494F,    1.09081841F,     1.02988803F,
      0.952379167F,    1.03960049F,     1.00326395F,     1.08000863F,
      1.16258967F,     0.989562929F,    1.00840366F,     1.02109396F,
      1.04877591F,     1.00462127F,     1.0437001F,      0.973185F,
      0.976363719F,    0.940989077F,    1.0964154F,      1.08755589F,
      0.991830707F,    0.982484579F,    1.09338629F,     0.58952409F,
      1.08693397F,     1.0745703F,      0.829922855F,    0.980286598F,
      1.01619136F,     1.05973208F,     0.852175951F,    1.00241899F,
      1.11462009F,     1.09196F,        0.996244848F,    1.01139009F,
      1.05642033F,     1.09196889F,     0.966913402F,    0.999385238F,
      0.874028385F,    1.01694787F,     0.97208637F,     0.93578732F,
      1.08908105F,     0.906996131F,    1.02051711F,     1.10508335F,
      1.16704798F,     1.131652F,       1.02142894F,     1.03908539F,
      0.801565051F,    0.95161742F,     0.991587579F,    0.954098225F,
      1.18185961F,     1.06881559F,     1.02740443F,     0.785321414F,
      1.0393666F,      0.894141257F,    0.868195832F,    1.06575096F,
      0.811941504F,    0.925739706F,    0.995761871F,    0.916036487F,
      1.15528655F,     1.35105824F,     1.04955924F,     0.933601F,
      1.07641518F,     0.885969222F,    0.935593426F,    0.90739727F,
      1.0290997F,      1.18549228F,     0.877303958F,    1.06609941F,
      1.04262435F,     1.06441212F,     0.971950054F,    1.51410294F,
      0.966916919F,    1.06841767F,     0.722224772F,    1.04548669F,
      1.05747378F,     1.06453443F,     1.03550303F,     1.03692567F,
      0.709615F,       0.955679655F,    0.970366716F,    1.09469354F,
      1.00057125F,     1.00443721F,     1.02218711F,     1.16214323F,
      0.94793725F,     0.879002F,       0.958844F,       0.98530823F,
      1.06290781F,     0.992258847F,    1.05025F,        0.961239159F,
      1.0196557F,      1.04233563F,     0.927028537F,    0.95245558F,
      0.972024739F,    0.939154F,       0.958720386F,    1.05405593F,
      1.17115486F,     1.05076826F,     1.0065012F,      1.06561601F,
      1.00800276F,     1.02481675F,     1.14297354F,     1.10344982F,
      1.04829156F,     1.05604959F,     0.837862611F,    1.12781501F,
      1.04509068F,     0.874155104F,    1.17034173F,     0.926093042F,
      0.959864F,       0.953735352F,    1.11606944F,     0.763190806F,
      0.821631491F,    0.960521519F,    1.00333202F,     1.01609766F,
      1.10472703F,     0.875544369F,    1.05467331F,     1.02889788F,
      1.09257555F,     0.917381227F,    1.03660226F,     1.38533843F,
      1.09600425F,     0.909049928F,    0.789253712F,    1.00899184F,
      0.824792624F,    0.963303F,       0.893707156F,    0.938339174F,
      1.04669404F,     1.05407953F,     0.902765632F,    1.0646832F,
      1.09233868F,     1.00375807F,     1.02906525F,     1.29577076F,
      1.50173628F,     1.0009383F,      1.17262685F,     0.996481121F,
      0.984404087F,    1.03219104F,     1.01287413F,     1.20072353F,
      0.837777376F,    0.84768635F,     0.898921728F,    0.790223658F,
      1.04158425F,     1.14590013F,     1.09769344F,     0.969652116F,
      1.14977884F,     1.08880925F,     0.975976229F,    1.0862242F,
      1.00777948F,     0.849343717F,    0.926812947F,    1.06028926F,
      0.813713789F,    0.750337422F,    0.851901531F,    0.956805587F,
      0.951867938F,    0.914574623F,    1.0930382F,      0.942281127F,
      0.924884319F,    1.02514839F,     0.996632278F,    1.2131443F,
      0.987442911F,    1.06096792F,     1.01372933F,     1.09885776F,
      1.00431788F,     1.03976882F,     1.17627943F,     0.953812659F,
      1.05275202F,     0.964527607F,    1.22840679F,     1.1066246F,
      1.03798199F,     1.39045751F,     1.02540302F,     1.36805737F,
      1.06532371F,     1.03542507F,     1.02153158F,     1.00195396F,
      0.982217729F,    0.906584382F,    0.959703565F,    1.33643878F,
      1.06558061F,     1.07978153F,     1.05747795F,     1.04267943F,
      1.02973139F,     1.0549711F,      1.14154959F,     1.06562603F,
      0.850354552F,    1.01776683F,     0.942375839F,    0.909940302F,
      0.933704674F,    1.04994416F,     0.987954855F,    1.04119027F,
      0.955431402F,    1.10394597F,     1.16172814F,     1.0316174F,
      1.06877F,        1.0507952F,      1.16714859F,     1.05537701F,
      1.18852806F,     0.969032168F,    1.07443321F,     0.811088443F,
      0.856333673F,    1.26408339F,     1.4856348F,      1.07745731F,
      1.12471056F,     1.07913542F,     1.10591328F,     0.873948097F,
      0.975531161F,    1.15658069F,     0.998221874F,    1.07417583F,
      0.968925178F,    0.986151F,       0.97616154F,     1.11843109F,
      1.07289183F,     1.44227898F,     1.03140795F,     0.968134522F,
      0.939355731F,    1.0024507F,      1.1464175F,      0.816201866F,
      0.952215791F,    0.88560009F,     0.887773752F,    0.839684963F,
      0.950680196F,    1.12803984F,     0.971566081F,    1.3582232F,
      0.852780104F,    0.977627039F,    0.81700623F,     0.923062503F,
      1.04346049F,     0.897238195F,    0.944597781F,    1.43861151F,
      0.96557045F,     0.985646427F,    0.839151442F,    0.685572F,
      1.02262366F,     1.03046775F,     1.29124761F,     1.25497913F,
      0.997262299F,    1.20594335F,     1.0052079F,      1.15469897F,
      0.953140199F,    1.12921381F,     1.02022552F,     0.985875309F,
      0.92613095F,     0.994484127F,    1.13108134F,     1.47845423F,
      1.04594F,        1.04599154F,     0.888678432F,    1.067958F,
      0.97427541F,     1.04072595F,     1.06678486F,     1.19807804F,
      1.03041375F,     1.0182029F,      1.03269756F,     0.987149F,
      0.670929432F,    0.952915132F,    0.933710694F,    1.48626935F,
      1.25427377F,     1.0652529F,      1.24407F,        1.04671669F,
      1.04797328F,     0.769976795F,    1.02670205F,     1.06599247F,
      1.17472923F,     1.01407F,        1.04411614F,     1.07634652F,
      0.0445430055F,   0.0797401071F,   0.0710672289F,   -0.0167090409F,
      0.0642092526F,   -0.0397887826F,  -0.0840515271F,  0.296418816F,
      0.41725567F,     0.339245886F,    -0.134902149F,   0.811426342F,
      0.228470102F,    -0.00625629537F, 0.06841F,        -0.765628159F,
      0.0899009556F,   -0.05987975F,    -0.128566101F,   0.985267282F,
      0.575026751F,    0.054543931F,    0.146626905F,    0.559186816F,
      0.27276817F,     0.412055731F,    -0.300665021F,   -1.01792562F,
      -0.270046532F,   0.192994893F,    0.20643051F,     0.246259734F,
      0.101857886F,    -0.372860223F,   -0.0371747091F,  0.00352597609F,
      0.299441218F,    0.635504F,       -0.715941489F,   -0.014114337F,
      -0.765275955F,   0.111031167F,    0.0931341201F,   0.105495401F,
      0.0513638481F,   0.167606384F,    0.373087615F,    -0.317623734F,
      0.632383168F,    -0.452066809F,   1.3028419F,      0.276147187F,
      0.206453934F,    -0.055007454F,   0.262754261F,    0.527688205F,
      0.0411431454F,   0.0646731108F,   -0.216189668F,   0.028445866F,
      0.164165169F,    0.125789136F,    0.503366828F,    -0.00444853771F,
      0.353185505F,    0.159560904F,    -0.0314022377F,  0.00829675328F,
      -0.113283359F,   -0.0104648387F,  0.241999879F,    0.529208302F,
      0.0250917561F,   0.484163761F,    0.277536839F,    -0.51913631F,
      0.209207818F,    1.12132883F,     0.0423055291F,   -0.0555455685F,
      0.399983943F,    0.203585133F,    -0.00701125572F, -0.498466402F,
      0.517904639F,    0.0838759616F,   -0.890923619F,   0.0336486325F,
      -0.122289866F,   0.0831477791F,   -0.945426404F,   0.0289686378F,
      0.013493224F,    -0.124541439F,   -0.399986774F,   0.0420181192F,
      -0.325731516F,   -0.236541405F,   -0.117459029F,   0.195843756F,
      -0.865072072F,   -0.0776862428F,  -0.0946874321F,  -0.193256184F,
      0.407876313F,    -0.459699273F,   0.625605702F,    0.436805248F,
      0.303634822F,    0.0374142416F,   0.0110812811F,   0.166866034F,
      -1.12754667F,    -0.676681578F,   -0.0624453276F,  -0.360397935F,
      -0.0849400759F,  0.363104522F,    0.346791536F,    -0.883860052F,
      0.157054424F,    -0.407164037F,   -0.949789166F,   -0.0526442081F,
      -0.790808F,      0.722901523F,    -0.228186682F,   -0.518085361F,
      0.0391420387F,   0.363063484F,    0.240636975F,    -0.655557215F,
      0.493985802F,    -0.0196370129F,  0.471201122F,    -0.540260136F,
      0.0231621824F,   0.198368147F,    -0.674331367F,   0.207538784F,
      0.056208577F,    -0.150164485F,   0.131517425F,    0.309295326F,
      0.318048656F,    0.0629761443F,   0.106020458F,    0.118173979F,
      0.0756568611F,   0.291734248F,    0.155563653F,    0.0515361875F,
      -1.29481924F,    -0.0945048854F,  1.12741065F,     0.335623443F,
      -0.0536245257F,  0.109264158F,    -0.00894649234F, 0.265772879F,
      0.316356927F,    0.0407102406F,   0.687857687F,    0.387833923F,
      0.312719375F,    0.0168851428F,   0.191084877F,    0.213097543F,
      -0.0365159065F,  0.121115036F,    0.52934736F,     -0.065625757F,
      0.0528626703F,   -0.0253251791F,  -0.0511390083F,  0.0881918222F,
      -0.00459304685F, 0.168919921F,    0.0759030953F,   -0.00339651643F,
      -0.184876218F,   0.0531442575F,   0.192453817F,    0.0391204581F,
      -0.150505021F,   -0.0892611295F,  -0.64283818F,    -0.0526710153F,
      -0.203108937F,   -0.582494199F,   0.488393307F,    0.56449616F,
      0.32377544F,     -0.00725159468F, -0.010840497F,   -0.856507957F,
      -0.899059892F,   0.445012122F,    0.0679858923F,   0.686543345F,
      0.085790813F,    -0.49599731F,    -0.0720194429F,  -0.0360686518F,
      -0.003668305F,   0.148262084F,    0.115384109F,    0.594496608F,
      -0.0253497418F,  -0.191839635F,   -1.13750696F,    -0.0309165847F,
      -1.07151556F,    0.516963959F,    -0.598186493F,   0.0391855203F,
      -0.0298659075F,  0.0896676406F,   0.081732817F,    -0.0850655138F,
      0.371646702F,    -0.0176330134F,  0.0544660091F,   0.332392484F,
      0.314920723F,    0.0326187573F,   0.220611244F,    0.0711117387F,
      0.0117394971F,   0.65378505F,     -0.0748518556F,  0.213996246F,
      -0.849388F,      -0.825137436F,   -0.678844392F,   0.314019799F,
      0.0652110875F,   0.571830273F,    -0.0902526677F,  -0.195273459F,
      0.615545571F,    0.583600461F,    -0.143227503F,   0.0544860177F,
      -0.114927486F,   -0.99507767F,    0.172148198F,    0.0976999924F,
      0.0567534827F,   -1.45335865F,    -0.819874F,      0.000569724885F,
      0.102828257F,    0.34750703F,     -0.101267383F,   0.184650183F,
      -0.478656709F,   0.015116632F,    0.115227155F,    0.410688281F,
      -0.0223249458F,  -0.165612951F,   0.132188857F,    0.0265472624F,
      -0.120364323F,   1.21287584F,     0.183547869F,    -0.425125033F,
      0.400924F,       -0.03197762F,    -0.0347457789F,  0.499428183F,
      0.0930379778F,   0.162565708F,    0.0364261791F,   0.695189059F,
      -0.0224529728F,  0.190906599F,    0.0646108165F,   0.0714604184F,
      0.386222601F,    0.921059847F,    0.0119377179F,   0.523844481F,
      -0.119187571F,   -0.0632251725F,  0.039287094F,    0.0493056513F,
      0.090504393F,    -0.0128114223F,  0.501123607F,    0.185990319F,
      -0.955917299F,   0.112110198F,    -0.0671137273F,  0.477119058F,
      0.251860499F,    -0.0856995434F,  0.260767221F,    -0.0621420108F,
      0.118072927F,    0.184808895F,    0.0483396165F,   0.415464133F,
      -0.00819541793F, 0.0264552627F,   0.170576721F,    -0.105002292F,
      0.201267943F,    -0.0490788445F,  -0.0310013518F,  -1.06543481F,
      0.127952635F,    0.693832815F,    0.278511673F,    0.0598158576F,
      0.473407447F,    0.090099968F,    0.0125538046F,   -0.699671328F,
      0.254525691F,    0.150306955F,    0.257052839F,    -0.0367203F,
      -0.0457527302F,  -0.167254791F,   0.113589808F,    0.0544654913F,
      -0.0584598295F,  0.390850753F,    -0.00574914692F, 0.0841847062F,
      0.417334408F,    0.35756132F,     0.1514588F,      -0.886437416F,
      -0.369772136F,   -0.213300064F,   -0.569788277F,   -0.143678874F,
      0.0562920943F,   0.3615053F,      0.213496357F,    0.213713646F,
      -0.810762048F,   -0.0169213805F,  -0.318619639F,   0.139481097F,
      0.179366067F,    0.0767168179F,   0.123730734F,    0.394744575F,
      0.675607502F,    0.0874486938F,   -0.59299624F,    0.0539391115F,
      0.0574403144F,   0.0540020801F,   0.5603109F,      0.204614908F,
      -0.143932953F,   -0.0868314356F,  -0.0428376906F,  0.191617623F,
      -0.452748805F,   0.126060501F,    0.76788795F,     0.10972172F,
      0.668325067F,    0.28100118F,     -0.0136722364F,  0.549728155F,
      0.428408802F,    0.40923813F,     -0.493590564F,   0.0348901413F,
      0.129207328F,    0.492598414F,    0.0278346036F,   -0.118728526F,
      -0.0391880646F,  -0.0114071444F,  -0.169078544F,   -0.496632F,
      0.155411601F,    -0.00636211829F, -0.544875562F,   0.497778028F,
      0.189117357F,    0.364367038F,    0.333348572F,    -0.150494888F,
      0.00730657252F,  -1.07514834F,    0.0280140191F,   -0.105532393F,
      0.622836053F,    0.411517084F,    0.031910602F,    0.159972206F};
  static const float stateBias[400] = {
      -0.145745099F,    -0.0106598968F,   -0.10682556F,     0.0579995103F,
      0.0365910605F,    0.00341349235F,   0.000263779162F,  -0.00563360238F,
      0.0358509198F,    0.223087311F,     0.0497820601F,    -0.00637496123F,
      -0.0459538326F,   0.00236925273F,   -0.0373204537F,   0.000215604276F,
      0.0340005048F,    -0.00699464139F,  -0.116719835F,    -0.0305107515F,
      0.00140059483F,   -0.0411243476F,   -0.0242094323F,   -0.192817F,
      -0.0272951815F,   0.0464742519F,    -1.91636846E-5F,  -0.000907406211F,
      0.000988957705F,  0.0201888308F,    -0.0338766463F,   -0.0584567338F,
      0.129745126F,     -0.00104615418F,  -0.0455100648F,   0.106436707F,
      0.0574195385F,    0.0492358096F,    -0.000687985041F, -0.121464014F,
      0.00014116813F,   -0.00594811607F,  -0.061095424F,    -0.173450276F,
      -0.0376582034F,   -0.15243417F,     -0.0238082763F,   0.00083599909F,
      -0.0536377467F,   0.000296976417F,  0.00832696F,      -0.0844493657F,
      -0.142614529F,    0.0584465042F,    0.000476169516F,  0.0459887274F,
      0.0711979866F,    -0.0920142606F,   0.00127111364F,   0.0650379732F,
      -0.227840573F,    -0.00159688329F,  0.200910404F,     0.0208874904F,
      -0.225884899F,    0.0873756856F,    -0.0262036379F,   0.0200735666F,
      0.00679926528F,   -0.106468633F,    -0.0511487462F,   -0.0131556736F,
      -0.137579739F,    0.0242709666F,    0.168528035F,     8.89303046E-5F,
      0.0572478F,       0.00795040559F,   0.0441310108F,    0.191565871F,
      -0.0174469016F,   0.0114282425F,    -0.10765186F,     0.000503659772F,
      -0.186846226F,    0.117804721F,     0.000466650235F,  -0.0219686776F,
      0.0434746705F,    0.0107922768F,    0.000445397833F,  0.0433998406F,
      -0.104422666F,    -0.0714622F,      -0.000258134271F, 0.0811969414F,
      0.000867933326F,  -0.00101234904F,  0.00213358575F,   0.0369684547F,
      0.00150444568F,   0.00419129664F,   -0.0475800969F,   0.00103824399F,
      -0.0425262F,      -0.000498203852F, -0.0343513191F,   -0.0493360311F,
      0.220076621F,     -0.176808327F,    0.0221911222F,    -0.287105054F,
      -0.00117957091F,  -0.00049742026F,  -0.163667291F,    0.000769796898F,
      -0.030626595F,    -0.0119586978F,   -0.0107725896F,   -0.000997722498F,
      0.132225499F,     -0.000116561416F, 0.000238000881F,  0.113854267F,
      0.000114735936F,  -0.028752571F,    0.00023560453F,   -0.000960820937F,
      0.0184612293F,    0.263059527F,     -0.108453006F,    0.00046035173F,
      0.132964656F,     -0.000703983707F, 0.0481226295F,    0.000394025235F,
      0.0242795981F,    0.191622689F,     0.00181201054F,   0.0143835507F,
      -0.0551525205F,   0.0166168027F,    -0.0104974015F,   0.249986231F,
      0.129792184F,     -0.0428975411F,   0.204840764F,     0.0860822424F,
      -0.150293931F,    -0.157329634F,    0.054906223F,     0.0327347964F,
      0.000457379618F,  -0.0786861703F,   -0.0226995908F,   0.176867202F,
      0.0137529215F,    0.253870845F,     0.0872767717F,    0.133887306F,
      -0.070562087F,    0.00597516727F,   0.0265221875F,    0.0213555228F,
      -0.0686227307F,   0.0241124853F,    -0.0945385695F,   -0.0469461419F,
      -0.0350634605F,   -0.255761713F,    -0.0286709256F,   -0.035300523F,
      -0.0670094341F,   0.0995987952F,    0.0855744481F,    -0.00825928897F,
      -0.010283038F,    -0.0534754768F,   -0.0753929466F,   0.0213150121F,
      -0.000760168303F, 0.0156466272F,    -0.116127163F,    0.0821871608F,
      -0.113564663F,    0.01180562F,      -0.000903685694F, 0.0311549939F,
      0.0107942279F,    0.000455794885F,  0.0448610447F,    0.0289231949F,
      0.0733184516F,    0.0891448781F,    -0.0425261334F,   0.000444096659F,
      0.000349958631F,  -0.0268413927F,   0.0515025F,       -0.0493338369F,
      -0.0479245819F,   -0.000156607639F, -0.0495322272F,   -0.0187449716F,
      0.0437417068F,    0.0743142813F,    0.122695103F,     0.0777046233F,
      0.108470775F,     -0.087119855F,    -0.00020643069F,  -0.0276091564F,
      0.000433697714F,  0.049820777F,     -0.000262813701F, 0.0284580234F,
      -0.161848694F,    0.112371311F,     -0.180831686F,    0.0242815018F,
      -0.069196716F,    -0.194786191F,    0.0648681745F,    0.0746360868F,
      0.040397048F,     0.0697149F,       -0.163533255F,    -0.132984489F,
      0.0597832315F,    0.0369765684F,    0.000428731058F,  -0.162693053F,
      0.00026420303F,   -0.00103064242F,  0.00096577924F,   -0.107133009F,
      -0.0527968109F,   -0.0327251032F,   -0.0513313226F,   -0.0179080423F,
      -0.0427305773F,   0.0479277112F,    -0.103587031F,    0.0584281087F,
      -0.0921904147F,   -4.07324223E-5F,  -0.0443686359F,   0.101013064F,
      0.162308767F,     0.00151471072F,   0.000848835916F,  0.00762343314F,
      0.0844775F,       0.14666371F,      0.025592139F,     -0.0264971517F,
      0.000504719443F,  -0.053309422F,    -0.0557964034F,   0.0555136725F,
      -0.0401859768F,   -0.000448224077F, -0.0529542193F,   0.0490076505F,
      0.115605064F,     -0.0312616825F,   0.141438439F,     -0.000113714603F,
      -0.047406327F,    0.012295221F,     -0.0150107099F,   0.0482403897F,
      0.0513093509F,    0.118547797F,     -0.198547244F,    0.0554621369F,
      -0.0840641335F,   -0.0899968F,      -0.0171371065F,   -0.0544679761F,
      0.016969692F,     0.0256874096F,    0.0894574448F,    0.0266258959F,
      -0.000251719583F, -0.103717394F,    -0.0227729473F,   0.086216256F,
      -0.0731164292F,   0.0785691813F,    0.247748122F,     -0.0399363339F,
      -0.000962359889F, 0.192492351F,     -0.0831030905F,   -0.110613495F,
      -0.0323043577F,   0.0712903365F,    0.108091734F,     -0.15206416F,
      0.113177687F,     -0.176594615F,    0.0925376862F,    -0.0212075636F,
      0.00349825807F,   3.67129105E-5F,   0.0844794065F,    0.0261240378F,
      0.175830796F,     0.0938588604F,    -0.0336019136F,   0.000378261466F,
      0.0649585426F,    -0.0138378739F,   -0.0424016528F,   -0.0622755364F,
      0.0340800397F,    0.166572034F,     0.0370907038F,    0.000269515731F,
      -0.0938120857F,   0.0596025325F,    -0.0144301821F,   -0.089256309F,
      0.0138121899F,    0.0048189424F,    -0.12045566F,     -0.120481506F,
      -0.0253322516F,   -0.0331297442F,   -0.0545514748F,   -0.0102462647F,
      0.0178123526F,    -0.263798654F,    0.158105358F,     0.000114681352F,
      -0.000191989486F, -0.0512445793F,   0.000643619918F,  0.107385427F,
      0.118639991F,     -0.0300023556F,   -0.0203962401F,   -0.168624699F,
      7.51136467E-5F,   -0.0785272792F,   -0.000244988769F, -0.0472170636F,
      0.00393169047F,   0.0193861183F,    0.0947562531F,    -0.0315365456F,
      -0.0160657503F,   -0.00903128926F,  0.000862391142F,  -0.17356354F,
      -0.0256899484F,   0.00669600628F,   -0.0156824328F,   0.232449099F,
      -0.0612140261F,   -0.0577927269F,   0.116432399F,     -0.142163485F,
      0.000317862316F,  0.0962519348F,    -0.0211936682F,   -0.0470336452F,
      -0.0339923F,      0.0381001309F,    -0.0497916639F,   -0.0290534701F,
      0.0523443483F,    -0.132393837F,    -0.000862808083F, -0.14818871F,
      -0.0621642359F,   -0.0515127145F,   -0.0208840445F,   -0.0304163843F,
      0.0580832139F,    0.0356923603F,    -0.0842844695F,   0.000489866594F,
      -0.0930224285F,   0.00643320708F,   -0.000587825663F, -0.0486513041F,
      -0.0801074132F,   -0.0199385229F,   0.203195184F,     -0.0745318F,
      0.0465927608F,    -0.00107753358F,  -0.0217023958F,   0.133005708F,
      0.0243954565F,    0.034340281F,     0.0343407802F,    -0.0673794299F};
  static float recurrentGateWeights[480000];
  static float recurrentStateWeights[160000];
  static boolean_T bufferInitialized;
  __m128 r;
  __m128 r1;
  __m128 r2;
  __m128 r3;
  float Z[1200];
  float gateValues[1200];
  float CS[400];
  float YT[400];
  float d_B[400];
  float stateValues[400];
  float inMinibatch_0_f1_data[10];
  float B[5];
  float b_B[5];
  float c_B[5];
  float e_B[5];
  float layerOutput[3];
  int i;
  if (!bufferInitialized) {
    readDnnConstants(
        &recurrentGateWeights[0],
        "../docs/largeDnnConstants_1539771.bin", 480000);
    readDnnConstants(
        &recurrentStateWeights[0],
        "../docs/largeDnnConstants_1539776.bin", 160000);
  }
  bufferInitialized = true;
  for (i = 0; i < 10; i++) {
    inMinibatch_0_f1_data[i] = (float)inputs->f1[0].f1[i];
  }
  if (!obj->IsNetworkInitialized) {
    memset(&obj->InternalState[0].f1[0].f1[0], 0, 400U * sizeof(float));
    memset(&obj->InternalState[0].f1[1].f1[0], 0, 400U * sizeof(float));
    obj->IsNetworkInitialized = true;
  }
  matrixMultiply1(1200, 400, 1, 128, 128, 128, &recurrentGateWeights[0],
                  &obj->InternalState[0].f1[0].f1[0], &Z[0]);
  for (i = 0; i <= 1196; i += 4) {
    r = _mm_loadu_ps(&Z[i]);
    _mm_storeu_ps(&Z[i], _mm_add_ps(r, _mm_loadu_ps(&gateBias[i])));
  }
  for (i = 0; i < 5; i++) {
    B[i] = inMinibatch_0_f1_data[i];
  }
  matrixMultiply1(1200, 5, 1, 128, 128, 128, &inputGateWeights[0], &B[0],
                  &gateValues[0]);
  for (i = 0; i <= 1196; i += 4) {
    r = _mm_loadu_ps(&gateValues[i]);
    r1 = _mm_loadu_ps(&Z[i]);
    _mm_storeu_ps(&gateValues[i], _mm_add_ps(r, r1));
  }
  lambdaForColumnMajorGeneric(gateValues);
  matrixMultiply1(400, 400, 1, 128, 128, 128, &recurrentStateWeights[0],
                  &obj->InternalState[0].f1[0].f1[0], &YT[0]);
  for (i = 0; i <= 396; i += 4) {
    r = _mm_loadu_ps(&YT[i]);
    _mm_storeu_ps(&YT[i], _mm_add_ps(r, _mm_loadu_ps(&stateBias[i])));
  }
  for (i = 0; i < 5; i++) {
    b_B[i] = inMinibatch_0_f1_data[i];
  }
  matrixMultiply1(400, 5, 1, 128, 128, 128, &inputStateWeights[0], &b_B[0],
                  &stateValues[0]);
  for (i = 0; i <= 396; i += 4) {
    r = _mm_loadu_ps(&stateValues[i]);
    r1 = _mm_loadu_ps(&YT[i]);
    _mm_storeu_ps(&stateValues[i], _mm_add_ps(r, r1));
  }
  c_lambdaForColumnMajorGeneric(stateValues);
  for (i = 0; i <= 396; i += 4) {
    r = _mm_loadu_ps(&stateValues[i]);
    r1 = _mm_loadu_ps(&gateValues[i]);
    r2 = _mm_loadu_ps(&gateValues[i + 400]);
    r3 = _mm_loadu_ps(&obj->InternalState[0].f1[1].f1[i]);
    r = _mm_add_ps(_mm_mul_ps(r, r1), _mm_mul_ps(r2, r3));
    _mm_storeu_ps(&CS[i], r);
    _mm_storeu_ps(&stateValues[i], r);
  }
  e_lambdaForColumnMajorGeneric(stateValues);
  for (i = 0; i <= 396; i += 4) {
    r = _mm_loadu_ps(&stateValues[i]);
    r1 = _mm_loadu_ps(&gateValues[i + 800]);
    _mm_storeu_ps(&YT[i], _mm_mul_ps(r, r1));
  }
  matrixMultiply1(1200, 400, 1, 128, 128, 128, &recurrentGateWeights[0], &YT[0],
                  &Z[0]);
  for (i = 0; i <= 1196; i += 4) {
    r = _mm_loadu_ps(&Z[i]);
    _mm_storeu_ps(&Z[i], _mm_add_ps(r, _mm_loadu_ps(&gateBias[i])));
  }
  for (i = 0; i < 5; i++) {
    c_B[i] = inMinibatch_0_f1_data[i + 5];
  }
  matrixMultiply1(1200, 5, 1, 128, 128, 128, &inputGateWeights[0], &c_B[0],
                  &gateValues[0]);
  for (i = 0; i <= 1196; i += 4) {
    r = _mm_loadu_ps(&gateValues[i]);
    r1 = _mm_loadu_ps(&Z[i]);
    _mm_storeu_ps(&gateValues[i], _mm_add_ps(r, r1));
  }
  lambdaForColumnMajorGeneric(gateValues);
  memcpy(&d_B[0], &YT[0], 400U * sizeof(float));
  matrixMultiply1(400, 400, 1, 128, 128, 128, &recurrentStateWeights[0],
                  &d_B[0], &YT[0]);
  for (i = 0; i <= 396; i += 4) {
    r = _mm_loadu_ps(&YT[i]);
    _mm_storeu_ps(&YT[i], _mm_add_ps(r, _mm_loadu_ps(&stateBias[i])));
  }
  for (i = 0; i < 5; i++) {
    e_B[i] = inMinibatch_0_f1_data[i + 5];
  }
  matrixMultiply1(400, 5, 1, 128, 128, 128, &inputStateWeights[0], &e_B[0],
                  &stateValues[0]);
  for (i = 0; i <= 396; i += 4) {
    r = _mm_loadu_ps(&stateValues[i]);
    r1 = _mm_loadu_ps(&YT[i]);
    _mm_storeu_ps(&stateValues[i], _mm_add_ps(r, r1));
  }
  c_lambdaForColumnMajorGeneric(stateValues);
  for (i = 0; i <= 396; i += 4) {
    r = _mm_loadu_ps(&stateValues[i]);
    r1 = _mm_loadu_ps(&gateValues[i]);
    r2 = _mm_loadu_ps(&gateValues[i + 400]);
    r3 = _mm_loadu_ps(&CS[i]);
    r = _mm_add_ps(_mm_mul_ps(r, r1), _mm_mul_ps(r2, r3));
    _mm_storeu_ps(&CS[i], r);
    _mm_storeu_ps(&YT[i], r);
  }
  e_lambdaForColumnMajorGeneric(YT);
  for (i = 0; i <= 396; i += 4) {
    r = _mm_loadu_ps(&YT[i]);
    r1 = _mm_loadu_ps(&gateValues[i + 800]);
    _mm_storeu_ps(&YT[i], _mm_mul_ps(r, r1));
  }
  matrixMultiply2(3, 400, 1, 128, 128, 128, &A[0], &YT[0], &layerOutput[0]);
  layerOutput[0] += 0.063112095F;
  layerOutput[1] += 0.00391856208F;
  layerOutput[2] -= 0.0430706441F;
  SoftmaxLayer_predict(layerOutput, outputData);
}

/*
 * File trailer for predictForRNN.c
 *
 * [EOF]
 */
