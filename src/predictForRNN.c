/*
 * File: predictForRNN.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 12-Dec-2024 16:04:45
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
      -0.000811605947F, -0.000403957471F, -0.00243100966F,  -0.00382114528F,
      -1.61899629E-6F,  8.94312834E-5F,   3.33629373E-8F,   -0.00425082352F,
      0.00117815973F,   0.000285012764F,  -0.000811230741F, -0.00724314759F,
      -0.000264779141F, -0.00258482061F,  0.00162944873F,   8.97908249E-5F,
      -0.000722732279F, 6.67564E-5F,      -0.000458168564F, -0.00289038289F,
      0.00118018349F,   -0.00354100973F,  -0.00536254933F,  0.00176362577F,
      -0.00750035793F,  -0.00060664257F,  -0.000997146126F, -0.0116265547F,
      -0.00482856948F,  -0.00205494091F,  -7.65724544E-5F,  0.000909248949F,
      -0.00269363518F,  -0.00750014372F,  0.00344320945F,   -0.00143878628F,
      -4.41497E-5F,     -2.53833714E-5F,  0.00156619423F,   6.4109925E-5F,
      0.00199011597F,   -0.00476080133F,  -0.000650383416F, -0.00143824052F,
      0.00153224671F,   0.000200324575F,  0.000473109016F,  0.00872225F,
      -0.00109102495F,  0.00278788828F,   -0.000165615755F, 0.000912846881F,
      -4.58752729E-5F,  0.000656165648F,  -8.32701626E-5F,  -0.00211699493F,
      3.13232704E-5F,   -0.00015071973F,  -0.00205210666F,  -3.14534554E-5F,
      -0.00257907459F,  -0.000839801331F, 0.000303567096F,  0.000858099433F,
      -0.000419144606F, 0.00110944198F,   -0.000466345955F, 0.00601498084F,
      -0.00128040288F,  -0.0017145155F,   -0.00270682387F,  -4.08081432E-5F,
      0.000802380615F,  0.00108201848F,   0.00547708385F,   3.56709029E-6F,
      0.000848850817F,  0.000112977606F,  0.0191977564F,    -1.78888968E-6F,
      0.001068527F,     -0.00191915617F,  -0.000542698079F, -0.000778352842F,
      -0.00399637595F,  -0.000301109743F, -0.000833869795F, -4.78726815E-6F,
      0.000922654697F,  -0.000965991348F, 0.00956691802F,   -6.71592034E-7F,
      0.00641599111F,   0.00158196315F,   -0.00165505824F,  -0.00164006534F,
      -0.000922036357F, -0.0122742346F,   -0.00288693188F,  -0.00179624034F,
      -0.00144492928F,  0.00200292049F,   0.00038812781F,   0.00243068673F,
      -0.000384585699F, -0.00131170952F,  -0.0133962277F,   -0.000903941866F,
      -0.0105738929F,   -1.36190774E-5F,  -0.00137141324F,  -0.00745969266F,
      -0.00398253417F,  0.00192800432F,   -0.00624481402F,  0.00340897264F,
      -0.00877490267F,  0.00107280724F,   8.23896244E-6F,   1.90006795E-5F,
      -0.000297067629F, 0.000141839613F,  -0.00270557613F,  -8.70891818E-5F,
      -1.74382512E-5F,  2.75350067E-6F,   -0.0216990504F,   8.67056224E-5F,
      -0.0078932941F,   -0.00243158638F,  0.00200448371F,   -0.00421523768F,
      0.00108560862F,   1.69544592E-6F,   0.0149881607F,    0.000174406043F,
      -0.00089146645F,  -0.029095266F,    -0.00348802F,     0.00324869575F,
      -0.0134712942F,   -0.000393848866F, -0.000382144848F, 0.00499655865F,
      -0.00390520808F,  -0.0143280849F,   -0.0170159284F,   0.00143948745F,
      -0.00839268696F,  7.617E-5F,        -0.00247039157F,  -3.76980388E-5F,
      -0.0126846023F,   0.00071061874F,   0.000569965632F,  0.000760204915F,
      -8.71344218E-6F,  -0.00246503623F,  -2.8254788E-5F,   0.00113789295F,
      -0.0192671493F,   -5.61246566E-7F,  -4.27587293E-5F,  -5.76605162E-5F,
      0.00314563443F,   -0.000292386336F, 0.00187341729F,   -0.00175093219F,
      -0.00920009706F,  0.00645841891F,   -0.00190291763F,  -0.00525206327F,
      0.000170764935F,  -0.0051871459F,   -0.000854811049F, -0.00232053944F,
      0.000217197885F,  -0.007993415F,    0.000167308128F,  -0.00017179655F,
      -0.00069077F,     0.000195302957F,  -0.00406250637F,  -0.000487260608F,
      -0.00466525508F,  -0.00457048F,     6.33207455E-5F,   -0.0177318417F,
      -0.001868158F,    -0.0034441324F,   0.011472363F,     -0.0108168544F,
      0.00105403725F,   -0.000770160696F, 0.00436559506F,   0.000644307933F,
      0.000752061489F,  0.0016595393F,    -8.42808458E-6F,  -0.00146652537F,
      -0.016702015F,    8.31717716E-6F,   -0.00506815035F,  -0.00175201625F,
      -0.000143088153F, -0.00729175285F,  8.3304E-6F,       7.93801E-6F,
      0.00237559131F,   0.00630716048F,   -0.00536399335F,  0.00410625711F,
      0.00445362646F,   -0.00364957773F,  -0.000552527F,    -0.000721465272F,
      -0.0205494743F,   -0.00212246971F,  4.41369593E-6F,   -0.011017411F,
      -0.00364588737F,  0.00188267208F,   -0.00330484216F,  0.000617115758F,
      -0.00467597088F,  0.000911520037F,  0.00586306769F,   -0.00307324366F,
      0.0010423233F,    0.000501304283F,  -0.00309185148F,  -0.00420717057F,
      0.000252720696F,  -0.00433394825F,  0.0320770554F,    -0.000810697966F,
      -0.00100285269F,  0.00127381389F,   -0.00421597F,     -0.0132333869F,
      0.00924789347F,   -3.01955042E-5F,  -0.0017936381F,   -0.00629194966F,
      0.00310924463F,   0.00210362556F,   0.00228453963F,   -0.00128047599F,
      0.000490717415F,  -0.000878808612F, -0.00242064195F,  9.85230486E-7F,
      -0.00412050541F,  -0.00421520695F,  -0.0108643984F,   -2.54645693E-6F,
      0.00265787682F,   0.00266699796F,   -0.012327808F,    0.00526723638F,
      0.000719826785F,  0.000910493545F,  -0.00204292685F,  -0.000629368878F,
      4.45231899E-6F,   -0.000880324F,    0.000793417043F,  -0.00177949714F,
      0.00126109668F,   -0.00467776274F,  -0.00124699401F,  -0.00387738692F,
      -4.50263833E-5F,  -0.0021794159F,   0.00236567738F,   0.0402653404F,
      3.16186788E-6F,   -0.00423286483F,  -0.000512570725F, -0.00147495267F,
      2.60538582E-6F,   0.0002340629F,    -0.00420566043F,  0.00110991602F,
      -0.000987997F,    0.00204872806F,   -0.00046921274F,  0.000477836933F,
      -0.00619898969F,  -1.32577316E-6F,  -0.000911664742F, 0.000740749296F,
      0.00134773948F,   0.000356668519F,  -0.00101690216F,  -0.00582189392F,
      -0.000109753295F, 0.000211407358F,  -0.000429577776F, 0.00546530867F,
      0.00132332661F,   -0.00350617012F,  -0.00282140449F,  -0.000563936424F,
      8.49358184E-6F,   0.00376087753F,   -0.00164849241F,  -9.77022137E-6F,
      9.86206796E-5F,   -0.00163003604F,  0.000961513259F,  2.17204324E-5F,
      0.000889442279F,  -0.00200066692F,  0.00219748635F,   -0.000264933F,
      3.27803682E-5F,   0.000112305403F,  -0.00109216128F,  7.83550513E-6F,
      -0.00147194078F,  -0.000386311905F, -0.0067365896F,   -0.00144044787F,
      0.00102290057F,   -0.00197097636F,  0.00126772863F,   0.000304743706F,
      -0.000149174026F, 0.00581812207F,   0.00347587373F,   0.000638974714F,
      0.00573324598F,   -0.000721504621F, 0.00735181663F,   0.00508338353F,
      -0.00079733436F,  0.00590155972F,   -0.00139975F,     0.000223178053F,
      -0.017197445F,    0.00138708483F,   0.00198702631F,   -0.000357715908F,
      0.00834593736F,   0.000662176928F,  -0.00143365178F,  0.000239587578F,
      -0.00252752309F,  -0.000405279046F, -0.00196682988F,  -0.0526564904F,
      2.09116308E-7F,   1.84966175E-5F,   0.00208460912F,   -0.0049990993F,
      -0.00153592939F,  -0.00217976142F,  0.00137076678F,   -0.00504119089F,
      -0.00764889177F,  -0.00695648836F,  0.00287892297F,   -0.00640079333F,
      -0.000941274222F, -2.07167141E-5F,  4.51033975E-6F,   -6.72665774E-5F,
      -0.00554905413F,  -0.00759694213F,  1.45602771E-5F,   -0.00225794129F,
      5.01600152E-5F,   0.00210504816F,   -0.000236742271F, 1.56753395E-5F,
      0.00058416347F,   1.20942204E-5F,   2.6500471E-5F,    1.82370741E-7F,
      -0.000187352882F, 0.00033283548F,   1.59073056E-6F,   -0.00135960849F,
      -0.000113003458F, -0.00062688085F,  -0.00190243125F,  0.00105367391F,
      0.000124148515F,  0.00194192713F,   0.000295474136F,  -0.000529131328F,
      -0.00266979425F,  -0.000748379F,    -1.00908437E-6F,  -0.00230876752F,
      0.00046129516F,   0.00184460951F,   0.00173564104F,   -0.000515196298F,
      -0.00014261168F,  0.000451887754F,  -0.000650619215F, -0.00239279517F,
      3.33026094E-7F,   -0.000644662825F, 1.94168464E-7F,   -0.00160344341F,
      0.00578579679F,   1.89723651E-5F,   -0.000120462428F, -0.00150031981F,
      0.000384779589F,  -0.00758471387F,  -0.00306786F,     9.10715753E-5F,
      0.000395086652F,  8.08577624E-7F,   -0.00165234099F,  0.000297687366F,
      -7.7729128E-5F,   -0.00158998289F,  0.00111567031F,   0.000756858615F,
      -0.00189930841F,  0.000165515579F,  0.000165742254F,  -0.000606068643F,
      -0.00150391879F,  -0.00198520347F,  -1.50397636E-6F,  0.000577893399F,
      -0.000767694903F, -0.000283680594F, 0.000543883536F,  -0.000703565544F,
      -8.15253827E-7F,  -3.11458223E-7F,  0.000276491832F,  0.000115314906F,
      -0.00378054962F,  -0.00126468565F,  -0.000267570693F, -0.00165691797F,
      0.00428311573F,   0.00136005017F,   -0.00193680625F,  0.00331872725F,
      0.000398474251F,  0.0130816931F,    6.77057733E-6F,   -0.000400098827F,
      -1.56202009E-6F,  -0.000719008269F, 7.35082431E-5F,   -6.14573873E-5F,
      3.76121679E-5F,   -5.11929138E-5F,  -0.000980286F,    -1.87690148E-7F,
      -0.000606924645F, -0.000931074144F, -0.00013314806F,  0.000270338962F,
      8.15370186E-6F,   0.00888133422F,   6.20551145E-5F,   0.00766234659F,
      -0.000573340803F, -0.00023898542F,  -0.000986089581F, -1.91030199E-6F,
      -0.000222552597F, 0.00929437391F,   -0.00629347796F,  -5.66376315E-8F,
      0.0111953896F,    -0.000158965049F, 0.00551053416F,   1.26934268E-8F,
      0.0123075927F,    -0.00124351669F,  -0.00292155F,     -0.000379053119F,
      -4.62019416E-5F,  -0.000138360847F, -0.000680214085F, -1.89772095E-7F,
      0.0123702716F,    -0.00014069106F,  -0.0012336747F,   -1.48647587E-7F,
      0.00131486F,      -0.00022802077F,  0.000190068982F,  -0.000668313529F,
      0.000139561715F,  -0.0047541284F,   -0.000198949245F, 9.53895287E-5F,
      -0.000370188529F, -0.000309302297F, 7.29186504E-5F,   0.00331131881F,
      6.27085537E-5F,   5.12397528E-5F,   -0.00133802113F,  -0.0129850926F,
      -0.00232384773F,  7.95079541E-6F,   0.000107047716F,  -0.00180797349F,
      -0.000651682843F, 0.00257152971F,   -0.000841458677F, 0.00596895907F,
      -0.00225629727F,  -0.00188369118F,  9.5196441E-8F,    -1.2867124E-6F,
      2.67723444E-5F,   -5.34412975E-5F,  -0.000374720839F, 1.91901199E-5F,
      -1.55552172E-7F,  -2.56239701E-7F,  -0.00111451687F,  -2.74797385E-5F,
      -0.00115484663F,  -0.000386414409F, -0.000161124408F, -0.000318843522F,
      0.0125539303F,    4.1508514E-7F,    0.00291154417F,   -0.000378979516F,
      6.91920141E-5F,   -0.00353199337F,  -2.84090802E-5F,  -0.002148428F,
      -0.0358575881F,   0.000263321592F,  9.35499193E-5F,   0.00251562265F,
      -0.00115710613F,  -0.000480831979F, -0.00159233692F,  0.000873686164F,
      -0.00206386927F,  -0.000281714136F, -0.000790852588F, -7.20734647E-7F,
      -0.00109115406F,  0.000320314284F,  0.000101897145F,  0.0191060193F,
      -1.26713599E-6F,  -0.000196462104F, -6.78992456E-6F,  -0.00202317326F,
      -0.00260305521F,  6.79560372E-8F,   -1.30943505E-8F,  -3.18288926E-7F,
      0.000292524346F,  3.44303699E-5F,   0.00290977093F,   -0.00195633643F,
      -0.0047193463F,   0.00731484871F,   -0.000117639567F, -0.000554707949F,
      0.000105244195F,  -0.00118221925F,  -0.00077762123F,  0.000990227913F,
      0.0116961971F,    -0.00110797922F,  0.000242607217F,  -0.000159077274F,
      0.000154975394F,  -0.000126425613F, -0.000364270469F, -0.000879781262F,
      -0.000195086497F, -0.000620274164F, -0.00042732872F,  -0.00298540411F,
      -0.000394841016F, -0.00127542717F,  0.000980596757F,  -0.00178104138F,
      0.00068550976F,   0.000129752429F,  0.00111229403F,   -2.02965221E-5F,
      0.00295794546F,   0.007908375F,     -4.944377E-7F,    -0.00119032722F,
      -0.00176996214F,  3.3249588E-8F,    -0.000405175553F, 0.00127139525F,
      -0.000176477901F, -0.000768122612F, -7.41921582E-8F,  -9.67997522E-8F,
      0.0167754423F,    0.000491204206F,  -0.00231744535F,  0.000554278202F,
      0.00786675513F,   -0.00124292052F,  0.0108619565F,    0.000669633388F,
      -0.00639148057F,  -8.13798324E-5F,  -2.96530231E-8F,  -0.00120318541F,
      -0.000579812622F, 0.00642213924F,   -0.00011562679F,  0.00351559953F,
      -0.0015330622F,   0.00786566548F,   0.0016404792F,    -0.000253595121F,
      -0.00419046218F,  0.000165636666F,  -0.00176428992F,  -0.000851699966F,
      -0.000477203837F, -0.000326357549F, 0.00570593774F,   -7.82384886E-5F,
      -0.000161923861F, 5.02669E-5F,      -0.00053282F,     -0.00424104603F,
      0.000396475429F,  -2.50267271E-6F,  0.000120643635F,  -0.00204919977F,
      -0.00041275876F,  0.000769685372F,  0.00418855436F,   -0.00150115555F,
      5.37781E-5F,      0.000158021794F,  -0.000442112039F, 3.69502402E-8F,
      -0.000606196816F, -0.00179795758F,  -0.00540665537F,  -5.75662852E-7F,
      0.00870712F,      0.00148048345F,   -0.00277970312F,  -0.00397114083F,
      0.00363991084F,   0.000233072918F,  0.000809254299F,  0.000110961344F,
      -2.1880453E-10F,  -0.00111917499F,  -7.99785485E-5F,  -0.00110256637F,
      0.0143343061F,    -0.0009974F,      -0.00189436448F,  -0.000437969487F,
      8.30399358E-8F,   -0.000884850509F, 0.000687210239F,  0.00505657867F,
      -4.42468973E-7F,  -0.000901515363F, -0.00177047623F,  -0.00097598508F,
      1.50889662E-7F,   0.000165444115F,  -0.000719634641F, 0.0129850926F,
      -0.000686098123F, 0.000850242272F,  0.00219735201F,   -0.000208899117F,
      -0.000262733316F, 1.51694876E-5F,   0.000175611203F,  0.0077589117F,
      0.00075067475F,   0.00991781708F,   -0.000313583F,    -0.0158041436F,
      7.4094889E-5F,    0.000193189175F,  -0.000258028274F, -0.000630856259F,
      -0.00180789409F,  -0.00569119724F,  0.000214955435F,  -8.92779499E-5F,
      0.000206874873F,  0.000155327769F,  -0.000957209209F, -2.71123383E-7F,
      8.41224755E-5F,   -0.000454080058F, 0.00130374578F,   -3.28658132E-7F,
      6.77177522E-5F,   -0.000290184369F, 0.00196300074F,   -0.000132500514F,
      -1.04030619E-7F,  4.86098215E-5F,   -0.000782931922F, -4.07849683E-7F,
      0.000260191387F,  0.000133648617F,  -0.0011504821F,   0.000160504831F,
      -0.00230136095F,  0.000340919243F,  0.01486474F,      0.00875595491F,
      -0.000733483874F, 0.00347366789F,   0.00249532843F,   0.00979369227F,
      0.0118656624F,    6.23203814E-5F,   0.00125633203F,   -0.00270320382F,
      -1.39859876E-5F,  0.00249506417F,   -0.000368554145F, 2.53509334E-5F,
      -0.00258716987F,  -0.000624460576F, 0.00152955006F,   3.70989073E-5F,
      0.00114001764F,   -0.000720094191F, -0.000843603397F, -0.000453024171F,
      -0.000321123196F, -0.000709735614F, 7.37099617E-5F,   0.00668225531F,
      1.31032195E-7F,   -1.47728383E-6F,  -0.000141604658F, -0.00109070854F,
      0.000120466706F,  0.00140152627F,   0.0108877169F,    -0.000963613275F,
      -0.00533930119F,  -0.00143392663F,  0.000418288895F,  -0.000776159519F,
      -0.000339441438F, -3.18142526E-7F,  -7.64513516E-7F,  -4.95546715E-9F,
      -0.00187881896F,  -0.000128142143F, 1.39529192E-7F,   -0.000601065636F,
      1.89759521E-5F,   0.00141948438F,   -5.97627586E-5F,  2.38558812E-7F,
      0.0172189251F,    -1.69510031E-7F,  -7.09732944E-7F,  3.85238792E-7F,
      0.00239633257F,   4.8831138E-5F,    -2.04676127E-7F,  -8.42860209E-6F,
      -6.74265248E-5F,  -0.000330363953F, 1.12161806E-5F,   0.00158293906F,
      0.00118106545F,   0.000395420444F,  0.000149852465F,  -0.000117827905F,
      -0.000451952947F, -1.94844033E-5F,  1.3625332E-5F,    -0.000465184741F,
      0.00472352887F,   -0.000250676909F, -0.0103198057F,   -0.000504570722F,
      -0.000859845604F, 0.000237973072F,  -0.00227992097F,  -0.00708614F,
      -1.29671844E-6F,  -0.00154623564F,  -3.23575193E-7F,  -0.0147726163F,
      0.000334113836F,  9.91036941E-5F,   -0.000837447529F, -0.00266561774F,
      -0.000462967466F, -0.0157461166F,   0.000996652409F,  -0.00155728776F,
      -0.00861608F,     6.55998883E-5F,   -0.00628548255F,  -0.000260568748F,
      -0.00745093357F,  -0.000934191106F, -0.0181049462F,   0.00325698033F,
      -0.00658648834F,  -0.00119085377F,  -0.000581591914F, -0.00251859706F,
      -0.00806534F,     -0.00619440293F,  -7.49534156E-5F,  0.00113818992F,
      -0.00563348318F,  -0.0157052F,      0.00511393929F,   -0.00144257722F,
      -4.38337192E-5F,  -2.5205E-5F,      0.00223064958F,   -0.000349623093F,
      -0.0012159053F,   -0.00165131467F,  -0.000725020596F, -0.000782784889F,
      0.00160143734F,   0.0522256792F,    -0.0225508772F,   0.00163438683F,
      -0.00210259343F,  -0.0321532115F,   -0.000169835053F, -0.011985993F,
      -4.67040481E-5F,  -0.00298216543F,  -3.38267528E-5F,  -0.00203739316F,
      0.000105981788F,  -0.000322094245F, -0.00186713622F,  -3.12570119E-5F,
      -0.0201219916F,   -0.00491749635F,  -0.00108589616F,  0.00141438982F,
      -0.00146036537F,  0.00178330357F,   -0.000177395166F, 0.0175635908F,
      -0.00110250921F,  -0.00200994103F,  -0.00708772149F,  -4.17101401E-5F,
      0.000707290834F,  0.00166387495F,   0.000184838049F,  3.54530471E-6F,
      0.000252770522F,  0.0027557807F,    0.0163466223F,    -1.96415431E-6F,
      0.00114006863F,   -0.00570592098F,  -0.0101526417F,   -0.00102705F,
      -0.00515065482F,  -0.000583505491F, 0.00149405119F,   -4.83496615E-6F,
      0.00113466661F,   -0.00297136325F,  -0.00914050639F,  -5.1382608E-7F,
      -0.000802795286F, -0.00933057349F,  -0.0033023071F,   -0.011004067F,
      -0.000951872906F, -0.00352470274F,  -0.000801753253F, -0.00235633552F,
      -0.00158252148F,  3.31287301E-5F,   0.000314018835F,  0.00303674466F,
      -0.00125113304F,  -0.00191067357F,  -0.0218420979F,   -0.00153042504F,
      -0.00685124472F,  -2.46036543E-5F,  -0.00234800484F,  -0.0156736281F,
      -0.00785746612F,  0.0037051714F,    -0.0030598389F,   -0.424786985F,
      -0.0129194977F,   0.00396800879F,   8.10907568E-6F,   1.88803551E-5F,
      -0.00182471704F,  -0.00673355162F,  -0.000918688718F, -0.000103112761F,
      -1.76887424E-5F,  2.98584018E-6F,   -0.00902377442F,  0.000230684906F,
      0.00367539842F,   -0.00125393446F,  -0.00490391813F,  -0.0196456835F,
      0.00136417034F,   1.94845575E-6F,   0.0113431802F,    2.75117654E-5F,
      -0.000745880534F, -0.00336119556F,  -0.0118103372F,   0.00386944483F,
      -0.00907657668F,  -0.00234367F,     -0.000399059878F, 0.00258899829F,
      -0.0066318484F,   -0.0210031401F,   -0.0224024858F,   0.00998132583F,
      -0.00514280051F,  -0.000115921968F, -0.0063096839F,   -3.79578705E-5F,
      -0.0052518039F,   0.00134264608F,   0.000626426539F,  0.000559518579F,
      -9.05252818E-6F,  -0.0118240081F,   -3.19181927E-5F,  0.0011950942F,
      -0.00287006074F,  -8.46952958E-7F,  -4.26690603E-5F,  -5.73537582E-5F,
      0.00320769614F,   -0.000276077539F, 0.00351903471F,   -0.000785001146F,
      0.000252684869F,  0.00101934094F,   -0.00253606564F,  -0.0191967413F,
      0.00102088484F,   -0.010198297F,    -0.000736612F,    0.00181817F,
      0.000949966197F,  -0.0058380831F,   -0.00111708359F,  -0.0011710265F,
      0.000694120419F,  0.000108589695F,  -0.00317300903F,  -0.0121384887F,
      -0.00459045079F,  -0.0094652269F,   0.000308411429F,  -0.00694590248F,
      -0.00205804943F,  -0.0019556277F,   0.00916089583F,   -0.0133883841F,
      0.000256727188F,  -0.00288571743F,  0.000869554409F,  0.000634851633F,
      0.00159337022F,   0.0230004154F,    -7.15737224E-6F,  0.000648670073F,
      -0.0207567625F,   8.53868278E-6F,   -0.00657061441F,  0.000338949176F,
      -0.000952504459F, -0.00150207349F,  7.96681616E-6F,   7.58805072E-6F,
      -0.229451254F,    0.00413796818F,   -0.00646067457F,  0.00373793906F,
      0.0842637196F,    -0.0196291693F,   0.000431654858F,  0.000870984339F,
      -0.00415979186F,  -0.0129028875F,   4.38744E-6F,      -0.00503046485F,
      -0.00696367165F,  -0.293174297F,    -0.0050171474F,   0.00116933067F,
      -0.00951735117F,  -0.000712142442F, -0.0060752444F,   -0.0118836034F,
      0.000940119906F,  0.000588068389F,  -0.00746840285F,  -0.0150755178F,
      -0.00453179143F,  -0.00569637259F,  0.00120737823F,   -0.00167322659F,
      -0.00123233732F,  -0.00932754111F,  -0.00138994353F,  -0.0149029195F,
      0.0155318426F,    -2.91528013E-5F,  -0.00279398775F,  -0.00797204766F,
      0.000707997708F,  0.00201954949F,   0.00258813193F,   -0.00298131537F,
      -0.0011777319F,   -0.0022795382F,   -0.0024240911F,   8.11865448E-7F,
      -0.00559594575F,  -0.00863348413F,  -0.0191905927F,   -2.38651478E-6F,
      0.00160833809F,   0.00184033159F,   -0.0185900461F,   0.000227240525F,
      0.00151157228F,   -0.000521856477F, -0.00843021553F,  -0.000384458574F,
      4.2764741E-6F,    -0.00114347728F,  0.000683964055F,  -0.00181512244F,
      0.00155142089F,   -0.00546799228F,  -0.00408021966F,  -0.00406586565F,
      -4.34932081E-5F,  -0.00175575342F,  0.00287986593F,   0.00024320402F,
      3.17872968E-6F,   -0.011115077F,    -0.00113475765F,  -0.00303917076F,
      2.63028778E-6F,   0.000618360413F,  -0.00731243705F,  0.000986313797F,
      -0.00198191986F,  0.00143922854F,   0.000670098292F,  -0.0285085402F,
      -0.00842475F,     1.14616341E-5F,   -0.00126387167F,  0.00136230572F,
      0.00195986498F,   -0.000353102543F, -0.00686073396F,  -0.00575021887F,
      -3.385729E-5F,    0.000194722874F,  -0.000180467905F, 0.00268399715F,
      0.00170211075F,   -0.037167754F,    -0.00500745466F,  -0.00286192726F,
      0.000415885472F,  0.000633316464F,  -0.00223438162F,  -1.00904417E-5F,
      -0.000467287202F, -0.0140134403F,   0.000998592586F,  2.13966414E-5F,
      0.00122002803F,   -0.00850346312F,  0.00360819721F,   -0.000201326358F,
      3.25992733E-5F,   0.000240735535F,  -0.000914311735F, 7.78196409E-6F,
      -0.0031645475F,   0.00120107387F,   0.00118140143F,   0.000398644072F,
      0.00103147456F,   -0.00359596894F,  0.00150569598F,   -0.000694301969F,
      0.000439276017F,  0.00293165282F,   0.00402415777F,   0.00101999321F,
      0.00346351764F,   0.000903275853F,  0.000236360458F,  -0.00943185F,
      -0.00144045951F,  0.00185347721F,   -0.0038095566F,   -0.000463318982F,
      -0.000991646F,    -0.00766952103F,  0.00283192983F,   -0.00064843538F,
      0.00718120532F,   -0.000310957868F, -0.00106281403F,  -0.0108272824F,
      -0.00244478323F,  -0.00157388416F,  -0.0036254162F,   -0.465218693F,
      2.35750178E-7F,   2.08851543E-5F,   -0.00256513059F,  -0.0086866105F,
      -0.000845739618F, 0.000689039065F,  0.00160949468F,   -0.0093180323F,
      -0.00286404393F,  -0.00773181953F,  0.00236424035F,   -0.0143797761F,
      -0.00250010891F,  -2.03289164E-5F,  4.24339578E-6F,   -6.55815384E-5F,
      -0.0232975781F,   -0.0131051224F,   1.45642916E-5F,   -0.00386156607F,
      5.4420052E-5F,    0.00381375942F,   0.000430612417F,  1.56584429E-5F,
      0.000927171495F,  1.21403809E-5F,   2.62203539E-5F,   6.14203782E-6F,
      0.000739936659F,  0.000271304394F,  2.55098666E-6F,   -0.000905648F,
      -0.0010548993F,   -0.00363388937F,  -0.00286136661F,  0.00506290747F,
      0.0356920399F,    0.000297347287F,  0.000400027755F,  -0.000698350312F,
      -0.0016475548F,   -0.00194624986F,  -7.85311568E-6F,  -0.00229220325F,
      0.000890460389F,  -0.00840341207F,  0.000661588332F,  8.74464458E-5F,
      0.0156358816F,    -0.120300665F,    0.0445312038F,    0.0202315431F,
      -0.00621251715F,  0.0089610517F,    -0.00558252633F,  -0.015076098F,
      0.192519352F,     -0.120919868F,    0.0206557773F,    -0.0295346882F,
      -0.0245486293F,   0.014012374F,     0.189219326F,     -0.146334559F,
      -0.190424815F,    -0.0104626501F,   0.0948371738F,    -0.0505331308F,
      0.104474366F,     -0.0869478881F,   0.07674326F,      0.019250758F,
      -0.0272440333F,   -0.0180632584F,   0.0847142488F,    -0.0187896695F,
      0.0407559723F,    -0.0370429941F,   -0.0097754F,      -0.158017382F,
      -0.154878989F,    -0.0621243194F,   0.021553481F,     0.0676778182F,
      -0.00974398945F,  -0.00862585381F,  -0.0910054892F,   -0.107235081F,
      -0.0314954966F,   -0.0437611304F,   -0.0198954903F,   0.0781552568F,
      0.397571683F,     0.276344955F,     0.0857517496F,    -0.0750907212F,
      0.0675651953F,    0.51270324F,      -0.0438874774F,   0.0459572189F,
      -0.0082496535F,   0.0531830899F,    -0.0305902325F,   -0.108082689F,
      -0.0284256879F,   -0.0859369114F,   -0.0453366935F,   -0.0119369086F,
      -0.146487802F,    0.0337372795F,    0.0842731372F,    0.0131746605F,
      -0.0803363F,      0.319330305F,     -0.17268087F,     0.107568949F,
      0.0883835703F,    0.0309852548F,    -0.125756592F,    -0.0124478163F,
      0.0175470226F,    0.310069025F,     0.225708127F,     -0.00768979872F,
      0.23495391F,      0.0159210265F,    -0.00964548904F,  -0.0079640178F,
      0.267619163F,     0.0461737774F,    0.121547379F,     -0.0518264547F,
      -0.0463084355F,   0.0841503143F,    -0.0235264767F,   -0.00996197946F,
      0.403045177F,     -0.128494039F,    -0.0172362253F,   -0.00975637604F,
      0.0303903986F,    -0.00616691634F,  -0.0932951F,      -0.017757928F,
      0.0107298205F,    0.0779110491F,    -0.0520207658F,   -0.150536522F,
      0.0760147423F,    -0.236159712F,    -0.0418248624F,   0.299678534F,
      -0.0754491463F,   -0.0660272092F,   -0.0314602926F,   0.112541236F,
      -0.0371991768F,   -0.017344296F,    0.0727705061F,    0.0131254038F,
      -0.0739323273F,   0.454170495F,     -0.0316375755F,   0.604306F,
      -0.0143699637F,   0.248587191F,     -0.00648447871F,  -0.00702323485F,
      -0.073622413F,    -0.00592147047F,  -0.0510663688F,   -0.0499898046F,
      -0.00992729608F,  -0.0116385715F,   -0.0956774727F,   -0.00358672952F,
      -0.0235641599F,   0.0335619822F,    0.128405809F,     0.0690655261F,
      0.343144774F,     -0.0106132934F,   -0.278337806F,    -0.0927541F,
      0.0115901036F,    -0.0209594015F,   -0.187636539F,    0.153578445F,
      0.133199066F,     -0.0819013938F,   -0.148532957F,    0.205214322F,
      -0.00502834562F,  -0.20333524F,     -0.254161179F,    -0.0517925769F,
      -0.0829561129F,   -0.0535426475F,   -0.196347773F,    -0.00790658407F,
      -0.0275397636F,   0.0170407295F,    -0.0500803F,      0.328775316F,
      -0.0132309347F,   0.0270551797F,    -0.0178062674F,   0.442423731F,
      -0.0179445427F,   -0.0120567521F,   -0.00854594167F,  -0.0101565281F,
      -0.206387043F,    0.0114818905F,    0.404645592F,     0.100600086F,
      -0.102011353F,    -0.05807532F,     -0.148296088F,    -0.244718611F,
      0.0156916138F,    -0.0976199433F,   0.0693328083F,    -0.0634862855F,
      0.366848201F,     -0.039595928F,    -0.0328488164F,   -0.0451899F,
      0.0174252111F,    -0.106917828F,    -0.0321680419F,   0.0543322489F,
      -0.123357564F,    -0.103142768F,    0.0318598412F,    -0.0211290419F,
      -0.100836463F,    -0.0195682831F,   -0.260177314F,    -0.0876769125F,
      0.0546653084F,    -0.0809008852F,   -0.287584484F,    -0.0411743857F,
      0.143359393F,     0.544623673F,     -0.0131718591F,   0.132317558F,
      -0.223546639F,    -0.010054742F,    -0.0549867824F,   -0.0521799326F,
      0.0203507524F,    -0.0881914049F,   -0.00808322337F,  -0.00862284563F,
      0.627836585F,     0.0156674795F,    -0.018634934F,    -0.118522644F,
      0.302279145F,     0.0154499123F,    0.411677897F,     -0.0283736773F,
      0.0614187419F,    -0.194254428F,    -0.0109136393F,   -0.0204115026F,
      -0.061094068F,    0.343773842F,     -0.0875794888F,   0.140063345F,
      0.0236265901F,    0.342781276F,     0.0238679219F,    0.0590350442F,
      0.397354305F,     0.0117365904F,    0.0583493598F,    0.0266328F,
      -0.14234294F,     -0.0578804836F,   -0.0770623F,      -0.097386308F,
      0.0139115816F,    -0.0960146859F,   -0.0344771706F,   -0.0285359323F,
      -0.0637716502F,   -0.0099494569F,   -0.189204812F,    -0.0349811316F,
      -0.0089023672F,   0.012863378F,     0.306300581F,     0.0773095265F,
      -0.235405937F,    -0.0703856498F,   0.0883149132F,    -0.0112735005F,
      -0.0762585849F,   -0.0208872091F,   -0.0108019607F,   -0.00967807323F,
      0.401371747F,     0.0132294688F,    -0.0802314878F,   0.105948962F,
      0.417815596F,     -0.180849671F,    0.0236845165F,    0.000344851811F,
      -0.0078228889F,   0.0915556699F,    -0.041644197F,    0.0829527825F,
      0.41996479F,      -0.118456461F,    0.0552982204F,    -0.0585849583F,
      -0.00636454858F,  0.0740533918F,    -0.0233971123F,   -0.0790255219F,
      -0.00978426542F,  0.00200059451F,   0.0844805688F,    -0.0901649222F,
      -0.00548229367F,  0.019044742F,     -0.0827581F,      0.367288619F,
      -0.102074303F,    -0.228305891F,    0.0516551509F,    0.214005738F,
      0.0153655028F,    -0.021174442F,    -0.0601716824F,   0.427391291F,
      0.0300839208F,    0.225331366F,     -0.166227639F,    0.0910387486F,
      -0.0388795175F,   -0.0381186604F,   0.0361924693F,    -0.250168592F,
      0.420496643F,     0.136730164F,     -0.28646037F,     -0.0351749659F,
      0.0270806756F,    -0.0614520311F,   -0.152119741F,    -0.010864988F,
      -0.0312970243F,   0.0871922523F,    0.359226376F,     -0.00604353752F,
      0.0120121343F,    -0.0186188146F,   0.376970947F,     -0.0186358988F,
      -0.00862731319F,  -0.0409256928F,   0.0151986377F,    -0.00637789071F,
      0.0621181317F,    -0.052649118F,    0.032121826F,     -0.0479717441F,
      0.138608918F,     -0.0391943939F,   0.448542476F,     0.283807337F,
      0.0803125F,       0.154017314F,     0.341078401F,     0.416369051F,
      -0.0101726539F,   0.00862662774F,   -0.0788677856F,   0.0719797462F,
      0.0125529123F,    -0.0736009F,      -0.00796119589F,  -0.0698289424F,
      -0.018307725F,    0.0997652039F,    0.446916789F,     -0.0532275178F,
      -0.269560456F,    0.0720769763F,    0.0303690042F,    0.0705022365F,
      0.0507141687F,    0.0623067059F,    -0.0500602126F,   -0.0246492103F,
      -0.0100932047F,   -0.00992266F,     0.113085546F,     -0.0255877227F,
      -0.121299945F,    -0.0433101F,      0.23830469F,      -0.0770482123F,
      -0.0246662F,      -0.0327989645F,   -0.0971007645F,   0.00622480689F,
      0.0280481316F,    -0.00545777334F,  -0.00921673235F,  -0.0122735025F,
      0.031742F,        -0.340941668F,    -0.0119912075F,   -0.0212843977F,
      -0.0304009486F,   0.0307548121F,    0.228673577F,     -0.00774139259F,
      0.370676309F,     -0.00920305122F,  -0.00864098314F,  -0.0198391173F,
      0.326521039F,     -0.0321539044F,   -0.00881213136F,  0.00118429016F,
      -0.143021867F,    -0.103718162F,    -0.0300797913F,   0.0725559741F,
      0.233848765F,     0.011896437F,     0.0155232484F,    -0.00411319267F,
      -0.115194462F,    -0.162447959F,    -0.0202823449F,   -0.0916206464F,
      0.383730739F,     0.0739927F,       0.166611493F,     0.0388298444F,
      0.00635115802F,   -0.000213235471F, -0.0555501767F,   -0.0211454928F,
      -0.000145654019F, -0.00300268061F,  -5.41522641E-5F,  -0.0435728319F,
      -0.080880329F,    -0.0265665222F,   0.0110548884F,    -0.0969672278F,
      0.00232045096F,   -0.0083966665F,   -0.0723532438F,   0.00542015443F,
      -0.00524396077F,  -7.55404908E-5F,  -0.0316300653F,   0.0386507846F,
      -0.00516376458F,  -0.0221243445F,   0.0805278197F,    -0.0362302437F,
      -0.0180104F,      -0.00239168503F,  -0.0706994906F,   -0.0522094294F,
      -0.0236585755F,   0.0182228349F,    -0.000375368545F, 0.0312403869F,
      0.00599418301F,   0.0285570528F,    -0.0907101929F,   -0.0345298946F,
      -0.000105065192F, -2.26368629E-5F,  0.00717271538F,   -0.0078999633F,
      -0.142090693F,    -0.0778052211F,   -0.0151367588F,   -0.0270840768F,
      -0.0861515403F,   -0.198288023F,    -0.00663411F,     0.0875595063F,
      -0.0754767731F,   -0.0897610635F,   0.00110274344F,   -0.0225990191F,
      -2.18402092E-5F,  -0.032043F,       -0.00103207096F,  0.00815953128F,
      -0.000212386469F, 0.00606032414F,   0.0250548907F,    -4.7209327E-5F,
      -0.00499202032F,  -0.0296986941F,   -0.0205378812F,   0.0145499613F,
      0.00563175697F,   -0.0466866046F,   0.00655869581F,   -0.0144442078F,
      -0.0536838658F,   -0.0362364873F,   -0.0212393645F,   -0.000262105954F,
      -0.0548695847F,   -0.0280086324F,   -0.0830514804F,   -4.06113286E-5F,
      -0.00565779954F,  -0.0183236804F,   -0.0321476981F,   3.85862768E-6F,
      0.0117029715F,    -0.0567748509F,   -0.071117945F,    0.0125027718F,
      0.00449814461F,   -0.00344038662F,  -0.0533335023F,   -2.87757011E-5F,
      -0.0046974523F,   -0.0061924113F,   -0.0116022732F,   -5.74115911E-5F,
      -0.000259032182F, -0.0034438842F,   0.000703728176F,  -0.0148754921F,
      -0.00631415099F,  -0.0303297713F,   -0.0552081876F,   0.0361452438F,
      -0.0285580792F,   0.0119984373F,    -0.00213781325F,  0.0636041611F,
      0.00210111309F,   -0.00354670966F,  -0.0257677846F,   -0.0288765766F,
      -0.0785514116F,   3.12275915E-6F,   0.0127176233F,    -0.0370882042F,
      0.0116858929F,    -0.152552992F,    -0.0432153456F,   0.0399931781F,
      -0.0304008983F,   -0.0836773962F,   -2.18002715E-5F,  -0.000497439061F,
      0.00256904075F,   -0.0108590964F,   -0.0668707937F,   0.00228711963F,
      -5.09769561E-5F,  -2.15300906E-6F,  0.0526983067F,    -0.00223158882F,
      -0.0335922465F,   -0.0486717783F,   -0.00989935827F,  -0.011765752F,
      -0.0607998818F,   -0.000119347642F, 0.0620973632F,    0.0210839827F,
      0.0112383254F,    -0.0323820114F,   0.0152057307F,    -0.0265777055F,
      -0.0275764354F,   0.00401101727F,   0.00716884574F,   0.0827405304F,
      -0.0640587881F,   -0.0543829277F,   -0.107486859F,    0.0195160769F,
      0.0181852318F,    0.0028181F,       -0.104980148F,    -6.79438E-6F,
      0.0702239722F,    0.00581584917F,   0.00418959279F,   -0.0730842203F,
      -7.55977671E-5F,  -0.00457642414F,  -0.000278160616F, -0.0718260705F,
      -0.0797290057F,   -9.37727164E-5F,  3.45320659E-5F,   -5.07327E-6F,
      0.0336641334F,    0.00902358722F,   -0.0851387829F,   -0.0237296149F,
      0.00198583584F,   -0.0571818277F,   0.0277627613F,    -0.0642863289F,
      0.0130572189F,    -0.000415618182F, -0.0300351102F,   0.0579015948F,
      -0.0783356577F,   0.0170002747F,    -0.00364978402F,  -0.000784177566F,
      -0.0253635235F,   0.00287192012F,   0.0200577378F,    -0.0161685627F,
      0.0122554135F,    -0.0710278451F,   -0.0322019719F,   -0.0484185182F,
      0.0266051628F,    -0.10575074F,     0.0759705678F,    0.0205606706F,
      -0.0456961878F,   0.00429738732F,   0.0487415679F,    -0.000673093833F,
      0.00855694246F,   -0.0492443219F,   -0.000121205558F, -0.0137928119F,
      -0.215534583F,    -2.82968012E-5F,  -0.0474861339F,   0.0110798115F,
      -0.00432496844F,  0.0370462462F,    -4.90293023E-5F,  -6.14608362E-5F,
      0.0467652269F,    -0.0591148F,      -0.00949929934F,  0.00190093543F,
      -0.00942254812F,  -0.0324971974F,   -0.0794334039F,   -0.0220516976F,
      -0.0270983092F,   3.36905141E-5F,   -0.000367653876F, -0.0713456273F,
      -0.0188907553F,   -0.0266548414F,   0.00389642F,      0.0069635231F,
      -0.066021353F,    -0.073382996F,    -0.138836384F,    0.00167853769F,
      -0.070191741F,    9.2645023E-6F,    -0.0368397385F,   -0.00570640713F,
      -0.00812998228F,  -0.00680219708F,  -0.148550913F,    0.00777458446F,
      0.00994184799F,   -0.022342233F,    -0.0736120492F,   -0.0279220417F,
      -0.15270634F,     -9.04201734E-5F,  0.0248623937F,    -0.0473640673F,
      -0.0138379652F,   0.012622497F,     0.0465248488F,    -0.025312908F,
      0.0299531966F,    0.00884186F,      -0.0665597618F,   -1.51127697E-5F,
      -0.0360260718F,   -0.0256866924F,   -0.00302314712F,  -9.06221467E-5F,
      -0.0839763358F,   0.0146111446F,    0.00772043178F,   -0.0225987341F,
      -0.0604838356F,   0.0081674559F,    0.0739108473F,    -0.00422944129F,
      5.12173528E-7F,   -0.0397033319F,   0.00502928114F,   -0.0448821411F,
      -0.0405508801F,   0.0115936967F,    -0.0225237347F,   0.00644669402F,
      0.000125405888F,  -0.0773669928F,   0.0206681304F,    -0.156152129F,
      -3.32259078E-5F,  -0.0366942175F,   -0.0196324978F,   -0.035888616F,
      -7.2415809E-5F,   0.018755326F,     -0.0422393568F,   -0.0796191916F,
      0.0502114445F,    -0.0303351115F,   -0.107447408F,    0.0750292242F,
      -0.0200724229F,   0.000795939646F,  5.56775885E-5F,   -0.0516314916F,
      -0.0252503511F,   -0.0188633148F,   -0.026688315F,    -0.0188935883F,
      -0.00216096407F,  -0.00197518477F,  0.0143949352F,    -0.0574767664F,
      -0.0866284743F,   0.00352312182F,   -0.0436023548F,   0.0172171984F,
      0.024970429F,     -0.0801105797F,   -0.119313069F,    3.01062573E-6F,
      -0.000401673F,    -0.0251032077F,   -0.0413225107F,   -0.000175537134F,
      0.00928535406F,   -0.00703683F,     -0.00299600442F,  -0.00218111044F,
      3.6592437E-6F,    -0.00521646626F,  -0.0161923058F,   -0.000121469107F,
      0.010302241F,     0.0376203F,       0.0335261486F,    -0.0453394204F,
      -0.0458221436F,   -0.0113949422F,   -0.0271304715F,   -0.0832287371F,
      -0.0292690564F,   -0.0115214046F,   0.0904996172F,    -0.0671069175F,
      -0.128664747F,    -0.00809220411F,  0.118248969F,     -0.0148674082F,
      0.00836195797F,   0.090805009F,     -0.00381944608F,  0.00424740789F,
      -0.0654850677F,   -0.00204463373F,  -0.125862598F,    0.0033910519F,
      0.0493750311F,    0.00359896407F,   -0.0467774309F,   -0.0120652821F,
      -0.0247006062F,   -0.0227315649F,   0.003363939F,     0.0454593897F,
      -1.58740913E-5F,  -0.000102603874F, -0.0042887223F,   -0.0573766567F,
      0.0390929058F,    -0.00562139461F,  -0.063695468F,    -0.0257034115F,
      -0.0129852816F,   -0.0556433909F,   0.0114663104F,    -0.0105945058F,
      0.00321393088F,   -6.44269385E-5F,  -7.69509061E-5F,  -0.000127168518F,
      -0.0457998924F,   -0.019606337F,    -7.1672308E-5F,   -0.0303308F,
      -0.00022907056F,  -0.0466595702F,   -0.0696990043F,   3.93359915E-5F,
      -0.0355135389F,   9.39369602E-6F,   -9.14997436E-5F,  0.000148543666F,
      -0.0695278868F,   -0.00234198F,     -2.64897062E-6F,  -0.00692611467F,
      -0.000193786153F, -0.00773071824F,  -0.0027026094F,   0.0852008909F,
      -0.160452947F,    0.0120855579F,    0.0118633742F,    -0.0111197513F,
      0.00803737342F,   0.0345999412F,    9.82778438E-5F,   0.0286855847F,
      -0.0738314241F,   -0.000319826911F, -0.0906244889F,   0.0152353859F,
      0.0122100497F,    -0.0685587153F,   0.0532146804F,    0.0227196049F,
      -0.00631499756F,  0.0155053977F,    -0.00533545716F,  -0.0644893944F,
      0.212158099F,     -0.0334987827F,   0.0352170356F,    0.0477775969F,
      -0.0589357F,      -0.026223246F,    0.195533857F,     -0.165626824F,
      -0.428328454F,    -0.0103451079F,   0.038197808F,     0.0845859423F,
      -0.0581872761F,   0.0486425F,       -0.0251490101F,   0.0263362955F,
      -0.00419124402F,  -0.0461379625F,   0.0886044F,       0.0253925398F,
      0.0588818379F,    0.0342758037F,    -0.00965406839F,  -0.20378311F,
      -0.148321524F,    -0.126850337F,    0.0408417806F,    0.0625894591F,
      -0.00966451876F,  -0.00837832876F,  -0.0904068202F,   -0.0608444475F,
      0.231439859F,     0.0486833043F,    -0.0467549115F,   0.0928693339F,
      0.388925493F,     -0.07535436F,     -0.0195855163F,   0.501822829F,
      -0.0903884247F,   -0.0352251567F,   -0.0141168255F,   -0.199645519F,
      -0.00832673069F,  -0.0419501103F,   -0.0369554684F,   0.000323199754F,
      -0.0440039672F,   -0.0328562073F,   0.0265806764F,    -0.011721937F,
      -0.396837205F,    -0.0427218154F,   0.0892463923F,    0.0172633734F,
      -0.100997679F,    0.366015345F,     -0.0573327653F,   0.0782054812F,
      0.0989174247F,    -0.0218780451F,   -0.251846284F,    -0.0132232169F,
      0.0192113835F,    0.369668812F,     0.20848377F,      -0.00760616036F,
      0.249507785F,     -0.113934778F,    -0.00771580823F,  -0.00798258092F,
      0.274757087F,     -0.0464448333F,   -0.0428891033F,   0.0174851399F,
      -0.0612759106F,   0.0785965398F,    0.0176926218F,    -0.0103143975F,
      0.423895866F,     -0.136655226F,    0.00433056755F,   -0.00987614784F,
      0.00445838785F,   -0.425296634F,    -0.106992744F,    -0.202676311F,
      -0.151352733F,    0.11685089F,      0.0151905753F,    -0.0382440761F,
      0.0786911696F,    -0.0590581819F,   -0.0295229945F,   0.278740019F,
      -0.0725664422F,   -0.0535531677F,   -0.0552248619F,   0.103519768F,
      0.024521647F,     -0.0178675298F,   0.0471550338F,    -0.0218482632F,
      -0.0417471714F,   0.494657636F,     0.0229082238F,    -0.0113709643F,
      -0.0468977317F,   0.217227638F,     -0.00644671777F,  -0.00701419264F,
      -0.0825049356F,   -0.202501759F,    0.031184053F,     -0.0168172624F,
      -0.00989209861F,  -0.0116518307F,   0.0670624599F,    -0.0552705489F,
      0.0408383347F,    -0.0213750172F,   -0.115840919F,    -0.0233335178F,
      0.333226562F,     -0.0102646593F,   -0.0820362642F,   0.00795999542F,
      0.0149957417F,    0.0904043242F,    -0.274379551F,    0.133289203F,
      0.0781881437F,    -0.100619607F,    -0.0510360561F,   0.261164874F,
      -0.0358819775F,   -0.248341084F,    -0.275048852F,    -0.218417138F,
      0.0237846449F,    -0.0664244443F,   -0.300778747F,    -0.00855548307F,
      0.0796352F,       0.023017304F,     -0.0551304854F,   0.365181446F,
      -0.0134512223F,   -0.0490501113F,   -0.0164642036F,   0.456593275F,
      0.0910415F,       -0.0133188386F,   -0.00850841403F,  -0.0100368317F,
      -0.142800376F,    0.0129822716F,    0.44139567F,      -0.0363783613F,
      0.125792235F,     0.11412587F,      -0.0221545715F,   -0.444271415F,
      0.018955335F,     -0.0784400851F,   0.0756348446F,    0.183961272F,
      0.37708962F,      0.0836915746F,    -0.0656416044F,   -0.111177929F,
      -0.0984497666F,   -0.0372203775F,   -0.00469301408F,  -0.0437098294F,
      -0.00156622531F,  -0.189774618F,    0.0371136814F,    0.0380729772F,
      0.0278666243F,    0.0723317713F,    -0.0762064382F,   -0.0252632536F,
      -0.0819161758F,   -0.112240359F,    -0.0743421763F,   -0.0218971558F,
      0.139626428F,     -0.000638152065F, -0.0128601184F,   0.216307253F,
      -0.223684266F,    -0.0100939749F,   -0.0379333347F,   0.0276659094F,
      -0.153472945F,    0.068330504F,     -0.00808516052F,  -0.00859430339F,
      -0.00568165584F,  0.0163984057F,    0.0231793281F,    -0.0703228265F,
      -0.013821179F,    -0.0289726965F,   0.419392765F,     -0.0252177678F,
      0.0813333839F,    -0.440927148F,    -0.0108009344F,   0.0155383786F,
      -0.0967788175F,   -0.0071179769F,   -0.122692935F,    0.133552521F,
      -0.0282410923F,   0.402847767F,     -0.0210398789F,   -0.0288439859F,
      0.411232799F,     0.0116970912F,    0.0550419874F,    -0.0220345948F,
      -0.263746172F,    -0.0482895933F,   0.201818258F,     -0.120142974F,
      0.0156068029F,    -0.385730654F,    0.0270699635F,    -0.0206026081F,
      -0.02621953F,     -0.00982292835F,  -0.061464306F,    -0.0218103118F,
      -0.0401697233F,   0.0164465159F,    0.285549372F,     0.0828840286F,
      -0.0590068065F,   -0.117411815F,    0.0994634181F,    -0.0116516436F,
      -0.0755177662F,   -0.0455015339F,   -0.0484327488F,   -0.00983721483F,
      0.370258182F,     0.0129215F,       -0.0231515076F,   0.0566748939F,
      0.45010969F,      -0.160208359F,    -0.0187660176F,   -0.0720971674F,
      -0.00766664371F,  0.102645919F,     -0.035322845F,    0.0913397521F,
      0.427625239F,     -0.0342435651F,   0.0391591676F,    -0.0311865266F,
      -0.00625828188F,  0.0809864402F,    0.038864065F,     0.249547616F,
      -0.0101988399F,   -0.0563614815F,   0.082578F,        -0.123294152F,
      -0.00546018686F,  0.019858351F,     -0.0850289688F,   0.419696063F,
      0.0347664505F,    -0.0342086479F,   -0.0666712672F,   0.0200210158F,
      0.011899217F,     -0.0246609785F,   -0.0564363226F,   0.441695333F,
      0.0353772491F,    0.270569861F,     -0.323727399F,    0.0636938214F,
      -0.0391183309F,   -0.0406536609F,   0.0357506387F,    0.0153196659F,
      0.452888846F,     0.0483137593F,    -0.317688137F,    -0.15978162F,
      0.0293045044F,    0.112300038F,     0.0531493798F,    -0.0110477805F,
      -0.0737797F,      -0.00126062077F,  0.385192811F,     -0.00600622687F,
      0.014536228F,     -0.0917219222F,   0.393331587F,     -0.0476891473F,
      -0.00854020193F,  -0.0657039732F,   0.0164507087F,    -0.00636454346F,
      0.0305303279F,    0.0467177294F,    0.0853680447F,    0.0277033933F,
      0.173933968F,     -0.0642797872F,   0.461403489F,     0.281231463F,
      0.0940809473F,    0.214337498F,     0.288533509F,     0.441987574F,
      0.535260499F,     -0.171780556F,    0.446156412F,     -0.0165174827F,
      0.0150784655F,    0.557187617F,     -0.0986835733F,   -0.0855563805F,
      0.0664859638F,    -0.0450743251F,   0.440525442F,     -0.0839298F,
      -0.0674522892F,   0.0719793886F,    0.0357224643F,    -0.0377775058F,
      0.056445282F,     0.0309617259F,    -0.103132062F,    -0.00390446652F,
      -0.0103183752F,   -0.00972218253F,  -0.12386255F,     -0.0380674526F,
      0.0371365212F,    0.0699562356F,    0.238469586F,     -0.0989149585F,
      0.062368542F,     -0.00999525934F,  -0.128712803F,    -0.0228541084F,
      -0.0559775718F,   -0.00547699F,     -0.00914524589F,  -0.0120327948F,
      -0.0298265666F,   -0.428830534F,    -0.0122575127F,   -0.056188941F,
      -0.0329085886F,   0.0322112814F,    0.227899268F,     -0.00773536414F,
      0.381818444F,     -0.00921407901F,  -0.00856687874F,  -0.0174099449F,
      0.317714632F,     -0.0450186059F,   -0.00888759736F,  -0.123930529F,
      -0.129365861F,    -0.123661771F,    -0.061000593F,    -0.165653303F,
      -0.0770261511F,   0.0141702015F,    0.0180270132F,    -0.100285344F,
      0.032040298F,     -0.0394924581F,   -0.0224842317F,   0.024350293F,
      0.392343104F,     -0.359627694F,    0.170824885F,     0.0431743F,
      -0.00104879588F,  0.00122505322F,   -0.000398700417F, 0.000126924759F,
      -4.91828632E-6F,  -0.00162497454F,  -1.32124603E-6F,  -0.000311106676F,
      0.00318569597F,   0.000895474048F,  -0.000698033255F, -0.000572909485F,
      -0.00027907535F,  -0.000368794601F, 0.00184699474F,   0.00176433416F,
      0.0069358889F,    4.06231084E-5F,   -0.0009901023F,   0.00403373595F,
      0.000948386616F,  3.12563061E-5F,   0.00110224017F,   0.000240038411F,
      -0.000769922684F, -0.00133787841F,  -0.00304876012F,  0.00271599065F,
      -0.00101769087F,  0.00208732742F,   -5.45834264E-5F,  -0.00175762142F,
      0.00358284405F,   -0.00088126387F,  0.00136606605F,   -0.000413512229F,
      -2.82769597E-5F,  -2.67427131E-5F,  0.00178644294F,   -0.000315875484F,
      0.00499117328F,   -0.00112152F,     -0.00116505427F,  -0.00190259772F,
      -0.00309612F,     -0.00223297579F,  0.00202301424F,   0.0145351468F,
      0.000451144821F,  -0.00100904505F,  -0.000541782472F, 0.00375205372F,
      -4.6045443E-8F,   -0.00316751609F,  -0.000539765344F, -0.000180952033F,
      -0.000512380502F, 0.000646403234F,  -0.00218133698F,  -2.73030928E-5F,
      0.00777847273F,   -0.00138592604F,  0.00139404612F,   -0.00260750344F,
      0.00037840748F,   0.000499652699F,  -0.00133662752F,  0.000268684642F,
      -0.00180097681F,  0.00110610272F,   0.00538502214F,   -1.13555343E-5F,
      0.000611855648F,  -0.000542670779F, -0.000148559673F, -8.40650443E-7F,
      0.002185405F,     -0.000302537897F, 0.00636084517F,   -2.20387278E-6F,
      0.00243463763F,   -0.000316448306F, -0.000229730693F, -0.000719608506F,
      -0.00110177696F,  0.000122410711F,  0.00471732F,      -1.92244479E-5F,
      -0.000430384971F, 0.000521854439F,  0.0254353192F,    -3.49193124E-6F,
      -0.000103668135F, 0.00719236862F,   0.00115215918F,   0.00427249307F,
      -0.000521284237F, -0.00764319301F,  -0.00121944037F,  -0.00429185899F,
      -0.00304517243F,  0.000572327175F,  -0.000475642795F, 0.000255057035F,
      0.000383799721F,  -0.000496108492F, -0.00106719451F,  -0.00291779055F,
      0.00015664252F,   -9.04038097E-6F,  -0.000756725145F, 0.00124762091F,
      -0.00322175771F,  -0.00369951385F,  -0.00310252351F,  -0.00610823929F,
      -0.00450276025F,  0.000405402243F,  6.10611505E-6F,   -6.10171764E-6F,
      0.000442118966F,  0.00372252404F,   -0.00311367586F,  -0.000295845588F,
      -1.43886191E-5F,  2.57110842E-5F,   -0.00768435653F,  0.000131112116F,
      -0.0103045534F,   0.00115376164F,   0.0016702346F,    0.000475131266F,
      0.0010296586F,    2.5592466E-5F,    -0.00585983135F,  -0.00131589151F,
      -0.000131836277F, 0.00929134246F,   -0.000688278058F, -0.00151414704F,
      -0.00496745203F,  0.00207575224F,   -0.000270612829F, 0.00185639062F,
      0.000989779597F,  0.00588577893F,   0.0086405F,       -0.000218421847F,
      -0.00298699131F,  0.000272868259F,  0.0066388147F,    -3.3407312E-5F,
      -0.0012737012F,   -0.00150458165F,  0.000475732231F,  0.00168903044F,
      1.77227357E-5F,   0.000379092467F,  8.1069E-6F,       -0.00202574022F,
      -0.00521624275F,  -7.29081876E-7F,  1.19432198E-5F,   -3.36367048E-5F,
      -0.00410106406F,  -0.00186054816F,  -0.00354637671F,  -0.00152362569F,
      -0.0118474439F,   0.0064198887F,    -0.00210309681F,  0.00721537F,
      -0.00181087479F,  -0.00261060172F,  -0.00145578757F,  0.00940808561F,
      0.00154968083F,   0.00339201F,      0.000214781496F,  2.47573371E-6F,
      0.0001789526F,    0.000588562631F,  0.0004848073F,    0.000618254067F,
      -0.000691086694F, 0.00363202789F,   -0.000832942198F, -0.00141269935F,
      -0.00161646341F,  0.00164134149F,   -0.00200066785F,  -0.00399336964F,
      0.00230363128F,   0.000993569498F,  -0.0101338588F,   0.000319719606F,
      0.00171477848F,   -0.00429724203F,  -2.91865017E-5F,  0.00189796F,
      0.0118919192F,    6.50000857E-7F,   -0.000620687148F, 0.00663156249F,
      -0.000203108502F, -0.00564379F,     1.20214927E-5F,   -9.23706466E-6F,
      -0.00248157163F,  0.000273964572F,  -0.000576939259F, 0.00330283376F,
      -0.00285131345F,  -0.000375441334F, 9.06682762E-5F,   -0.00422670878F,
      -0.00101360085F,  0.00641079573F,   1.90990795E-5F,   0.004177914F,
      0.000308713585F,  -0.00414241804F,  0.00223308662F,   -0.000389215536F,
      -0.00094422017F,  0.000955884112F,  -0.0106104352F,   -0.00142868818F,
      -0.0010101425F,   -0.000455451838F, -0.00370007614F,  -0.000998015283F,
      0.00485840347F,   -0.0018297662F,   0.0114736967F,    -0.000615280063F,
      0.000520518399F,  0.00790923368F,   -0.0019201223F,   0.000834202103F,
      0.00188740727F,   -2.16960252E-5F,  -0.00686572725F,  0.00149340811F,
      -0.000158699797F, -0.00039494611F,  0.000979375676F,  -0.00185820018F,
      -0.000332072377F, 0.000800371286F,  -0.00408341084F,  1.48902291E-5F,
      0.00176885305F,   -0.000595376652F, 0.00563839311F,   4.59891316E-6F,
      -0.000106595908F, 0.00224087015F,   -0.00624096813F,  -3.10930227E-6F,
      -0.00237845141F,  -0.000483203446F, 0.00340924156F,   -0.000562378031F,
      2.60543911E-5F,   -0.00169735285F,  0.000257855456F,  -0.00278747967F,
      0.00202119444F,   -0.00454289513F,  -0.00197101617F,  -0.000850963697F,
      -3.0989544E-5F,   -0.00378441042F,  0.00386330625F,   0.013040971F,
      -2.5194E-6F,      0.00144309876F,   0.00199356489F,   -0.000263961294F,
      -1.21923449E-9F,  -0.00157226459F,  0.00123739673F,   0.00165068405F,
      -0.00359358569F,  -0.00384409539F,  -0.000579100102F, 0.00168589724F,
      -0.00304144528F,  -4.62716816E-5F,  -0.000236924709F, -0.00250286912F,
      0.00149876019F,   0.00248341681F,   0.0066131684F,    -0.00329861743F,
      -0.000286463124F, -0.000317552622F, -0.000103593258F, -0.00211487897F,
      -0.00230940105F,  -0.00536245247F,  0.0059840153F,    0.000712878886F,
      -0.000581895409F, -0.00010165065F,  0.00336751086F,   -8.22307697E-7F,
      -1.327273E-5F,    0.00023130374F,   -0.00176576083F,  9.39281E-6F,
      0.000150799329F,  -0.000127520776F, -0.000530267309F, -0.0011144086F,
      5.14563762E-5F,   0.000326894398F,  -0.00330682728F,  1.44141695E-5F,
      -0.000782322604F, -0.000975119649F, 0.00109610287F,   0.00110557151F,
      0.0021561156F,    -0.000533617858F, -0.00310291769F,  0.000144790014F,
      -0.0010256269F,   0.00424530311F,   -0.000152450943F, -0.00148287439F,
      -0.00207342021F,  -0.000849407865F, 0.0119932108F,    0.00454514194F,
      -0.000182128861F, 0.0144086163F,    0.00162861531F,   -0.000780305942F,
      0.00349279307F,   0.00252406136F,   -0.0028676244F,   -0.000578143925F,
      -0.0054622693F,   0.00231663114F,   -0.00194219186F,  0.000592018F,
      -0.000766567071F, -0.00358455116F,  -0.00136732846F,  -0.0174916349F,
      1.46602069E-5F,   -1.30600347E-5F,  0.00191745756F,   1.67093131E-5F,
      0.000437316136F,  0.00023415382F,   0.00299473759F,   0.000560754444F,
      0.00060969434F,   5.00386668E-5F,   -0.00219711568F,  0.00156658806F,
      -0.00116250769F,  -3.15053E-5F,     -8.80028347E-6F,  -2.12953601E-5F,
      -0.00998950377F,  0.00565182464F,   -1.34087159E-5F,  -0.00147542858F,
      -9.13184849E-5F,  0.00132937706F,   0.000882414F,     3.32027184E-6F,
      0.00121272192F,   5.92899596E-6F,   5.66179442E-5F,   -7.82490497E-6F,
      -0.00252069812F,  -0.000490448379F, -4.6128057E-6F,   -0.000649441208F,
      0.000157590737F,  0.0010792868F,    -0.00041455985F,  -0.000383277278F,
      -0.00249876874F,  0.000855196384F,  -0.000702227349F, 0.000769878156F,
      -0.000580672291F, -0.00410091504F,  -7.42261764E-5F,  0.000525502663F,
      -0.0021454927F,   0.00513273524F,   -3.70707785E-5F,  -0.000453728833F,
      -0.000356241595F, 5.29796889E-5F,   -0.000650519971F, -0.000225255586F,
      6.05650143E-8F,   -0.00185716455F,  2.39891193E-7F,   -0.00111545273F,
      0.00102939631F,   2.82283618E-5F,   0.000156881521F,  -0.00092172221F,
      0.000482309028F,  0.00131118565F,   0.000297331193F,  0.000552533544F,
      0.00313133118F,   1.62901779E-6F,   -0.000557236664F, 0.00058563333F,
      0.000351134571F,  -0.000428024767F, -0.00327819749F,  -5.66049421E-5F,
      -0.000779950176F, 0.000388088025F,  -0.000900095445F, -0.00134147168F,
      -0.000239813846F, -0.00280024158F,  -4.36084156E-6F,  0.000540043111F,
      -0.000312036776F, 0.000155777612F,  0.00108219974F,   -0.000619637722F,
      -1.04016408E-6F,  2.58432351E-6F,   0.000917020137F,  0.00046833232F,
      -0.00345716579F,  -0.000556990213F, 4.04993334E-5F,   -0.000990620931F,
      0.000647964131F,  0.00900976F,      -0.00145957F,     0.00037827043F,
      -0.00108379603F,  -0.00455356063F,  1.95089888E-6F,   0.00100878743F,
      1.18762983E-7F,   0.000169103121F,  -2.22167237E-5F,  -0.000263246766F,
      -6.8342968E-5F,   -0.000319360028F, -0.000682825805F, -5.5898164E-7F,
      0.00253772689F,   2.20269594E-5F,   -0.000396914722F, -0.000418025185F,
      0.00015029349F,   0.00170558388F,   -6.69145957E-5F,  0.00977218896F,
      -0.000692162372F, -0.00024337467F,  0.000980372F,     -5.88631394E-7F,
      -0.000645271095F, -0.000194869135F, 0.0026620077F,    1.91581151E-8F,
      0.0011851833F,    0.000662848062F,  0.00351505657F,   -4.85702323E-8F,
      0.00181777705F,   -0.000614257413F, 0.00011135314F,   8.32419537E-5F,
      -0.000200923736F, -0.00033673193F,  0.00131211197F,   -1.81388558E-7F,
      0.00241914275F,   -8.60247E-6F,     -0.0122067854F,   3.6605266E-7F,
      0.00486872112F,   0.00188065297F,   -0.000392324524F, 0.000527759083F,
      0.000128910426F,  -0.00918389764F,  0.00112014008F,   0.000252162601F,
      -0.000494620181F, 8.98041E-5F,      -4.70122141E-5F,  0.00101645233F,
      0.000177395617F,  -5.57862659E-5F,  -0.000302972767F, -0.0086837355F,
      -0.000427602237F, -5.59851833E-6F,  -2.3241575E-5F,   -0.000850267592F,
      -0.000506571785F, 0.000121999728F,  -0.000537700078F, -0.0041533364F,
      -0.00114617066F,  0.000812396465F,  6.49868809E-7F,   -1.44033459E-6F,
      0.00072668877F,   0.000884049281F,  -0.000276184961F, -5.89964941E-7F,
      9.24499446E-8F,   -2.05685865E-7F,  -0.000529556F,    7.3881929E-5F,
      0.00259627705F,   -0.000829039665F, 0.00051535F,      -2.77228683E-5F,
      0.000849962526F,  5.71458827E-8F,   -0.000639315287F, -0.000611476542F,
      -0.000241334026F, -0.00400276436F,  0.0016410572F,    -0.000132081041F,
      -0.0112096956F,   0.000235358559F,  -0.000166948332F, 0.00238432642F,
      -0.000169535968F, 0.00219109491F,   0.00255403668F,   -0.000691943F,
      -0.000416015595F, -4.56350936E-5F,  0.00147266767F,   -1.02735089E-6F,
      -0.00106963771F,  -0.00013324579F,  0.000465668F,     -0.000226206757F,
      -8.18237424E-8F,  -2.40063309E-5F,  -4.26517499E-6F,  0.000966400723F,
      -0.00217168056F,  1.08865549E-7F,   6.57477841E-8F,   6.66861865E-7F,
      7.17131406E-5F,   -0.000358786638F, -0.000257942476F, 0.00125974196F,
      -0.00264225062F,  0.00317060878F,   0.000321588072F,  0.00351989013F,
      -0.000343258464F, -0.000475382636F, -0.000937784789F, 0.00196453347F,
      0.00210295268F,   -0.00354318833F,  0.000248211902F,  -0.000217025197F,
      -0.000259515888F, -0.000162694399F, -0.000483855256F, -0.000830256206F,
      7.232801E-5F,     0.00108488498F,   -0.000461800548F, -0.00142866501F,
      0.000190513223F,  -0.00238715089F,  0.000384332641F,  -0.000686930609F,
      -0.00264534145F,  -0.000301186665F, 0.00083133881F,   -6.7719724E-5F,
      0.00148078182F,   -0.00228402275F,  1.25217787E-6F,   -0.0014458939F,
      0.00375667773F,   9.28989152E-8F,   -7.87453391E-5F,  0.000618499238F,
      0.000666853564F,  -0.000723190373F, 9.08144528E-8F,   9.78560593E-7F,
      -0.00905068F,     0.00132867217F,   -0.00166482909F,  0.000890279713F,
      0.0193887614F,    0.000124661936F,  0.000157294955F,  0.00146732293F,
      -0.00610592496F,  0.00346673606F,   3.81952191E-7F,   -0.000963421771F,
      1.93799133E-6F,   -0.00430528866F,  0.000305529917F,  0.00138899498F,
      -0.000893222459F, -0.000107704771F, -0.00160098635F,  -0.000356016419F,
      -0.000231904691F, -4.63609067E-5F,  -0.000662861F,    -1.7936225E-5F,
      0.00162277604F,   -0.000172902524F, 0.00495565729F,   -0.000136772112F,
      -0.000441026466F, 0.00223356881F,   -2.48727665E-5F,  -0.000983665814F,
      0.00508182915F,   1.0843944E-6F,    -0.00010617722F,  -0.000973979477F,
      -0.000154044043F, -9.73853093E-5F,  0.00239368877F,   -7.00704768E-5F,
      0.000672406692F,  0.000480921561F,  -0.00100308319F,  1.65155356E-7F,
      -9.071776E-5F,    -0.00131913228F,  0.000368805369F,  5.02834041E-8F,
      0.00161221623F,   0.00124900369F,   -0.00136656081F,  -0.000978007214F,
      0.00324095902F,   0.000422669778F,  -0.00123129296F,  0.000500977272F,
      1.19180044E-7F,   -0.000535226543F, -7.06380524E-6F,  -0.0010121332F,
      0.00130217418F,   -0.000422415964F, -0.00102740317F,  -0.00039051153F,
      -4.01165437E-7F,  -0.00165887643F,  0.000869072159F,  0.00528663397F,
      2.79177948E-6F,   3.44220243E-5F,   -0.000352635F,    -0.000311243697F,
      1.46634221E-7F,   0.000154146517F,  0.000124932296F,  0.00180438033F,
      -0.000326485082F, 0.000453686691F,  -0.00210963143F,  -0.000929327391F,
      -0.000132440982F, -3.77753568E-5F,  0.000268578733F,  0.0007337844F,
      1.69890227E-5F,   0.000309077848F,  0.00223580934F,   -0.00602967339F,
      9.17947364E-6F,   0.000231715429F,  -0.000363855448F, 0.000255803141F,
      -0.0022455235F,   -0.0156831667F,   0.00223794556F,   -6.88443179E-5F,
      -0.00064767868F,  0.000308684277F,  -0.000431922643F, -8.91612189E-8F,
      0.000211067687F,  -0.000569895317F, 0.000628498732F,  -4.08265578E-7F,
      -0.000359673169F, -0.000412282359F, -0.0010405652F,   -0.000359090016F,
      7.73134047E-7F,   -1.0769174E-5F,   -7.04462E-5F,     -5.14659604E-8F,
      -0.000142193327F, -0.000969909073F, -0.00055429F,     0.000598256243F,
      -0.00104646734F,  3.59918231E-5F,   -0.00128658663F,  0.00149602897F,
      -0.000972952112F, 0.00725401053F,   0.00183704752F,   0.00252638222F,
      -0.00465418678F,  -0.000283771165F, 0.000104799867F,  -0.0017065194F,
      -0.000412436522F, -0.000332503841F, -0.000551979407F, 0.000350930379F,
      0.00216443976F,   -0.000522947F,    -0.000301574968F, 0.000241645714F,
      -0.000534963969F, -0.000414495473F, -0.00148209766F,  -0.000312389457F,
      -0.000678326469F, -0.000887024566F, -5.36687439E-5F,  -0.00513844425F,
      -2.34258607E-7F,  -5.05592482E-7F,  0.000793138519F,  -0.000495796965F,
      2.44689581E-5F,   -0.000415282208F, 0.00209183828F,   -5.58289794E-5F,
      -0.00321734953F,  -0.000987460837F, 3.36667144E-5F,   -0.000423887424F,
      0.000112940157F,  -1.00013222E-6F,  -1.42809745E-6F,  1.37473717E-6F,
      -0.000898805854F, 0.00342704984F,   -9.94964466E-8F,  -0.000386862113F,
      -8.97002246E-6F,  0.000990600791F,  -0.000792257255F, 2.56307459E-7F,
      0.00149250601F,   3.60995699E-7F,   1.803283E-7F,     -2.7594017E-6F,
      0.00142843893F,   5.74976511E-5F,   -2.55965098E-8F,  -0.000411171059F,
      0.00027668511F,   0.000191464438F,  -0.000214549204F, -0.000406398874F,
      0.0041371244F,    -0.000259377499F, -0.000512571F,    7.67849124E-5F,
      0.000444031815F,  -0.000147597442F, -2.41813427E-6F,  0.000643260777F,
      0.000276057021F,  0.00191366009F,   0.00120859302F,   -0.000990478671F,
      -0.000905316905F, -0.000270580465F, -0.000554436294F, -0.000697356882F,
      -6.50180482E-6F,  0.00175932329F,   -1.87074505E-7F,  -0.00181808707F,
      0.00177165016F,   0.000704746111F,  -0.000609510054F, -0.00103087607F,
      -0.00126689381F,  -0.0220004339F,   0.00220116763F,   0.000610114424F,
      0.00994851F,      4.44014404E-5F,   -0.0050572129F,   0.00307815592F,
      -0.000983981881F, -0.000860925706F, -0.00314769591F,  0.00153971231F,
      -0.00141894026F,  -0.00261958409F,  -0.00313783297F,  0.00205085892F,
      -0.00449502328F,  -0.00329121761F,  -5.45460789E-5F,  -0.00497362111F,
      0.00316603901F,   -0.0020755718F,   0.00144696212F,   -0.00227026735F,
      -2.7318325E-5F,   -2.63893853E-5F,  0.00841834862F,   -0.000482959673F,
      -0.000135965063F, -0.000306848728F, -0.00204238459F,  -0.00121873512F,
      -0.0027364695F,   0.0207271278F,    -0.00805950165F,  -0.000828904856F,
      -0.000950173591F, -0.00574493408F,  -0.000466708909F, 0.00681707915F,
      -7.03526894E-7F,  -0.00210177572F,  -0.000599672727F, -0.000830670353F,
      -0.000713150424F, 0.000148509163F,  -0.00346527644F,  -2.517346E-5F,
      0.0135116717F,    -0.00276829721F,  -0.00156729796F,  -0.00232079509F,
      -5.0501927E-5F,   0.00121280167F,   -0.00130546535F,  0.00792968273F,
      -0.00196553674F,  0.00127247209F,   0.007454623F,     -1.11008676E-5F,
      0.000701395154F,  -0.000276232255F, 0.000784944161F,  -2.75580305E-7F,
      0.00263328152F,   0.0055037667F,    0.00177942647F,   -2.23767825E-6F,
      0.00278757955F,   -0.00104876759F,  0.00141231227F,   -0.000438002317F,
      -0.00208542985F,  -0.00109180436F,  0.00112940744F,   -1.95823741E-5F,
      -0.00036781776F,  0.000503774092F,  0.00571201649F,   -2.68506574E-6F,
      0.00352289039F,   0.0125315646F,    0.000455222646F,  0.00520749576F,
      0.000347682653F,  -0.0112430537F,   0.00019149654F,   -0.00384429214F,
      -0.0031567819F,   0.00114962226F,   -0.00055224041F,  0.00123782328F,
      -0.000658832898F, -0.00111113128F,  -0.00173901394F,  0.00167939055F,
      -0.000723194156F, -1.94585391E-5F,  0.000679622055F,  0.00310925883F,
      -0.00573641527F,  -0.00276948907F,  -0.00210651592F,  -0.0865276307F,
      -0.00799148F,     0.0039662919F,    6.70023292E-6F,   -8.6989E-6F,
      0.000799075176F,  0.00515406113F,   -0.00248437421F,  -0.000354347809F,
      -1.46105667E-5F,  2.65709714E-5F,   -0.00303904759F,  0.000237696091F,
      0.000839697081F,  0.00251528784F,   0.00227491581F,   0.00208421447F,
      0.00185435126F,   2.46844229E-5F,   -0.00354624679F,  -0.000671897666F,
      -3.59030014E-6F,  -0.00368319545F,  -0.00174005143F,  0.000199998743F,
      -0.0121639045F,   0.00152115955F,   -0.000751565793F, 0.001288992F,
      0.000678480719F,  0.00850675534F,   0.0108413706F,    -0.00361223659F,
      -0.00226686569F,  0.000321792846F,  0.00861854851F,   -3.48054309E-5F,
      -0.00474175531F,  -0.000581100117F, -0.000156439972F, 0.00187289726F,
      1.9968471E-5F,    0.000669713307F,  1.72258697E-5F,   -0.00236882595F,
      -0.00235721073F,  2.32100092E-7F,   1.15750245E-5F,   -3.18918756E-5F,
      -0.00403148588F,  -0.00188854639F,  -0.00185669516F,  0.00288299541F,
      0.0008928405F,    -0.000108252032F, -0.00185622205F,  0.00992874429F,
      -0.000565615948F, -0.00406952901F,  -0.00153614988F,  0.00313397F,
      0.00222004F,      -0.00451397337F,  -0.00135677203F,  -0.000929786125F,
      0.000423896738F,  0.000209580539F,  -0.00130681659F,  0.00180187763F,
      -0.000844739494F, 0.00569560286F,   -0.000869892247F, -0.00303014112F,
      -0.00194344483F,  -0.00158216513F,  -0.00312555954F,  -0.00678705F,
      -0.00108546403F,  -0.000296348211F, -0.00511725945F,  0.00051975681F,
      0.00261350069F,   0.00120453618F,   -2.95049558E-5F,  0.00195598439F,
      0.0139668304F,    2.716979E-7F,     -0.00073891F,     0.00298976339F,
      0.00156981032F,   -0.00429279357F,  1.20747745E-5F,   -7.05095681E-6F,
      -0.0672391877F,   -0.00128747954F,  -0.00546480389F,  0.00176157837F,
      0.018181067F,     -0.000925427303F, -5.91967546E-6F,  -0.0043084207F,
      -0.00364006124F,  0.00978341F,      1.95708108E-5F,   0.000392989983F,
      0.000629582384F,  -0.0435516164F,   0.00178028154F,   0.00089847378F,
      -0.00316148507F,  0.00204826915F,   -0.0202243179F,   -0.000207692385F,
      -0.00222496875F,  -0.000593348F,    -0.00476196222F,  -0.00108368939F,
      0.00546776F,      -0.00222789403F,  -0.00303569017F,  -0.00115013262F,
      0.000462797791F,  0.0126115326F,    -0.000757590111F, -0.0020811744F,
      0.00730250496F,   -1.94641925E-5F,  -0.00519294757F,  0.00133368501F,
      -0.00291226804F,  0.000368896552F,  0.00106843433F,   -0.0019545F,
      0.000174010114F,  0.000122669953F,  -0.0042621321F,   1.60827476E-5F,
      0.000617175712F,  -0.00302431686F,  -0.00670743035F,  4.98964891E-6F,
      -0.000264213624F, 0.00312933326F,   -0.00732701132F,  0.00219172426F,
      -0.00247349427F,  -0.00202496257F,  0.00769765908F,   -0.000598822488F,
      2.60053876E-5F,   -0.00203919061F,  0.00043366375F,   -0.00311439531F,
      0.00240932941F,   -0.00343307178F,  -0.00368479383F,  -0.00101666967F,
      -3.1405878E-5F,   -0.00356597942F,  0.00373284565F,   -0.00228113122F,
      5.74445949E-8F,   0.0019404164F,    -0.00160106469F,  0.00143149972F,
      -9.21497758E-8F,  0.000270294375F,  0.00095987F,      0.00103838416F,
      -0.00232775742F,  -0.00203328836F,  -0.00100378448F,  -0.0202542767F,
      -0.00288665388F,  -6.73033792E-5F,  -0.000521369628F, -0.00214108941F,
      0.00126311951F,   0.00130577362F,   0.00921861548F,   -0.0053782016F,
      -0.000245117786F, -0.000156470021F, 0.00018615053F,   0.00153858203F,
      -0.00301476358F,  -0.0196457412F,   0.00784728304F,   -0.000794535852F,
      -0.000626558554F, 0.000302499539F,  0.00223742961F,   -9.09492655E-7F,
      -0.00080589694F,  0.000331549323F,  -0.000646717846F, 9.7569955E-6F,
      0.000512951694F,  -0.0007248724F,   -0.000475541427F, -0.00145359652F,
      5.13048617E-5F,   8.65181646E-5F,   -0.0035223458F,   1.53416986E-5F,
      0.000483318348F,  -4.23084457E-5F,  -0.000205853037F, 0.000162415672F,
      0.00260469387F,   -0.00130364893F,  -0.00352760055F,  0.00248485501F,
      -0.000331669551F, 0.000861688051F,  0.000735329406F,  -0.00116138393F,
      -0.00395637052F,  0.000115215618F,  0.00191561587F,   0.00138252298F,
      -0.000680791738F, -0.00270125642F,  0.00136553717F,   -0.001452761F,
      0.00391785195F,   -5.60121116E-5F,  -0.00142688933F,  -0.000671288697F,
      -0.00425610784F,  0.00263032061F,   -0.00203758595F,  0.000267575757F,
      -0.000753610861F, -0.0013226812F,   -0.00224311673F,  -0.123240069F,
      1.24001772E-5F,   -8.29528108E-6F,  0.000546451833F,  -0.000989828492F,
      -0.00022107226F,  0.000943021965F,  0.00308996788F,   0.000492428779F,
      0.00154850539F,   -0.000267575146F, -0.00153733103F,  0.00228367234F,
      -0.00107291178F,  -3.0848405E-5F,   -9.85276347E-6F,  -2.12620489E-5F,
      -0.0151785677F,   0.00710098585F,   -1.33586645E-5F,  -0.00223927503F,
      -0.000185353521F, 0.0012356654F,    0.00183174247F,   2.95938071E-6F,
      0.000853509177F,  6.0238217E-6F,    5.91040298E-5F,   5.64618222E-6F,
      -0.000809745F,    -0.000249823206F, -5.29891895E-6F,  0.000352010073F,
      8.08183813E-6F,   0.0029377467F,    -0.000967827509F, -0.00316951703F,
      0.0105116786F,    -0.000888024166F, -0.000553193851F, 0.00161534618F,
      -0.00111381919F,  -0.00217665266F,  -7.99514237E-5F,  -0.000393206486F,
      -0.00123631337F,  0.0146604525F,    0.0014596628F,    -0.000443209836F,
      -0.00686865207F,  -0.00942524F,     -0.011525264F,    0.00449397974F,
      -0.00156124495F,  0.0147918575F,    -0.00197958364F,  -0.0320340395F,
      0.0330131203F,    0.028766254F,     -0.0123010175F,   0.0326623246F,
      -0.00444224523F,  -0.119348772F,    -0.0138761718F,   -0.0167294536F,
      -0.0279483721F,   -0.00224563736F,  0.0472462587F,    0.0130637363F,
      0.028516354F,     -0.023316063F,    -0.0321405046F,   -0.0175819658F,
      -0.0682657808F,   -0.00554953283F,  0.0231550224F,    -0.0491914116F,
      -0.000158374547F, 0.0265680887F,    -0.00136397674F,  -0.0418447256F,
      0.0801156685F,    -0.00298535661F,  0.0036009294F,    0.0471965484F,
      -0.00118339865F,  -0.00186220452F,  0.152424932F,     0.00807514414F,
      -0.153717116F,    -0.00256074429F,  0.0290923379F,    0.0236273576F,
      0.0488731787F,    -0.0335815214F,   0.0415658168F,    -0.0295326393F,
      0.0155699123F,    -0.0234109201F,   -0.00645803334F,  0.0292472281F,
      -0.000512772705F, -0.0531717762F,   -0.00462948577F,  0.0192644093F,
      -0.00442852592F,  -0.0105818277F,   0.0303467605F,    -0.00296941912F,
      0.0100204907F,    0.00443351688F,   -0.0319474414F,   0.0143002383F,
      -0.0173403304F,   -0.0104886424F,   -0.00781358313F,  0.0105592981F,
      0.0431140698F,    -0.0107821589F,   -0.0292712562F,   -0.00221737521F,
      0.00545813935F,   0.0185759105F,    -0.0173867606F,   -0.00188694382F,
      0.0359266065F,    0.00880408753F,   -0.00703526102F,  -0.0018975276F,
      0.0429787748F,    0.020967247F,     -0.070534572F,    0.0177585017F,
      -0.0271812826F,   0.0512214862F,    0.00785573199F,   0.000144250225F,
      0.0300384369F,    -0.0351371206F,   0.458617479F,     -0.00275222026F,
      0.0202510562F,    0.104553752F,     -0.026489405F,    0.00141967565F,
      -0.00158693339F,  -0.209635481F,    0.0294696614F,    -0.00360614294F,
      0.0231268387F,    0.0139349569F,    -0.00626354944F,  0.0623747893F,
      -0.000451514177F, 0.000323699758F,  -0.0375578403F,   -0.01694322F,
      0.0138339316F,    -0.00312540517F,  0.0493383184F,    0.0281113759F,
      0.164575264F,     0.0352236107F,    0.12041638F,      0.0260954741F,
      -0.123429209F,    0.036980439F,     -0.00144131936F,  -0.00246315286F,
      0.0123664234F,    0.00397537369F,   -0.0106557608F,   -0.00697006F,
      -0.00296230661F,  -0.00187155372F,  -0.00453304732F,  -0.0025017336F,
      0.183578715F,     -0.0144064166F,   0.0298012346F,    0.0110484846F,
      -0.045303259F,    -0.00132348F,     -0.0276319776F,   -0.0169388447F,
      0.00697500631F,   -0.0936293826F,   -0.0137972739F,   -0.00558006344F,
      -0.0120656956F,   -0.0289825778F,   -0.00982612465F,  0.10157688F,
      0.0236914791F,    -0.0136976838F,   -0.0494871736F,   0.0188089833F,
      0.049305059F,     0.0201183092F,    -0.0368809849F,   -0.000891131407F,
      -0.0893468559F,   0.0468174629F,    0.0103646126F,    -0.023971932F,
      -0.00550787058F,  0.0339302905F,    -0.0079084076F,   -0.0391223617F,
      -0.0600245148F,   -0.00214393204F,  -0.0018292747F,   -0.00262692152F,
      -0.0560654551F,   0.00386944762F,   0.00360190962F,   0.0887055472F,
      -0.0417188518F,   -0.0156072509F,   -0.0130255362F,   -0.00485743769F,
      -0.0310218483F,   -0.0458611809F,   -0.0646903887F,   -0.084483467F,
      0.0164855346F,    -0.017776249F,    0.0118639404F,    0.00521624926F,
      0.0165377539F,    -0.00535147078F,  -0.0649863854F,   0.00287745101F,
      -0.0547026582F,   -0.0163135976F,   -0.00704899197F,  -0.0610399432F,
      0.0149369054F,    0.0461114943F,    -0.0121291298F,   0.169363946F,
      0.0249304567F,    -0.0484328158F,   -0.000969537592F, 0.0042802766F,
      0.0122482823F,    0.0635906532F,    -0.00574691407F,  0.0971849933F,
      -0.0446757711F,   -0.00262843771F,  -0.00257544871F,  -0.0834434405F,
      0.00739394873F,   -0.00152714923F,  -0.00311892899F,  -0.0021380859F,
      -0.0551935062F,   0.0125708375F,    0.00332463067F,   0.116671458F,
      -0.00245557725F,  0.0437380299F,    -0.0394903272F,   0.112166986F,
      -0.136568248F,    -0.00279223546F,  -0.00599852158F,  -0.080967389F,
      0.00610055216F,   -0.0190902855F,   -0.0204526894F,   -0.0896021575F,
      0.0134202577F,    0.0449388325F,    0.0965502784F,    0.00056924962F,
      0.0251092613F,    -0.0164758246F,   0.015187813F,     0.0325734392F,
      0.0163197313F,    -0.00321589271F,  -0.012240204F,    -0.0104976119F,
      -0.0592403561F,   0.119473167F,     0.00747740408F,   -0.0607305F,
      0.110857695F,     -0.00249154214F,  -0.0837121904F,   0.0184110906F,
      0.0486611798F,    0.0407211706F,    0.0707518086F,    0.0180048943F,
      0.0145766009F,    -0.019090252F,    -0.0280346088F,   -0.00113710912F,
      -0.0490212105F,   0.0136151342F,    -0.0851892382F,   -0.00244097295F,
      -0.0179456864F,   0.029953761F,     -0.0850033313F,   -0.012992329F,
      0.0122304326F,    -0.0609354377F,   0.22362113F,      -0.00513059739F,
      -0.00052476913F,  0.0024741462F,    -0.00550197111F,  -0.0347536542F,
      0.0328168049F,    -0.0220565498F,   0.0103854425F,    -0.0277583096F,
      -0.00173360924F,  0.0215455722F,    -0.00347126089F,  0.00321998307F,
      -0.00171985407F,  -0.00207236968F,  0.00403263792F,   0.0864769F,
      -0.00179265009F,  -0.011477408F,    0.0120468531F,    0.043847423F,
      -0.00569285732F,  -0.0558562167F,   -0.0508143455F,   0.0154587524F,
      0.0220789593F,    -0.00405501807F,  -0.00928577222F,  -0.0376343094F,
      -0.0121626798F,   0.0220762249F,    -0.041908F,       -0.00694452459F,
      -0.00463061407F,  -0.00455898745F,  -0.0245460514F,   -0.00108405598F,
      0.0113417916F,    0.0559490286F,    0.0042371084F,    -0.00293709827F,
      0.0273217261F,    -0.118767403F,    0.0280825943F,    -0.00137310044F,
      0.0042801206F,    0.00475762598F,   0.026177749F,     -0.00208459678F,
      0.010057725F,     -0.0113659622F,   0.0290140156F,    0.0100100804F,
      -0.0012755004F,   -0.00131443574F,  0.0413252264F,    -0.00130455557F,
      0.00251522055F,   0.0333328918F,    -0.0483489931F,   0.0294004865F,
      0.0472123846F,    -0.0214729477F,   -0.0317377597F,   -0.032455571F,
      0.023778934F,     0.0492023863F,    0.0186711326F,    0.0300753564F,
      0.133102462F,     0.00372185837F,   -0.032452926F,    0.143474877F,
      0.00274753431F,   -0.0415455215F,   0.00192643446F,   0.00305885798F,
      -0.0835157186F,   0.0200600009F,    -0.0303656049F,   -0.00272838236F,
      0.0179258268F,    0.00797419529F,   0.0774647221F,    0.0281909741F,
      -0.0250625238F,   0.0149086807F,    -0.0242947284F,   -0.0252799392F,
      -0.00203324133F,  -0.00265625166F,  0.0164179336F,    0.0729872659F,
      0.00665205857F,   -0.043807812F,    0.0294117108F,    0.0194195062F,
      0.0716371238F,    0.0114522846F,    -0.0441396348F,   0.051250644F,
      0.0219993982F,    -0.000929543865F, -0.00186388544F,  -0.00316384854F,
      0.0714101344F,    0.000205669421F,  -0.00603885064F,  -0.0195062365F,
      -0.00454815757F,  -0.0225360561F,   -0.00323132193F,  -0.00363527914F,
      0.0436791182F,    -0.00166003837F,  -0.00133614684F,  -0.00317442138F,
      -0.039589636F,    -0.000718942087F, -0.00446849177F,  0.0415370949F,
      -0.0246409662F,   -0.00205520564F,  -0.00513570523F,  0.0223415568F,
      0.0364433601F,    0.0114698093F,    -0.0414965339F,   0.0448225066F,
      0.0262142811F,    -0.0411966965F,   -0.00352199492F,  0.0823013932F,
      -0.0375965908F,   0.060425F,        -0.043486841F,    -0.0537722148F,
      -0.00453747F,     8.76551785E-5F,   0.0130886463F,    -0.01012938F,
      7.39599818E-6F,   0.00461578136F,   -1.66477821E-6F,  -0.00435255701F,
      0.00183088647F,   0.000473358668F,  0.012732137F,     -0.0115476679F,
      -0.00307059358F,  -0.184860662F,    0.00987383444F,   -0.00257022888F,
      0.0169995707F,    3.15115758E-5F,   0.00210503745F,   0.00943631F,
      -0.000395529583F, 0.00241510686F,   -0.0169227067F,   0.0323380232F,
      -0.0360195562F,   0.000111500463F,  0.00347552286F,   -0.0201108065F,
      -0.0110844914F,   -0.0540666394F,   -1.56564456E-5F,  0.00595618226F,
      0.0045998157F,    0.0163887367F,    -0.00166674459F,  -0.0105201118F,
      -2.61783553E-5F,  3.30599505E-5F,   0.00093707745F,   -0.00338786957F,
      -0.0381998308F,   -0.00873891637F,  -0.0010980136F,   -0.0160167832F,
      -0.0119863963F,   -0.014075039F,    -0.0173068102F,   -0.00167570391F,
      -0.00166182464F,  -0.0458007269F,   -0.00026305471F,  0.00667516375F,
      -0.000158949028F, -0.00593041489F,  -0.000574455713F, -0.00274943095F,
      -0.000192253792F, -0.00100315327F,  0.00300726085F,   -9.73238366E-6F,
      -0.00274772313F,  0.00202520168F,   0.000706491643F,  0.037269827F,
      -0.00102657499F,  -0.0234860722F,   -0.00216104416F,  0.150049284F,
      -0.00462159188F,  0.0025742834F,    0.00923787616F,   -4.4892171E-5F,
      0.000552231329F,  -0.0374358632F,   -0.00400787964F,  2.12919349E-5F,
      0.00578467129F,   -0.0287171621F,   0.046962589F,     -2.1198407E-6F,
      0.04108F,         0.017475903F,     -0.0158235077F,   -0.00218442664F,
      -0.000830664125F, -0.0053672404F,   0.0185054503F,    -2.38170196E-5F,
      0.0631411374F,    -0.00117324409F,  -0.223788589F,    1.66234549E-5F,
      0.0998362675F,    0.0291008428F,    -0.00280577294F,  0.00493105082F,
      -0.00272661471F,  -0.266240716F,    0.0209285617F,    0.000707035651F,
      -0.0031808177F,   -0.00194149616F,  -0.000512734638F, 0.00707431044F,
      -0.00304789678F,  -0.00454371F,     -0.00151070091F,  -0.07194525F,
      0.0016758017F,    -0.000236274529F, 0.00636288058F,   -0.00722349482F,
      -0.00757196872F,  -0.0311800819F,   -0.0259413812F,   0.000392506161F,
      -0.0729069859F,   0.0255751852F,    2.83183704E-6F,   5.17932203E-5F,
      0.00213747239F,   0.00842557661F,   -0.00137574435F,  0.000303488137F,
      1.12584448E-5F,   -4.38827E-7F,     0.0191278663F,    1.64091507E-5F,
      0.0779262409F,    0.00477839867F,   -0.00142076658F,  -0.00522276107F,
      0.00890669506F,   -5.74801834E-6F,  0.0977358446F,    -0.000385310152F,
      0.010542999F,     -0.0914774612F,   0.0261522569F,    0.0461603217F,
      -0.127146363F,    -3.58464058E-5F,  -0.000962289283F, 0.0193168614F,
      -6.32493102E-5F,  0.0284223929F,    -0.018996628F,    -0.0111982645F,
      -0.0435082577F,   -0.000640741724F, -0.00265425304F,  -1.29499549E-5F,
      0.0417544767F,    0.0102518918F,    -0.00377029786F,  -0.00884360168F,
      4.5895591E-5F,    7.54644134E-5F,   -1.20611967E-5F,  0.0565191358F,
      -0.0058779805F,   -2.6233256E-6F,   -1.22764686E-6F,  1.31822899E-5F,
      0.010716713F,     -0.00612985F,     -0.0189764854F,   0.00151645031F,
      -0.103111558F,    0.0493794195F,    -0.00626846636F,  0.0411053F,
      0.00380443549F,   0.00165861275F,   -0.00130155287F,  0.000553186F,
      0.00333513506F,   -0.0943460763F,   0.00336103304F,   -0.00284197275F,
      -0.00160505F,     -0.000655013893F, -0.014004509F,    -0.00848488603F,
      -0.000226395889F, 0.00114027131F,   -0.000896894489F, -0.0165120419F,
      -0.00976843573F,  -0.00363050075F,  0.0271431506F,    -0.0211667772F,
      -0.0192834586F,   -0.00111307076F,  0.00677319383F,   0.00119594671F,
      0.0858294293F,    0.035486076F,     6.26683759E-5F,   -0.071679391F,
      -0.0413738117F,   -8.70111E-6F,     -0.00469427463F,  -0.00533429859F,
      -0.0188055094F,   0.000384548941F,  6.39443488E-6F,   2.88473748E-5F,
      -0.0188491605F,   0.0031484561F,    -0.0554171912F,   0.000420818425F,
      0.252559632F,     0.00178148819F,   0.00343834027F,   -0.000333509961F,
      -0.127107412F,    0.0284240432F,    3.30653456E-5F,   -0.019080922F,
      -0.00590422098F,  0.0446648709F,    0.00348512409F,   0.0715444461F,
      0.00620130822F,   0.0050380053F,    -0.0335134715F,   -0.00191115064F,
      0.0126404418F,    0.00368336192F,   -0.014375749F,    -0.0015924488F,
      -0.000433315086F, -0.0012309663F,   0.046163246F,     -0.000954786199F,
      -0.00201960607F,  0.0294693168F,    0.00963504F,      -0.0348790847F,
      0.125184327F,     -6.65909101E-6F,  0.012276805F,     0.00147994445F,
      -0.0343494043F,   -0.00651057158F,  0.016961F,        0.0051425295F,
      0.00416751718F,   -0.00346220261F,  -0.00674111536F,  -1.28013653E-5F,
      -0.00734256394F,  -0.0169887207F,   -0.0903322175F,   -1.8604871E-5F,
      0.0374461263F,    0.00843305327F,   -0.066276595F,    -0.0893350765F,
      0.0379503779F,    0.00650409237F,   -0.00463662855F,  -0.00113333773F,
      2.04795306E-5F,   -0.0089426646F,   6.6233988E-5F,    0.00619515916F,
      -0.0512863658F,   -0.0186431129F,   0.00266616791F,   -0.0160625484F,
      -4.60170259E-6F,  -0.00986900367F,  0.00565192243F,   0.0471505821F,
      0.000170877378F,  -0.0108535811F,   0.00437774649F,   -0.0632747F,
      2.23533038E-6F,   -0.0177193545F,   0.00146832701F,   -0.00609495491F,
      -0.00309435977F,  0.00806932058F,   -0.0166497566F,   -0.0193406083F,
      0.000853965699F,  -0.000646000379F, -0.00193321507F,  0.0300355032F,
      0.0218330529F,    -0.0252136383F,   0.0182027612F,    -0.123052545F,
      -9.6054282E-5F,   -0.000764288823F, -0.000746982289F, 0.0231253505F,
      0.0399527F,       -0.373940051F,    0.0462000892F,    0.00273191463F,
      0.0308113378F,    -0.00812336616F,  -0.0226873476F,   -1.29780292E-5F,
      -0.000534033636F, -0.00717448397F,  -0.0368777253F,   1.23707978E-5F,
      0.00562387053F,   -0.00653780485F,  0.085629F,        0.000874772493F,
      5.57179519E-5F,   0.00050094584F,   -0.00481502339F,  2.7127362E-6F,
      0.00427070353F,   0.0104562985F,    -0.0068336362F,   0.00594123779F,
      -0.00661524618F,  -0.00394165516F,  -0.003062431F,    0.05022984F,
      0.00418087514F,   0.21438849F,      0.0120786708F,    0.0737456828F,
      -0.091991F,       -0.0137403421F,   0.00102413853F,   -0.0106307082F,
      0.0374462418F,    -0.013818454F,    0.000458038761F,  -0.00291326875F,
      0.0223504119F,    0.00756849954F,   -0.0823039189F,   -0.00147652416F,
      -0.00102823647F,  0.00789073855F,   -0.00739028F,     -0.0107781133F,
      -0.0024946325F,   -0.00427156407F,  -0.00258623878F,  -0.0226627942F,
      9.08500283E-7F,   2.99685926E-5F,   0.00306601496F,   -0.00158771547F,
      0.0042137336F,    -0.0324324779F,   0.0507338978F,    -0.00128364202F,
      -0.0329697877F,   -0.00177360035F,  0.00787202455F,   -0.00260705478F,
      0.00119643752F,   6.05915466E-6F,   -4.27800842E-5F,  1.44333535E-5F,
      -0.0938652083F,   0.0394122563F,    -6.39649943E-6F,  -0.00592790078F,
      -0.000485533499F, 0.0401520208F,    -0.00859235693F,  3.63790605E-5F,
      0.0500266887F,    -5.58928332E-6F,  8.04520314E-5F,   4.5843226E-5F,
      0.0218254142F,    0.000859198219F,  -3.01648015E-5F,  0.00304540317F,
      -0.0007424957F,   0.00536153512F,   -0.0005044383F,   0.0431798175F,
      -0.193358809F,    -0.00601687143F,  0.00351903355F,   -0.00818584859F,
      -0.0144377006F,   0.0108305365F,    -0.000372290931F, 0.00150129199F,
      0.060702242F,     0.0235975273F,    0.0830339491F,    -0.00161247107F,
      -0.00358744152F,  -0.00710185943F,  -0.00284384494F,  0.00140913879F,
      -0.00158492837F,  0.0155509273F,    -0.00190361927F,  -0.0129980566F,
      0.0915718898F,    0.0130156847F,    -0.00739686051F,  -0.00557038095F,
      -0.00774596957F,  0.419506788F,     -0.0230462737F,   -0.0428642593F,
      0.0279713608F,    -0.00216418761F,  0.00143897743F,   0.0442505702F,
      0.00655937335F,   0.00696173683F,   -0.087448F,       -0.0155988596F,
      0.105046764F,     -0.020983668F,    0.0225097016F,    0.0396928713F,
      -0.0979488343F,   -0.125353664F,    -0.00115656969F,  -0.0606693961F,
      -0.00834765285F,  -0.0267985072F,   -0.00957021303F,  0.0258113258F,
      -0.00111077807F,  -0.00172340812F,  0.151805446F,     0.00665425649F,
      -0.0874276236F,   0.00599947199F,   -0.0268752333F,   0.0338315964F,
      0.0427368507F,    0.00961930677F,   -0.0828220695F,   0.0250961632F,
      0.0292970929F,    -0.0162271224F,   -0.00474097114F,  -0.0328888632F,
      -0.000466737751F, -0.027535893F,    -0.00557904085F,  -0.0469023921F,
      -0.00767782331F,  -0.0155836679F,   0.000174457833F,  -0.00307181361F,
      -0.015339952F,    -0.0260088407F,   0.00717799552F,   0.030093722F,
      -0.0185159743F,   0.00975147448F,   -0.0138935894F,   -0.050892286F,
      0.0469213948F,    0.0282531586F,    -0.0112248743F,   -0.00231282623F,
      0.00772784557F,   0.027397804F,     0.0251420941F,    -0.00193150307F,
      0.0553981066F,    0.0027663291F,    0.101038046F,     -0.00184538797F,
      0.0535100326F,    0.00127890683F,   0.0536441542F,    0.0106909964F,
      -0.0579154715F,   0.0280385353F,    -0.0314954072F,   0.000281084533F,
      0.0481184088F,    -0.013279005F,    -0.150751635F,    -0.00273538847F,
      0.14890705F,      -0.0313222446F,   -0.0194536671F,   -0.0265731178F,
      -0.00514310645F,  0.0016606038F,    0.0558702946F,    -0.0422945432F,
      0.0244132504F,    0.013912404F,     -0.00792235322F,  0.0719175F,
      -0.0250222459F,   -0.0263614543F,   -0.00626815297F,  0.0252361055F,
      0.0166678596F,    -0.00340295909F,  0.0093438318F,    -0.0443020463F,
      -0.0502445251F,   -0.0132489307F,   0.135772064F,     -0.0638903156F,
      0.196561977F,     -0.0142124547F,   -0.00139827468F,  -0.00247416808F,
      0.0034279048F,    -0.0411467142F,   -0.0182666108F,   -0.00494772801F,
      -0.00288783596F,  -0.00178485562F,  0.100442134F,     -0.00175492023F,
      0.0244922079F,    0.0214424785F,    0.00794023741F,   -0.0331739932F,
      0.0474153981F,    -0.00121774094F,  -0.0308417976F,   -0.00235767383F,
      0.0322102681F,    0.141095981F,     0.0183685664F,    -0.070338808F,
      -0.0209188964F,   -0.00500500528F,  -0.0189165939F,   0.0550204292F,
      -0.0432117842F,   -0.0392608345F,   -0.0324128903F,   0.00903544F,
      0.000778355461F,  -0.00323533197F,  -0.0072015957F,   -0.00108600967F,
      -0.0815814361F,   -0.00476447679F,  0.0182074495F,    -0.00316203432F,
      -0.00551183941F,  -0.0293225814F,   -0.00280094775F,  0.0627665892F,
      -0.0275012627F,   -0.0022131037F,   -0.00161108223F,  -0.0025760869F,
      0.00864818878F,   0.0189360157F,    -0.0208341572F,   -0.297763199F,
      0.0557373054F,    0.0267181769F,    -0.0511059165F,   0.0138594778F,
      -0.0146387154F,   0.032749109F,     -0.0601273552F,   -0.0437824093F,
      0.062131308F,     -0.118492663F,    0.0103622507F,    -0.0190732088F,
      -0.025762707F,    -0.00623017224F,  0.158284739F,     0.0148422038F,
      -0.0420675874F,   -0.0397501104F,   -0.00303176627F,  0.0614507683F,
      -0.0512046069F,   -0.00712632807F,  0.0743606314F,    -0.0975603387F,
      -0.100934654F,    -0.0121221039F,   0.0128991716F,    0.0299485251F,
      0.0241338313F,    0.203356907F,     -0.00569736864F,  -0.0286942087F,
      -0.00875157211F,  -0.00268089F,     -0.0346411206F,   0.0183987897F,
      0.0189527217F,    0.0288806316F,    -0.00306243473F,  -0.00223618955F,
      0.00720873894F,   0.112220518F,     -0.00466879131F,  0.131556839F,
      -0.00955524761F,  -0.0393535495F,   0.0536426306F,    0.12878105F,
      0.105231605F,     0.0108747836F,    -0.00604924466F,  0.155762851F,
      -0.0514032915F,   -0.127869204F,    -0.0253902115F,   -0.0206353683F,
      0.0267929398F,    0.0817435607F,    0.215282366F,     -0.0440404713F,
      0.0428708568F,    -0.0264667124F,   0.0106350891F,    -0.0458469465F,
      -0.0383773968F,   -0.0158144329F,   0.0269951709F,    -0.0243799407F,
      -0.0188230779F,   -0.0146228867F,   0.0197180174F,    0.168000549F,
      0.000917923287F,  -0.00231873803F,  0.039763011F,     -0.0423587188F,
      0.130293712F,     0.0536992177F,    0.075288564F,     0.0221991185F,
      -0.0227686726F,   -0.0173157342F,   -0.0297064222F,   -0.00115125382F,
      -0.00256854412F,  0.0274870805F,    0.273706585F,     -0.00259843469F,
      0.0415415F,       0.0970095769F,    0.133721769F,     0.0311267022F,
      -0.0182275604F,   0.00509398524F,   -0.0569120981F,   -0.020448247F,
      -0.000471205072F, 0.00345402909F,   0.00397577742F,   -0.0365872085F,
      0.058376193F,     0.00371376239F,   -0.00713428948F,  -0.0364787653F,
      -0.00172459462F,  0.0248198099F,    0.00324366684F,   -0.0376552157F,
      -0.00155195873F,  -0.0288998429F,   0.0162265543F,    -0.0125531508F,
      -0.00175862317F,  0.0405440591F,    -0.038779784F,    0.0569362193F,
      -0.0554213859F,   0.00105611992F,   0.0831473619F,    -0.343414813F,
      -0.0871237367F,   -0.00408290653F,  -0.00173657958F,  0.034358006F,
      -0.0104427114F,   -0.0204405095F,   -0.00108559243F,  -0.049799867F,
      -0.00591596F,     -0.00777986553F,  -0.00933314208F,  0.025457941F,
      0.0534693785F,    -0.113824338F,    0.0357518345F,    -0.0173416212F,
      -0.0575407F,      -0.0478240065F,   -0.000619981613F, -0.00128848723F,
      -0.00187278574F,  -0.0154074039F,   0.0839410275F,    -0.00201888289F,
      0.0108928969F,    0.019866474F,     0.0716479048F,    -0.00765158283F,
      -0.00129133381F,  -0.0050023F,      0.0493774377F,    -0.00130094518F,
      0.0153404642F,    0.0348178446F,    0.163601682F,     0.0186490174F,
      0.0537384488F,    -0.0201202966F,   0.0465608165F,    0.106832F,
      0.0261063129F,    -0.00194868608F,  0.0602603182F,    0.0562144779F,
      0.103179693F,     0.0230951216F,    0.00443345495F,   -0.098668173F,
      0.0250441786F,    0.0388674475F,    -0.00899177883F,  -0.02125814F,
      0.0757261515F,    0.0289573763F,    0.0402329527F,    -0.0158817787F,
      0.00831135735F,   0.011831807F,     0.0852353871F,    0.0206059068F,
      -0.0335382074F,   -0.0147843519F,   0.00496988557F,   0.00758497696F,
      -0.00210844958F,  -0.00253903121F,  -0.000208573096F, -0.0539728254F,
      0.000160953554F,  0.0123094562F,    0.0403226F,       -0.0276894756F,
      0.0989974663F,    -0.0546126068F,   -0.00844595488F,  -0.0532852449F,
      -0.00128251838F,  -0.000909183407F, -0.001765389F,    -0.00313157681F,
      0.171425149F,     0.0147167407F,    -0.00592220342F,  -0.00697324425F,
      -0.00544762F,     -0.013354918F,    0.0164084751F,    -0.00362881296F,
      0.0533988029F,    -0.0015523514F,   -0.00128838F,     -0.00307122106F,
      0.0327852517F,    -0.00129664189F,  -0.00437377812F,  0.0744256899F,
      -0.0222927723F,   0.0408589318F,    -0.0184149072F,   0.0475051962F,
      0.0166320279F,    0.0454322435F,    -0.0234557353F,   -0.00726727443F,
      0.00133794604F,   0.0101984572F,    -0.00405565649F,  -0.0975389257F,
      0.0311634243F,    -0.0273725335F,   -0.0639165491F,   -0.0392880328F,
      -0.0390160568F,   -0.0219234154F,   -0.00506899692F,  0.0194818843F,
      -0.006276567F,    0.102776393F,     -0.00594544597F,  0.01019766F,
      0.0107346429F,    -0.0283075F,      0.0272610355F,    0.00811568648F,
      -0.0288769305F,   -0.133925542F,    -0.00932875555F,  0.022728499F,
      0.0495456755F,    -0.00829101074F,  0.0247833617F,    -0.0393680297F,
      0.0241924915F,    0.00597781595F,   -0.0637790114F,   -0.0112081748F,
      -0.0433425978F,   -0.0290499646F,   -0.00917339232F,  0.0623163506F,
      0.0140320277F,    0.026519537F,     -0.0115691032F,   0.0153951384F,
      0.0445274226F,    -0.0573538542F,   -0.00900530815F,  -0.0262234025F,
      -0.00753461476F,  -0.00953690149F,  0.0214206558F,    0.00154714554F,
      -0.0203068424F,   0.0124172391F,    -0.00989702344F,  0.00110173225F,
      -0.0104201781F,   -0.00641221879F,  -0.0287296157F,   0.00274074636F,
      -0.0283108708F,   0.00911552366F,   -0.0229865629F,   0.0317931436F,
      -0.00860681944F,  -0.0532010607F,   -0.0135800466F,   0.0161096919F,
      -0.0140129449F,   -0.010634345F,    0.0122489519F,    -0.0100681679F,
      0.0375628062F,    0.00971571635F,   -0.0423814617F,   0.00555490842F,
      -0.0272579044F,   0.00401311042F,   -0.0200987756F,   -0.0365078412F,
      -0.0331012234F,   0.00861770753F,   -0.00134792016F,  -0.0100050876F,
      -0.0108956899F,   0.00718032289F,   -0.0619654693F,   -0.00770228589F,
      0.00588331744F,   0.0211823732F,    -0.0204739701F,   -0.00843830872F,
      0.0265243948F,    -0.0562994257F,   -0.0100176865F,   0.0193365347F,
      0.0129819624F,    -0.0489880629F,   -0.0407907665F,   -0.0105518419F,
      0.0212497842F,    0.0021981441F,    -0.03379016F,     -0.00869807508F,
      -0.083085373F,    0.0339897797F,    -0.0164411701F,   -0.024920078F,
      0.00278598F,      0.045833949F,     -0.0374704786F,   -0.0222170856F,
      0.000330847775F,  -0.00952580664F,  -0.0146491565F,   0.0410982035F,
      -0.00942275766F,  -0.0252060592F,   0.00634077657F,   -0.0166240148F,
      -0.00820727181F,  -0.0104340138F,   -0.0566029213F,   0.0573016591F,
      0.0711948127F,    0.00289416965F,   0.0287663694F,    0.0770852789F,
      0.0409103148F,    0.00183079822F,   -0.00689143827F,  -0.00776658393F,
      -0.00818876829F,  0.0234343335F,    -0.0304421F,      -0.0219384823F,
      -0.0100024706F,   -0.0104994345F,   -0.018046869F,    -0.0177305844F,
      -0.0443203412F,   0.0100970203F,    0.0243433919F,    -0.0125325043F,
      0.00593960518F,   -0.00854167249F,  0.0110948933F,    -0.0110646849F,
      0.00717335055F,   -0.0444000587F,   0.0399871878F,    -0.000281837885F,
      -0.0327904671F,   -0.00349354418F,  -0.0169170145F,   -0.0492513105F,
      0.0384470113F,    -0.0355682522F,   -0.0513380095F,   0.049301751F,
      0.0124234864F,    -0.00115889823F,  0.0103381379F,    -0.00909674075F,
      0.0245193057F,    -0.0157582499F,   -0.0280983262F,   0.00685818586F,
      -0.013791535F,    0.0176041424F,    -0.0154694943F,   0.00189549883F,
      0.0690215081F,    -0.0113386214F,   -0.0100542977F,   -0.00975459442F,
      0.00902961846F,   -0.0368471667F,   -9.78482694E-6F,  -0.0751678497F,
      0.0808041394F,    -0.0107695963F,   0.0139331091F,    0.0629705116F,
      -0.021434078F,    0.024171466F,     0.009179716F,     -0.0271548405F,
      -0.00882043596F,  0.10733483F,      -0.0342138857F,   0.00801479723F,
      -0.00526848854F,  -0.0308125522F,   -0.0216487702F,   0.0182145F,
      -0.031004753F,    -0.0107821058F,   0.00484856823F,   -0.0310860928F,
      0.0297413785F,    0.0100195101F,    -0.0179340411F,   -0.00579411071F,
      -0.0591498464F,   0.00719102F,      0.01616171F,      0.0213309601F,
      -0.00221186271F,  -0.0151916742F,   -0.0127959102F,   -0.0825078487F,
      -0.0564374961F,   -0.0105190855F,   -0.0271570124F,   0.115527011F,
      0.0170003492F,    -0.028869709F,    -0.00771702407F,  -0.00816594809F,
      0.0607834719F,    -0.0474133156F,   0.0241863318F,    -0.0341028869F,
      0.0624477826F,    0.0294824876F,    -0.00644814F,     0.00731728133F,
      -0.0485155322F,   0.0543641F,       -0.0106657837F,   -0.00854472537F,
      -0.0353916F,      -0.118961364F,    -0.0233871881F,   -0.0230411254F,
      -0.00906670839F,  0.0137941623F,    0.0159291066F,    -0.0201004446F,
      -0.0135370968F,   -0.0225979704F,   0.00325939292F,   0.0282180645F,
      0.0500338227F,    -0.0194347221F,   -0.00588818593F,  -0.0141786011F,
      -0.014875168F,    0.0249941275F,    -0.00489065889F,  -0.0279823449F,
      0.0810302645F,    -0.00849893689F,  0.0174962804F,    -0.0169789158F,
      -0.0575963482F,   -0.066181235F,    0.0554747954F,    0.0141629297F,
      -0.0170199014F,   -0.0254737865F,   -2.15807195E-5F,  -0.0103351269F,
      -0.0217576306F,   0.0113802711F,    -0.0370433256F,   -0.00920802F,
      -0.00871865824F,  -0.0711841583F,   0.0461253151F,    -0.028935289F,
      -0.00353479129F,  0.0181380678F,    -0.0106215272F,   -0.028111022F,
      -0.00749229034F,  0.00122128194F,   -0.0344889127F,   0.0121289026F,
      -0.0047291317F,   -0.0303923152F,   0.012024397F,     -0.0349322073F,
      -0.00601658132F,  -0.00418366212F,  -0.0376555361F,   -0.00430478854F,
      -0.00942463707F,  -0.0328148268F,   -0.054605905F,    -0.042539306F,
      -0.00658822479F,  -0.0282963254F,   0.0119814938F,    0.0127309384F,
      0.00193207583F,   0.0126223387F,    0.0172443371F,    0.0333592519F,
      -0.00452473154F,  -0.0126826847F,   -0.0192842465F,   -0.00527266F,
      0.00257498818F,   0.0113514448F,    0.0128612304F,    -0.00940187182F,
      -0.0188971851F,   -0.0205251556F,   -0.00628173677F,  -0.0193276F,
      0.00119685626F,   0.0386700705F,    -0.00448342226F,  0.0214031506F,
      0.0144488579F,    -0.0217231512F,   0.0130189201F,    -0.00788075849F,
      -0.0250014551F,   -0.0101436423F,   0.0181320533F,    -0.00507428264F,
      -0.0300505236F,   -0.0481297597F,   0.0334999599F,    -0.0217343867F,
      -0.00778858177F,  -0.0158458911F,   0.0182139743F,    -0.00638542231F,
      -0.066762045F,    -0.0429260843F,   0.024747422F,     -0.0294325743F,
      0.0367734805F,    -0.029902203F,    -0.00454879459F,  0.00147082133F,
      -0.00803617481F,  -0.088516213F,    -0.000658191799F, -0.000605596113F,
      0.0480240881F,    0.0188802127F,    -0.0115462504F,   -0.0275351517F,
      0.00610940298F,   0.0139485467F,    -0.0310308542F,   -0.00292316498F,
      0.0527096875F,    0.0334701911F,    0.0119615616F,    -0.0103574013F,
      0.0102620125F,    0.0264285579F,    -0.00587664871F,  0.0190599859F,
      -0.0669157207F,   -0.0214527F,      -0.0226543639F,   0.0130819455F,
      -0.00974074192F,  -0.00880085F,     -0.0234066173F,   -0.0165743921F,
      0.00885987468F,   -0.0558967888F,   0.00511628948F,   0.0238897726F,
      0.00554517F,      -0.0378694534F,   0.0766308233F,    0.0397011638F,
      0.0155686559F,    -0.00703341421F,  -0.00981003698F,  -0.0111382669F,
      -0.00126180437F,  0.00415448193F,   -0.012428347F,    -0.00483147381F,
      -0.0167075451F,   -0.00925983768F,  -0.0466434807F,   -0.00825073849F,
      0.0252941791F,    -0.00967581198F,  -0.00841208F,     -0.0140585853F,
      -0.010195367F,    -0.015669208F,    -0.0097972434F,   0.00645353645F,
      -0.0156124141F,   0.0343501903F,    0.014318943F,     -0.0392871313F,
      0.0157704279F,    -0.0382945426F,   -0.0136974938F,   -0.060121987F,
      -0.00140025711F,  0.00661256583F,   -0.0113543402F,   0.00305811921F,
      -0.00922095124F,  0.0367320292F,    0.0209095851F,    0.00441699941F,
      -0.00503334729F,  -0.0199458059F,   0.0304128453F,    -0.0158197377F,
      2.56150597E-5F,   0.0268623643F,    1.48379568E-5F,   0.000902276195F,
      0.0709619746F,    -0.0170499384F,   -0.00230933516F,  -0.0149866836F,
      -0.00205297652F,  0.0629917234F,    -0.0255185384F,   0.00227710349F,
      0.0375279F,       2.41672678E-5F,   -0.00187997217F,  0.0138014648F,
      0.0013459248F,    -0.000829339493F, -0.0173542351F,   0.0659461617F,
      -0.0425290391F,   0.000917888654F,  0.00165151036F,   -0.023235511F,
      -0.019782247F,    0.0037292249F,    9.64917199E-5F,   0.0228276607F,
      0.0105787935F,    0.00950662512F,   0.0276393909F,    -0.0191652626F,
      3.12894554E-5F,   3.81532809E-5F,   -0.0455908962F,   -0.0179217942F,
      0.0340310596F,    -0.00141925598F,  0.00383092975F,   -0.0181895439F,
      0.00341328862F,   -0.0875485688F,   -0.0481382497F,   0.0337519422F,
      0.0255521908F,    -0.0514374673F,   -0.000469087914F, 0.019105345F,
      -0.00010393461F,  0.0482905544F,    -0.000568860327F, -0.00394775392F,
      0.00135250832F,   -0.00154879328F,  -0.000965173589F, 4.52188669E-6F,
      0.0459278263F,    0.0162536427F,    0.0120572941F,    -0.0179130863F,
      0.000906641188F,  0.0570425019F,    -0.00216182205F,  0.133576632F,
      -0.01012538F,     0.045324482F,     -0.0206537955F,   -7.4354386E-5F,
      -0.0167912263F,   0.0455110669F,    0.00747224083F,   1.55171329E-5F,
      0.041151505F,     0.0410247892F,    -0.0392811336F,   -8.46469629E-6F,
      0.0230137333F,    0.000892925367F,  -0.0102738906F,   0.00173239585F,
      -0.00145593926F,  0.0149397086F,    -0.0360289253F,   1.11804393E-5F,
      0.053124316F,     -0.00678071566F,  -0.084990412F,    2.35977659E-5F,
      0.0190591495F,    0.0288674664F,    -0.0194924232F,   -0.00921005942F,
      -0.00114747952F,  0.0189118162F,    0.00437884638F,   -0.0034732169F,
      -0.0109879309F,   -0.0248486102F,   -0.000917371246F, -0.0318839699F,
      0.000249909499F,  -0.0209735055F,   -0.00901185535F,  0.0190297775F,
      -0.00230160914F,  -0.000647098466F, 0.0171630625F,    -0.00590620283F,
      0.000123363658F,  -0.00410049828F,  -0.0236722138F,   0.0431869179F,
      -0.0443415754F,   -0.0503056571F,   7.31222553E-6F,   0.000171087086F,
      -0.000243300747F, 0.0173275489F,    -0.00167825108F,  0.00110085553F,
      -3.89841352E-6F,  2.39969322E-5F,   0.00582427159F,   -0.00133010512F,
      0.00377520174F,   0.0592311062F,    -0.00185450702F,  -0.0015280511F,
      0.0502355918F,    -0.000105101331F, -0.034689825F,    0.00927379634F,
      0.00953290425F,   -0.0499525853F,   0.0336838923F,    0.0336004309F,
      -0.0121718151F,   -0.00392298633F,  -0.00270243804F,  -0.0148959188F,
      0.00214157579F,   0.0236298535F,    0.0152786039F,    -0.0273004901F,
      -0.0571325235F,   -0.00148043316F,  0.0103012798F,    -8.1996739E-5F,
      -0.0253898203F,   0.00479629403F,   -0.00177751598F,  0.0512840897F,
      9.35591816E-5F,   -0.0069653336F,   -1.48293757E-5F,  0.00592407025F,
      -0.0102216508F,   -6.23707092E-6F,  -7.03260885E-5F,  -3.06754409E-5F,
      0.0139862075F,    0.0100245168F,    0.00427846052F,   0.0560464412F,
      -0.018598279F,    0.0555395372F,    -0.0032858171F,   0.041087687F,
      0.00239799F,      0.0180776883F,    0.0253087301F,    -0.0393659845F,
      0.0131668597F,    -0.0314026475F,   0.000299544161F,  0.00038690868F,
      0.0012863389F,    0.00526041491F,   0.00660000462F,   -0.0433290787F,
      -0.00184834434F,  -0.0147972442F,   0.0276789814F,    -0.046267394F,
      -0.00300763547F,  -0.00747380918F,  0.0178693868F,    -0.0513369106F,
      -0.0171488971F,   -0.000482875272F, 0.0124212271F,    0.00113260304F,
      -0.0189440437F,   -0.0114956796F,   -7.58868846E-5F,  0.00293438439F,
      2.41960042E-5F,   -1.453201E-5F,    -0.0201415308F,   0.0302154049F,
      -0.0161639731F,   0.00340952422F,   -9.51585571E-6F,  -1.7950988E-5F,
      0.0164113324F,    -0.0149687389F,   0.0254097078F,    -0.000999248587F,
      -0.06792178F,     0.0113164969F,    0.00587866595F,   0.0354857147F,
      -0.0140215363F,   0.0400571339F,    3.05843714E-5F,   -0.0189771373F,
      -0.0213596132F,   0.0616555773F,    -0.041547671F,    -0.0329392329F,
      -0.00865846779F,  0.0499835536F,    0.127388015F,     0.000155783375F,
      0.0101623368F,    0.00444620242F,   -0.027420558F,    0.00303560845F,
      0.0262018051F,    -0.00902110524F,  -0.0148952911F,   -0.000345919107F,
      -0.0223572012F,   0.0351824462F,    -0.0018685722F,   -0.0474859364F,
      0.0106536709F,    0.000110685753F,  0.0103681972F,    -0.0153932832F,
      0.0285292398F,    0.00198916811F,   -0.00383114256F,  0.00253988174F,
      -0.00568501139F,  -0.00193599914F,  -0.00160542713F,  -5.30442958E-5F,
      -0.0432616584F,   -0.0416002572F,   -0.0451868027F,   -2.88743086E-5F,
      -0.00965146255F,  0.00246810517F,   -0.0586545244F,   0.00379926572F,
      -0.00230379F,     0.0217795763F,    0.020419253F,     -0.00361607852F,
      -1.91807558E-5F,  -0.0138297807F,   0.011895298F,     0.020192517F,
      0.0622759648F,    -0.0527633354F,   0.0137765165F,    -0.0507362969F,
      2.02151787E-5F,   -0.0180200022F,   0.0099460762F,    0.00129704142F,
      0.000121961995F,  -0.0102311596F,   -0.00327790854F,  -0.00614775345F,
      7.73532793E-6F,   -0.0399716198F,   0.00357241114F,   -0.00303616375F,
      0.00992607139F,   0.0238331538F,    0.071389772F,     -0.0411728323F,
      0.0106070871F,    -0.00155769952F,  0.000364731415F,  0.00396608468F,
      0.0349264704F,    0.0302193891F,    0.0292348173F,    -0.0353335589F,
      -0.00273206783F,  -0.00301336171F,  0.00415890105F,   0.00366505678F,
      -0.00102961611F,  -0.0841999F,      -0.0537232906F,   0.00864897203F,
      -0.0210657492F,   -0.030188011F,    0.0228485502F,    4.91587525E-6F,
      -0.00424308935F,  -0.00145412947F,  0.0299509559F,    0.000167357342F,
      -0.00197061268F,  -0.0333239473F,   0.0340212584F,    -0.0032976279F,
      -7.83425203E-5F,  0.00226792041F,   -0.00839353725F,  4.15888971E-5F,
      0.0116770109F,    0.020810876F,     0.0323670171F,    -0.029765984F,
      -0.00239138678F,  -0.0142556764F,   0.0472346507F,    0.0539720729F,
      0.00183361524F,   0.0937133133F,    -0.030000288F,    0.00155663083F,
      0.00896620285F,   -0.0275390409F,   0.0337558836F,    -0.0215127151F,
      -0.0291665774F,   0.0354518928F,    -0.0156729F,      0.00132912584F,
      -0.0662753358F,   0.00570930867F,   0.00867140107F,   0.00173700775F,
      -0.0546759628F,   -0.00227589603F,  0.0471561775F,    -0.0285627861F,
      0.00297576701F,   0.000963415834F,  -0.0016506355F,   0.0176225789F,
      -1.27486792E-5F,  0.000208966885F,  0.00198313221F,   -0.00425575394F,
      0.00458215177F,   0.0383296199F,    0.0610946454F,    0.0070612547F,
      0.0326081365F,    -0.0255172066F,   -0.0175915398F,   -0.00390496221F,
      0.00983365066F,   -3.02965855E-5F,  -4.46496524E-5F,  4.58599607E-5F,
      0.0581062287F,    0.0316390134F,    9.12741143E-6F,   -0.0269519612F,
      -0.00026497044F,  0.0789519697F,    0.0119877374F,    -3.69261397E-6F,
      0.124953069F,     -2.71129175E-5F,  2.80642798E-5F,   -0.000279291038F,
      -0.00942626223F,  -0.000850982964F, 6.35804281E-6F,   -0.00480838679F,
      0.00407094136F,   0.0102279112F,    0.000612838776F,  -0.025860453F,
      0.0537767559F,    0.0112608336F,    0.00797417574F,   -0.025133552F,
      -0.0482092425F,   0.0122603457F,    -0.000670433685F, 0.00433018757F,
      -0.00560689857F,  0.0307411794F,    -0.00529361749F,  0.0246118419F,
      0.00717390748F,   -0.0200502239F,   -0.00771401078F,  0.0187316537F,
      -0.0062318393F,   -0.0357130393F,   -0.00574348075F,  -0.00547438813F,
      -0.0285270493F,   0.0133462353F,    0.0109856203F,    -0.00771708973F,
      -0.0278962646F,   -0.0556492098F,   -0.0126486048F,   -0.0288003609F,
      0.00088269F,      -0.00808373932F,  0.0138148014F,    -0.0395192057F,
      -0.0607830882F,   0.0216190834F,    -0.00708135124F,  -0.016153533F,
      0.0446049199F,    -0.0282095727F,   -0.00120107201F,  -0.0231805686F,
      -0.00017691929F,  -0.0544404797F,   -0.0111268098F,   0.00885707419F,
      -0.0397710055F,   -0.05882385F,     -0.0350631587F,   0.0275595952F,
      -0.00733120879F,  -0.0092581762F,   -0.0522061288F,   0.00489043444F,
      -0.0369384736F,   -0.0339749902F,   -0.0409853645F,   -0.0186262485F,
      0.0300629269F,    -0.00770219648F,  0.0664250851F,    -0.00799704809F,
      -0.0320677608F,   -0.00352215092F,  -0.00700242771F,  -0.00719992025F,
      -0.00843956787F,  0.0592598878F,    -0.013699363F,    -0.032839004F,
      -0.0138980085F,   -0.0282559115F,   0.00304872799F,   -0.0115039237F,
      0.00457200175F,   -0.00893436279F,  0.00147190026F,   -0.0045781252F,
      -0.02436045F,     -0.00251650531F,  -0.0367656201F,   -0.0513035096F,
      -0.0318193808F,   0.0312064085F,    -0.0346979462F,   -0.0109178731F,
      -0.0132610574F,   -0.0108438348F,   0.0272642951F,    -0.00760443322F,
      -0.0103412243F,   0.0391573571F,    0.0713195726F,    -0.00826421566F,
      0.0327335782F,    0.0319621935F,    -0.00123174756F,  -0.044291053F,
      -0.0380823389F,   -0.00798741821F,  0.115942985F,     -0.0105239181F,
      0.0331765935F,    -0.0355131924F,   0.0619894899F,    -0.00873350352F,
      -0.0378015228F,   0.00410274649F,   -0.0238186587F,   -0.0250653811F,
      -0.0489375032F,   0.00377150718F,   -0.0940046459F,   -0.0367167555F,
      0.0039949487F,    -0.0235004742F,   -0.0217793323F,   0.0498660132F,
      -0.0113417925F,   -0.00150737667F,  -0.0373770222F,   0.0120726405F,
      -0.0812287852F,   -0.0106441742F,   0.0529415756F,    -0.0297365654F,
      -0.046601098F,    -0.0104483748F,   -0.071615085F,    -0.00358015904F,
      -0.0144737298F,   -0.06018278F,     -0.00674917968F,  -0.0077018328F,
      -0.0312670618F,   -0.0133281201F,   -0.0478138365F,   -0.00749705639F,
      -0.00988253F,     -0.0101423739F,   -0.0630761608F,   -0.0159255173F,
      -0.0668077F,      0.0447853059F,    -0.0426254F,      -0.0322438851F,
      0.0279799663F,    -0.00808470696F,  0.0689765364F,    0.0132492362F,
      -0.0117880534F,   0.0561839975F,    -0.0165142864F,   0.0302612484F,
      -0.026703706F,    -0.0286976658F,   -0.0363600738F,   0.0531015098F,
      -0.0314430073F,   -0.012494619F,    0.0108842989F,    -0.003915F,
      0.0105418572F,    -0.0467276312F,   -0.00168000115F,  -0.00948975701F,
      0.0148913255F,    -0.0349437781F,   -0.0345480815F,   -0.00905223F,
      -0.0140190944F,   -0.011702395F,    -0.0253400337F,   0.000470761734F,
      -0.028023202F,    -0.0116482805F,   -0.00983089767F,  -0.00960000884F,
      0.0106553603F,    0.0317089148F,    -0.00572702056F,  0.0736556724F,
      -0.03032979F,     0.0341678448F,    -0.052645158F,    -0.00122173084F,
      -0.0126205441F,   0.00501484144F,   0.0118619734F,    0.0333091095F,
      -0.00717527419F,  -0.0587735325F,   -0.028307613F,    -0.0332906768F,
      -0.03308044F,     0.00939718075F,   -0.00814404245F,  0.020168731F,
      -0.0490353703F,   -0.0305759218F,   0.00440291176F,   0.0390772298F,
      -0.0497349799F,   -0.013399573F,    -0.00210712501F,  0.0391260497F,
      -0.0121949222F,   -0.0374400392F,   0.00667709578F,   -0.0482878201F,
      -0.00357648148F,  0.0630164221F,    -0.0126906158F,   -0.0219722539F,
      0.00403017784F,   -0.0105818389F,   -0.016096022F,    0.0275570303F,
      0.00205342914F,   0.0268478226F,    -0.00774207758F,  -0.00817798078F,
      -0.00388732529F,  -0.0388730913F,   0.00412518531F,   0.00253406074F,
      -0.0309450123F,   -0.0355510637F,   -0.00389453582F,  0.0103352638F,
      0.0566706099F,    0.0034254645F,    -0.0106790513F,   -0.0313507542F,
      -0.000125321763F, -0.00829311833F,  0.0329030193F,    -0.0170280207F,
      0.0071714567F,    -0.0299166292F,   -0.0153938197F,   -0.0339772068F,
      0.0279216319F,    -0.00844731F,     0.00421984727F,   -0.0398477241F,
      -0.00420645298F,  -0.0266349576F,   0.0377885178F,    -0.0177529063F,
      -0.00593080232F,  0.00203218125F,   -0.0750116631F,   0.0570460595F,
      -0.0220620893F,   -0.008336097F,    0.00367104146F,   -0.0139197074F,
      0.049220413F,     0.00501138251F,   0.0548327714F,    0.013021417F,
      -0.029163124F,    -0.0272397064F,   -0.00384467584F,  -0.0103800753F,
      0.0364386477F,    0.00965568516F,   0.0849544555F,    -0.0091971159F,
      0.0225499123F,    0.00977835059F,   -0.011230059F,    0.024032034F,
      -0.000557830557F, 0.006787939F,     -0.0169212036F,   -0.0302373469F,
      -0.0073574977F,   0.00183938362F,   0.0267850123F,    0.0146743497F,
      -0.0103343753F,   -0.00160557579F,  0.0117234867F,    0.0551893041F,
      -0.00595267024F,  -0.00192447193F,  0.00324404845F,   0.00118078F,
      -0.00958429463F,  -0.00943409838F,  0.00627600634F,   0.0649478734F,
      -0.00658918312F,  -0.00727240928F,  -0.0292203259F,   -0.00121375034F,
      0.0121603468F,    0.0171546638F,    -0.067914322F,    -0.0194232445F,
      0.0243947972F,    -0.0130632836F,   -0.0335535258F,   0.00715011F,
      -0.0108728614F,   -0.0181135554F,   -0.00365697173F,  -0.000531366F,
      -0.0181805361F,   -0.0206128266F,   -0.000510226178F, 0.0243414212F,
      -0.011581853F,    -0.0402299799F,   -0.00264144945F,  -0.02425991F,
      0.0134309866F,    -0.00170592661F,  0.033539746F,     -0.00779508194F,
      -0.0230023284F,   -0.0434312113F,   -0.0236094091F,   -0.00504304282F,
      -0.0419182852F,   -0.012306463F,    -0.0165353492F,   -0.0387402922F,
      -0.007708936F,    -0.0125234006F,   0.0268342439F,    -0.00636746408F,
      -0.0153734535F,   0.00408118451F,   -0.0210432801F,   0.0357750282F,
      0.0396066904F,    -0.0193430334F,   -0.00432342151F,  -0.0331865847F,
      -0.0167855453F,   0.0535690673F,    -0.0100949667F,   -0.00378415012F,
      -0.015158684F,    0.005531976F,     -0.0180341136F,   0.0482169241F,
      -0.00467946939F,  -0.00927979313F,  0.0126072122F,    -0.0106186932F,
      -0.111773841F,    0.0277580973F,    -0.0180096719F,   -0.013940556F,
      0.00942958426F,   0.0272374433F,    -0.00828523096F,  0.0206717849F,
      -0.0791594312F,   -0.0487848818F,   -0.0342774764F,   -0.00198816066F,
      -0.0103528257F,   -0.00856939703F,  -0.0353162363F,   -0.0218529478F,
      0.0216295514F,    0.0179675948F,    0.0156591218F,    -0.043010097F,
      -0.0483762473F,   -0.00413751276F,  -0.0176995769F,   -0.0300865509F,
      -0.0259361F,      -0.00701270765F,  -0.00955765322F,  -0.0107192257F,
      -0.0037715747F,   7.26962098E-5F,   -0.0127659133F,   -0.00264722365F,
      -0.0196341909F,   -0.0143031655F,   0.00121846702F,   -0.00823098328F,
      0.0353800617F,    -0.00947012194F,  -0.00830474868F,  -0.0137861287F,
      0.0153320059F,    -0.0151220551F,   -0.00975943636F,  -0.0300769098F,
      -0.0103086196F,   -0.0403247885F,   -0.0330124386F,   0.0882596821F,
      0.00917501654F,   0.0258501433F,    -0.00581257045F,  0.0647055283F,
      0.0189303234F,    0.00881302543F,   -0.011624625F,    0.0224221703F,
      0.0124327959F,    0.00386580778F,   0.0342976488F,    0.00888596103F};
  static const float inputStateWeights[2000] = {
      0.00314874668F,   -0.00787014514F,  -0.0160688609F,   0.00106169598F,
      5.37013148E-6F,   -0.0265336763F,   -5.25349533E-5F,  0.00105747813F,
      -1.23396134F,     -0.000590690237F, 0.00721616205F,   0.0247722119F,
      -0.00754664233F,  0.00530783553F,   -0.864171326F,    -0.00265270891F,
      0.00207279762F,   -0.000348971429F, 0.00247723935F,   -0.180901244F,
      -0.00439329306F,  -0.000570102478F, -0.0236441698F,   -0.0304217655F,
      -0.00694960402F,  -0.0126075419F,   0.0089141177F,    0.0284314398F,
      -0.00197980553F,  0.00980940741F,   0.000652824063F,  0.0037297185F,
      0.00212483131F,   0.000400989229F,  -0.0943855271F,   -0.018766759F,
      0.000149704108F,  0.000521104608F,  -0.000714612659F, -0.00325565482F,
      0.565664172F,     0.020244075F,     0.00936606713F,   -0.0170683619F,
      1.97718322F,      -0.0200508889F,   -0.00161764747F,  -2.35757089F,
      -0.0318946801F,   -2.05218577F,     0.000448196311F,  -0.00366514781F,
      -0.00050969352F,  -0.116347715F,    -0.00135769241F,  -0.00482169865F,
      -0.00290118367F,  0.00195220544F,   0.00347913895F,   -5.20110916E-5F,
      -0.00330490619F,  -0.0154373208F,   0.0187084358F,    0.0100479703F,
      -0.00511280028F,  -1.89655399F,     0.000470474275F,  -0.611541033F,
      0.0274785124F,    0.0406679921F,    -0.00274624606F,  0.000214247222F,
      -0.000974437513F, 1.7471261F,       -1.0899725F,      0.000122972808F,
      -1.6185627F,      -0.00392514421F,  0.0552157871F,    0.00014054068F,
      1.80896962F,      -0.0234160833F,   -0.0640470535F,   0.00219722139F,
      0.00557921687F,   0.0142233353F,    0.0532596074F,    0.000301863591F,
      -2.26187849F,     0.00049020088F,   -0.044443924F,    0.000355279655F,
      0.118296884F,     -0.00222709705F,  0.00853458792F,   -0.00200375682F,
      -0.0052079251F,   -0.265616834F,    -0.0952408314F,   0.00150596723F,
      -0.0081057176F,   0.00441052858F,   0.0013456014F,    -1.19141126F,
      0.00453175278F,   0.003145186F,     0.000855901802F,  -0.934025109F,
      -0.019014325F,    9.18011938E-5F,   0.0107302554F,    0.0193497166F,
      -0.00193090446F,  -2.15317249F,     0.0198837165F,    2.08740091F,
      0.000972989772F,  -1.25609946F,     -1.33202811E-5F,  -0.000187051846F,
      0.00738360081F,   0.00291070645F,   0.0177826956F,    0.00103478448F,
      0.000336823083F,  -0.000176062531F, -6.14874589E-5F,  -0.000906702771F,
      -0.0129935779F,   0.0448151305F,    -0.00466724811F,  -0.0098829316F,
      -1.97204542F,     -0.000439994445F, -0.0143369017F,   -0.00308070611F,
      0.00814661197F,   0.132519856F,     -0.00290464959F,  -0.404010803F,
      -0.66887027F,     -0.00784713F,     -0.000674243493F, 1.14527154F,
      -0.0117503088F,   -0.0031420372F,   0.00383420289F,   -0.00515279919F,
      -0.00184909184F,  -0.00534129282F,  0.00186458556F,   -0.000459458068F,
      0.0821182504F,    -0.0084285466F,   0.0136455595F,    -2.06126428F,
      0.000630363065F,  8.62760426E-5F,   -0.000916643301F, 2.24856782F,
      0.00415883632F,   0.000330363255F,  -0.000736651069F, 0.000334648241F,
      0.0042103515F,    5.65930823E-5F,   -2.03038263F,     -0.0451668687F,
      0.00162343227F,   -0.0303141847F,   0.00107299839F,   -0.0012400276F,
      0.00322519662F,   0.00168985093F,   0.00938133523F,   -0.361380488F,
      2.098773F,        -0.0184944365F,   -0.00943743158F,  -0.00228319643F,
      -0.0129082557F,   0.0012409006F,    -0.00110732019F,  0.0023427105F,
      -0.00598101597F,  0.000703069847F,  0.010200656F,     0.000269045675F,
      0.00241020252F,   0.0883612633F,    -0.00612445921F,  -0.00102298253F,
      -0.0298504177F,   0.0122520123F,    0.00395228341F,   -0.00125538011F,
      -0.661157072F,    2.28032851F,      -0.000853225414F, -0.849594116F,
      0.00261282665F,   -0.000147619605F, 0.00666978909F,   -0.00169915939F,
      0.00452597626F,   0.00139989262F,   0.00011003259F,   -8.19046545E-5F,
      2.30491233F,      0.0184453689F,    -0.000628479F,    0.00101093517F,
      -0.547155261F,    0.00257795F,      2.20300269F,      -0.00171728036F,
      -0.29382208F,     -0.00145565858F,  0.000385991851F,  0.0248869415F,
      0.00469830679F,   1.49312735F,      0.00969563052F,   0.560694814F,
      0.0258789044F,    2.00685596F,      -0.0109965019F,   -0.00459041F,
      2.0884223F,       -0.00871905778F,  -0.00105405319F,  0.00134589686F,
      -0.00239628251F,  -0.00194465008F,  -0.0163770337F,   0.00180788746F,
      -0.0107813338F,   0.00248084706F,   0.0171334278F,    7.22190598E-5F,
      -0.00219897251F,  0.000299782783F,  0.00304232491F,   0.00905538816F,
      0.0135661513F,    0.00725004543F,   -1.05954742F,     -0.00156907248F,
      0.00324954232F,   0.00845735241F,   0.0252398793F,    -0.00011273905F,
      0.00862967223F,   0.000722300378F,  -0.00484135933F,  0.00012051504F,
      -1.98108506F,     0.0255138632F,    -0.00151296647F,  0.210569724F,
      -2.21954751F,     -0.00333743822F,  -0.025880022F,    0.00567111559F,
      -0.000315748825F, 0.016167758F,     0.00421846379F,   0.0240398757F,
      2.24129963F,      0.000301912398F,  -0.000677884149F, 0.00363151799F,
      -0.000426125276F, 0.0151280509F,    0.0877668932F,    0.0151534127F,
      0.000702293066F,  0.00730168028F,   0.018601317F,     -0.0105317282F,
      -6.10272082E-6F,  0.0112039475F,    0.00641853828F,   -2.198524F,
      -0.00332006067F,  -0.00419089943F,  0.0707860216F,    -0.015182009F,
      -0.000407489977F, -0.000109790315F, -0.00430786423F,  -2.25603175F,
      -0.0128575973F,   1.52074504F,      -0.00262070214F,  0.341340542F,
      0.00250137085F,   -0.00373360515F,  0.00191095192F,   0.00461659255F,
      -2.13173938F,     0.0666869357F,    0.00355771324F,   -0.00113288581F,
      -0.0131852599F,   0.0851335675F,    0.00845393818F,   0.000270645774F,
      0.00878054835F,   -0.0117071364F,   1.8706882F,       -0.000286162627F,
      -0.00749951415F,  -0.00173526676F,  -1.86970961F,     0.00971619599F,
      -0.000277825282F, -0.00387782045F,  0.00297806924F,   -1.69801024E-6F,
      0.00868347939F,   -0.0470185131F,   -0.127653643F,    -0.0164096877F,
      0.00384706515F,   -0.0116116172F,   -2.37216926F,     1.72298658F,
      0.0112194512F,    -0.571332F,       1.35805857F,      -2.23222184F,
      -2.31718612F,     -0.00116825628F,  2.14065289F,      -0.0603947788F,
      -0.0180576947F,   -2.41457057F,     -0.00468166173F,  0.00622059777F,
      -0.055817049F,    -0.00359930145F,  2.02793336F,      0.00542130787F,
      -0.00598703744F,  0.000841203961F,  0.00497917132F,   -0.00340372277F,
      -0.016249001F,    0.028991973F,     0.00139618013F,   2.35152221F,
      0.000555252482F,  -0.000133993555F, -0.0048633134F,   0.00530644553F,
      -0.000219334135F, -0.0182277169F,   -1.65469587F,     0.00226948131F,
      -0.0296185985F,   -0.0113729415F,   -0.00365879107F,  0.0109182009F,
      0.012494334F,     -0.000147863131F, 8.38154083E-5F,   0.000304435816F,
      0.0517562032F,    -0.0025888849F,   -0.000480268296F, 0.00410074461F,
      -0.00159565872F,  -0.0569580831F,   -1.35143399F,     -0.000219413734F,
      2.2138288F,       -2.82401124E-5F,  -0.000328236085F, -0.00091946F,
      1.71955824F,      -0.00213139551F,  9.2703347E-5F,    -0.0032857398F,
      0.000755992543F,  -0.0132150501F,   -0.00317613804F,  0.0062529589F,
      -0.0236133244F,   -0.00102671026F,  -0.0026403307F,   -0.00750378473F,
      -0.00091003417F,  0.00337798265F,   0.000302495784F,  0.00178427773F,
      2.01879168F,      0.00327006285F,   -0.759178162F,    0.00357489754F,
      -0.00381655921F,  -0.00893094577F,  0.0154692261F,    -0.23356317F,
      -0.000169123567F, -0.00544636045F,  6.75563788E-5F,   -0.23691541F,
      0.000864642148F,  0.0206966437F,    -0.0285228025F,   -0.0193302948F,
      0.0177155063F,    0.292039394F,     0.000358055229F,  -0.240775466F,
      0.472415864F,     -9.43724372E-5F,  0.27207756F,      -0.000904396235F,
      -0.312694192F,    0.203077942F,     -0.00935666822F,  -0.00667019701F,
      0.0408457629F,    0.0225355476F,    -0.033748813F,    -0.00777953351F,
      -0.247596413F,    0.00433470681F,   -0.0002636227F,   0.33558926F,
      0.252325773F,     0.400243223F,     -0.0062199817F,   0.0270682704F,
      -0.000369563495F, 0.000231111364F,  0.0922597423F,    0.0599415712F,
      0.00106071495F,   -0.0193646364F,   -0.0186275821F,   0.0353815481F,
      0.000230730788F,  -0.0742417723F,   0.193724245F,     -8.87326678E-5F,
      -0.00475794449F,  -0.00179402402F,  0.00474830065F,   -0.346743673F,
      8.04468E-5F,      0.00445427047F,   0.00733521255F,   0.0771006718F,
      0.00803753454F,   -0.0336406119F,   0.0536745749F,    -0.00016724439F,
      -0.62286979F,     0.0226477142F,    -0.0350744389F,   0.00453044893F,
      0.0487474576F,    0.00172606215F,   0.0484898053F,    -0.00370464F,
      -0.0347283483F,   -0.0108105727F,   -0.27620396F,     -0.000464450801F,
      -0.00664520357F,  -0.00122941518F,  0.00153526373F,   5.04364725E-5F,
      0.00111592805F,   0.12124674F,      0.0219388586F,    -1.71683278E-5F,
      -0.00100745517F,  0.0192739293F,    0.0193309244F,    -0.00374710234F,
      -0.125252619F,    -0.0290058609F,   -0.0114607131F,   -0.000288153475F,
      0.00108892575F,   0.137946725F,     -0.00373323588F,  -0.000155904316F,
      -0.00423072046F,  -0.433949202F,    -0.0610083379F,   -0.245025367F,
      0.0519981571F,    -0.00192986848F,  0.00199754653F,   0.214552F,
      0.0352544039F,    0.28454572F,      -0.00973717F,     -0.00318251201F,
      -0.0397126563F,   -0.0446944796F,   -0.366369516F,    0.00796004292F,
      0.0428335965F,    0.00122456381F,   -0.0380760282F,   -0.0795356706F,
      -0.225847289F,    -0.000408564432F, -0.0165695176F,   0.00122393761F,
      0.218054935F,     0.000462346594F,  0.000203814605F,  -0.000165886144F,
      -0.0427345335F,   0.280971318F,     -0.0183905549F,   0.0069271517F,
      -9.40488317E-5F,  -0.000188976774F, -0.366076887F,    0.00657047238F,
      0.00524042267F,   -0.0137694608F,   -0.362454474F,    0.11293453F,
      0.000837288564F,  -1.2070087E-5F,   -0.278915793F,    -0.0416631103F,
      0.00466848677F,   -0.00680784974F,  -0.61334765F,     0.00211783755F,
      0.0184502825F,    0.0559945107F,    0.055399F,        0.00438668F,
      0.0453706682F,    -0.723795712F,    0.843705118F,     -0.233274639F,
      -0.280890554F,    0.0246865209F,    0.394439429F,     5.11934559E-5F,
      0.000858329528F,  -0.0075791725F,   -0.0129197994F,   0.000886485912F,
      9.56637377E-5F,   0.176858872F,     -0.000402158243F, -0.000233488798F,
      -0.154569596F,    -3.94108138E-5F,  7.46086662E-5F,   -0.000135366688F,
      0.298646808F,     -0.00736593921F,  -0.000707798055F, 0.0265713837F,
      -0.208270445F,    -0.0584057085F,   -0.105831593F,    -0.696201921F,
      0.00699091377F,   0.271099061F,     -0.0307991486F,   0.000882687804F,
      -0.000817586377F, -0.0255920719F,   0.027560113F,     0.0700276F,
      -0.00334488018F,  -0.0206137206F,   0.0418450311F,    0.201899201F,
      0.121838681F,     -0.20483835F,     0.0135532403F,    0.224712834F,
      -0.0541509055F,   -0.0216867495F,   -0.38239646F,     -0.275215447F,
      -0.0108626783F,   -0.0349869579F,   0.452906162F,     0.0105660623F,
      -0.00303142611F,  0.000520133F,     5.73302677E-5F,   0.000668372493F,
      0.704923749F,     9.83448263E-5F,   -0.0925686136F,   0.0560391694F,
      -0.0774831623F,   0.230283886F,     -0.000231842205F, 0.00021509784F,
      0.00183128694F,   0.00857338775F,   0.012016193F,     0.110252477F,
      -0.00338413566F,  0.261340082F,     -0.000228809775F, -0.0237688199F,
      0.00595274195F,   -0.551836312F,    6.62863167E-5F,   -0.0268080439F,
      -0.123874791F,    0.00165127323F,   -0.0671545416F,   0.00547763519F,
      -0.0185075905F,   -0.000740121701F, 0.00529649807F,   0.107087798F,
      -0.0011396507F,   0.00495788921F,   0.179709285F,     -0.137943313F,
      -0.316108763F,    0.169798642F,     -0.0802097544F,   0.113342926F,
      0.00680970075F,   0.440126061F,     -0.0129490308F,   0.199488163F,
      0.0545490496F,    -0.000400881137F, 0.333923459F,     -0.0870944932F,
      0.00553947734F,   0.00698705902F,   -0.00486489292F,  0.217657849F,
      0.298922122F,     -0.057348609F,    -0.0363166891F,   -2.29220823E-5F,
      -0.0741611347F,   0.167478353F,     -0.331225902F,    -2.86604973E-5F,
      4.64369514E-5F,   0.004873069F,     0.205814824F,     -0.00547283096F,
      0.000598929415F,  -0.321860284F,    0.00841201097F,   -0.0102878548F,
      -0.000128256332F, -0.0481821485F,   0.00700079463F,   -0.0360869765F,
      -0.000502204F,    0.169259265F,     -0.0791792497F,   -0.108235784F,
      -0.000153577086F, -0.0314448625F,   0.00571028283F,   0.0736708865F,
      0.000158556621F,  -0.0996549055F,   -0.0320827588F,   0.0892332792F,
      0.000154914611F,  0.00716051785F,   -0.111196727F,    0.00101557781F,
      -0.0796848908F,   -0.206619158F,    0.0044677509F,    0.00937444F,
      0.146141335F,     0.00254273228F,   0.00581218116F,   0.000443696714F,
      -0.0124219423F,   -0.00218953658F,  -0.387558311F,    -0.016614629F,
      -0.00498868152F,  0.0101265525F,    0.0130939102F,    0.344275147F,
      1.97588433E-5F,   0.00419318071F,   0.434338957F,     -0.191683963F,
      -0.0119603612F,   -0.006402181F,    -0.115166F,       -0.000202633892F,
      -0.0203646794F,   0.0601038486F,    -0.000200181821F, -7.22859331E-5F,
      0.00543207349F,   -0.211255506F,    -0.000269725686F, -0.0138152605F,
      -0.000505039119F, 0.0190531798F,    -0.0310895368F,   0.000281120447F,
      -0.053542953F,    -0.0138997035F,   0.0112753892F,    0.0150680356F,
      0.458059579F,     0.0332410149F,    0.000386727974F,  -0.00104909425F,
      -0.0336415805F,   0.000298837433F,  0.00469818711F,   0.00074477552F,
      -0.0013002078F,   0.0831075087F,    -0.000678756507F, 0.00964783132F,
      -0.00509319734F,  -9.2561073E-5F,   0.101890489F,     -0.0369965248F,
      0.0256403722F,    -0.316384554F,    0.000795295811F,  -0.0297608059F,
      -0.330662817F,    -0.13990733F,     -0.0177918915F,   -0.262164176F,
      0.0221537016F,    0.0209509563F,    -0.122528069F,    0.00343359937F,
      6.96575444E-5F,   -8.20618588E-5F,  -0.335979849F,    -0.108464018F,
      0.204122648F,     -0.0169068296F,   0.00130720297F,   -0.175090328F,
      0.0191006381F,    0.0623088703F,    -0.249306664F,    -0.0913918316F,
      -0.0224927906F,   0.000200999144F,  -0.00020956542F,  -0.000396768097F,
      -0.0194669366F,   -0.64970541F,     0.000280443172F,  -0.0503520407F,
      0.00403571734F,   -0.0071487003F,   0.00184423581F,   -2.31634094E-5F,
      -0.00041771424F,  0.000185203346F,  -0.000385811261F, 0.000321227475F,
      -0.000772911764F, 0.0131573128F,    -0.000200391398F, 0.0884129852F,
      0.0991241112F,    0.0435242914F,    0.0957473218F,    0.0485167354F,
      -0.0728614479F,   -0.00664073694F,  0.00697600702F,   0.0491638258F,
      0.192232311F,     0.257060111F,     -0.00227578334F,  0.123459667F,
      -0.000126005252F, 0.413479924F,     0.0197837129F,    0.0183744654F,
      0.000680345926F,  0.00230351347F,   -0.0100025842F,   0.00774193974F,
      0.000392046262F,  -0.00594484759F,  0.000284277747F,  0.00810131337F,
      -0.198676363F,    0.00362237031F,   0.00969484914F,   0.0163044222F,
      0.00237567187F,   -0.0116618518F,   0.0294740442F,    0.00794258434F,
      -0.0349789262F,   -0.000868049101F, -0.00490528811F,  0.0311872475F,
      -0.000640486192F, -0.0032825307F,   0.0848951936F,    -0.000949843379F,
      -0.0101017756F,   0.00205451902F,   0.00635144487F,   0.0447962582F,
      0.00480035227F,   0.0324775279F,    0.00168666F,      -0.0211865772F,
      -0.0012417495F,   -0.0215262249F,   0.00282091345F,   -0.00175741327F,
      -0.000418901443F, 0.000273008161F,  0.00986108091F,   -0.000193941742F,
      -0.0366522111F,   -0.0024935361F,   3.5455625E-5F,    -0.00944432802F,
      -0.0346461162F,   -0.0726346746F,   -0.0053151329F,   0.0540218F,
      0.0108549157F,    0.105655789F,     -0.00137287029F,  0.00265460392F,
      -0.00109697075F,  0.0241427477F,    -0.000717395102F, -0.00696078641F,
      -0.000253994396F, 0.00570952939F,   -0.0106065674F,   -0.000664773514F,
      0.0677406266F,    -0.00294974842F,  -0.00502753351F,  0.0096813F,
      -0.00268354663F,  -0.0160298571F,   -0.00415005162F,  -0.0925672874F,
      -0.000806729659F, 0.0409489162F,    0.00803135F,      -9.97148891E-5F,
      0.000109992317F,  0.0843617395F,    0.0761310086F,    0.000158074588F,
      -0.157226562F,    0.00963659212F,   0.0545576923F,    -0.000224657793F,
      0.109274276F,     0.0079282F,       -0.00472148089F,  0.00107064052F,
      0.00831806939F,   -0.000680703844F, 0.0145915542F,    0.00119129301F,
      0.00621725F,      -0.00396024203F,  -0.0991808549F,   2.61711848E-5F,
      0.0288820099F,    0.0247942228F,    -0.00447910791F,  -0.00300585036F,
      -0.00245903432F,  -0.0985395F,      0.00741299847F,   -0.00758320885F,
      -0.00589598948F,  -0.00357442815F,  0.00112205616F,   -0.160513893F,
      0.00270921574F,   0.00273453933F,   0.0194569565F,    -0.112213716F,
      -0.00829645F,     -0.000750220614F, -0.00122310396F,  0.0142268734F,
      0.00905021653F,   0.103238977F,     0.0130195804F,    -0.174182087F,
      -0.0117587699F,   0.132393092F,     5.86699607E-5F,   0.000124476821F,
      -0.00053076219F,  0.00248901593F,   -0.000387881271F, -0.00109869684F,
      -0.000567134819F, -0.00109686283F,  0.02387782F,      -3.63160871E-5F,
      -0.00609645713F,  0.0409346707F,    -0.00701291347F,  -0.00919855572F,
      -0.0956973806F,   -0.000855024788F, 0.00290714251F,   0.00176554115F,
      0.0098924879F,    0.13373448F,      0.0502766334F,    -0.0688679889F,
      -0.125380814F,    0.00121869461F,   -0.00274302135F,  0.0522492304F,
      -0.00963872F,     0.0610699F,       -0.0844984129F,   0.00541153F,
      0.0150168967F,    -0.000882919878F, -0.018871272F,    -0.000918548671F,
      0.0421002731F,    0.00202887901F,   -0.00187768938F,  -0.0524490699F,
      0.000330153882F,  -0.00201202184F,  -0.00150684721F,  -0.106714979F,
      0.0111479871F,    0.000864054193F,  -0.00107710843F,  -0.000304668443F,
      -0.0170039926F,   -0.00511689F,     0.0938041359F,    0.0381994173F,
      0.00540081644F,   -0.0127803078F,   0.00572882732F,   0.0706986338F,
      0.00049541879F,   -0.0116030183F,   0.0159163922F,    0.132004857F,
      0.0269345306F,    -0.0411693566F,   0.00107141747F,   -0.00644039409F,
      0.00347419176F,   0.00373189128F,   -0.0140809547F,   -0.00648943847F,
      -0.00687451335F,  0.00627272716F,   -0.0221989229F,   -0.00985278562F,
      0.0110013327F,    0.036441125F,     0.014067905F,     0.0185553674F,
      -0.0150249293F,   -0.00375093613F,  -0.0357098356F,   0.000371970877F,
      -0.0825389177F,   0.00505056465F,   0.000532988342F,  0.00736738555F,
      -0.0635705516F,   -3.40382467E-5F,  0.00502072694F,   0.0180624984F,
      -0.00570813613F,  -0.0134493653F,   -0.000114139279F, 0.00017823423F,
      -0.0559585877F,   0.0229962524F,    -0.020728359F,    0.0143619385F,
      -0.0875991955F,   -0.0101799071F,   -0.024066098F,    -0.0179767199F,
      -0.0573056377F,   0.044576861F,     0.000181824304F,  0.0189728644F,
      0.00349058351F,   -0.217242062F,    -0.0058870716F,   0.0405961648F,
      0.0153884571F,    -0.0276632383F,   -0.0151631404F,   -0.00786249712F,
      -0.030841561F,    0.00106509204F,   -0.00782022346F,  0.0082692029F,
      0.0113058807F,    -0.00695260568F,  -0.0497051664F,   -0.00412077783F,
      0.005991471F,     -0.0290992651F,   0.00929375552F,   -0.0080729425F,
      0.0275927074F,    0.000338342506F,  -0.0166487135F,   0.00963561703F,
      -0.0121077988F,   0.00955946092F,   -0.180156782F,    -0.00214600354F,
      -0.0125697562F,   -0.00137471443F,  0.0136403674F,    -0.0012789655F,
      1.40628144E-5F,   -0.00429559778F,  0.0193567164F,    -0.000372123788F,
      0.0399229042F,    0.00706020184F,   -0.00994324312F,  -0.00945355929F,
      0.053805761F,     0.0135746663F,    -0.054167334F,    -0.00505843665F,
      -0.000848945056F, 0.00484906277F,   0.00248761F,      0.0171708744F,
      0.0191855673F,    -0.00622265926F,  0.0075661242F,    0.00771086896F,
      -0.000474115484F, 0.00718709F,      0.0112716993F,    0.0554932132F,
      0.000180715331F,  0.00425315462F,   -0.00183575461F,  -3.94854469E-5F,
      3.3788514E-5F,    0.00386808207F,   0.00161695771F,   -0.0601852573F,
      0.0129765682F,    0.0033448881F,    -0.0209451709F,   0.0131391305F,
      -0.00354985381F,  -0.00127464125F,  0.00260834815F,   -0.0040828404F,
      0.00555834686F,   0.160374478F,     0.0178864673F,    0.0970129743F,
      -0.00119459722F,  0.0017075995F,    -0.0106811253F,   -0.0135023491F,
      0.0718180388F,    0.0396867469F,    -0.0380369574F,   0.00395198446F,
      0.00646085292F,   -0.0212954618F,   -0.00888305157F,  -0.000318268401F,
      0.000881629123F,  -0.000852009805F, -0.00542392768F,  0.000140928969F,
      0.00938701F,      0.00439199433F,   0.0822964832F,    0.000343250751F,
      -0.00082506286F,  -0.00274592731F,  0.0018239558F,    0.000370745925F,
      0.00317463418F,   0.0148885949F,    -0.0771108046F,   0.0103918966F,
      -0.00711305207F,  -0.0010198243F,   0.00463588769F,   0.157628864F,
      -0.00513939932F,  -0.067610234F,    0.195564717F,     0.0407267399F,
      0.0196026918F,    0.00452105049F,   -0.0821286663F,   -0.0390872806F,
      -0.00576363225F,  0.0387735851F,    0.00108715519F,   -0.00023271433F,
      -0.0391827449F,   0.00365680712F,   -0.109896205F,    0.00242763967F,
      0.0215034112F,    -0.00141352171F,  0.0271871351F,    0.00613742601F,
      -0.00671308814F,  0.00651179068F,   0.00464551337F,   -0.0877560154F,
      0.000401016558F,  -0.00101045799F,  -0.0038594543F,   0.0084049711F,
      -0.00355830742F,  -0.0274197888F,   -0.156455025F,    0.0061225309F,
      -0.0303422771F,   -0.00872753467F,  -0.00296742539F,  0.0105719483F,
      0.00256083021F,   -0.000590009557F, 3.2541313E-5F,    -0.000315975893F,
      0.0472089723F,    0.0564066544F,    0.000958625227F,  0.00809147488F,
      -0.0026891008F,   0.00994159747F,   0.0266902782F,    -9.09463633E-5F,
      0.0231766347F,    7.39515672E-5F,   -0.000818543485F, -0.000387888344F,
      0.00180229254F,   0.000419004471F,  -0.000143691912F, -0.00849154F,
      -0.00303285755F,  0.00229802378F,   -0.00523008732F,  -0.00982593093F,
      -0.0634372234F,   -0.00966440421F,  0.00390287954F,   -0.00393756619F,
      -0.00258959248F,  -0.00951138884F,  0.00117817323F,   -0.0066196057F,
      -0.106730245F,    -0.0191456955F,   -0.0355957039F,   -0.0146408761F,
      0.010181075F,     0.0182597954F,    0.00287473435F,   0.00311690872F,
      0.000749245577F,  -0.0753863901F,   -2.49237655E-6F,  0.0331173316F,
      0.00153616315F,   -0.0139333028F,   0.0424549654F,    0.003855824F,
      -0.000466364087F, 0.0270436238F,    -0.005988135F,    0.0174780842F,
      0.0294550415F,    -0.0010467075F,   0.0118141202F,    -0.015347518F,
      -0.0200422015F,   0.0334838182F,    -0.126879334F,    -0.00570217567F,
      0.0497274958F,    0.0139471488F,    -0.00942287F,     -0.185853049F,
      -0.00197222456F,  -0.135231435F,    -0.000508284487F, -0.0315054283F,
      -0.0986040384F,   0.0577993169F,    -0.00312748179F,  0.0225855745F,
      -0.000175076944F, -0.00028970823F,  -0.0597486906F,   -0.0151358461F,
      -0.0610801131F,   6.58598219E-6F,   -0.00690713106F,  0.0112176975F,
      0.00128451409F,   -0.020741228F,    0.101864658F,     0.00126444024F,
      0.0301863793F,    -0.00163323572F,  0.00117655331F,   -0.0888636932F,
      -5.81458335E-5F,  0.0309854988F,    0.0019981754F,    0.0110544367F,
      0.00280677504F,   0.0261043515F,    0.0126044722F,    0.000110393426F,
      0.0374291353F,    -0.0204336531F,   -0.00552914804F,  -0.0225009359F,
      -0.000642686267F, 0.000664175488F,  -0.000352519186F, -0.0280420631F,
      -0.0148163838F,   -0.00085235195F,  -0.0651497692F,   -0.000274499092F,
      0.0170628354F,    -0.00190405187F,  0.00195317343F,   0.000149671294F,
      0.00327512273F,   0.00852747F,      0.0315057822F,    8.73110475E-5F,
      -0.000605952344F, 0.0187656134F,    -0.0243841652F,   -0.000107033942F,
      -0.037755996F,    0.0310810488F,    0.076190576F,     -0.000634965603F,
      0.000564539456F,  0.00575539889F,   0.248707563F,     -0.000308337971F,
      0.0215083472F,    -0.0825016722F,   -0.0187458489F,   -0.0927705318F,
      -0.015712304F,    0.14484185F,      -0.0115215108F,   -0.00170000922F,
      0.0145483492F,    0.000922772451F,  -0.00259188819F,  0.00475018797F,
      0.00390146766F,   -0.0203045234F,   -0.0475396216F,   0.00313332723F,
      0.00369995926F,   -0.000729818479F, -0.0167903882F,   0.0273110066F,
      0.00635063089F,   -0.00104942F,     0.071261473F,     0.00530780759F,
      0.00731963944F,   -0.00331887812F,  0.000256339F,     -3.13873315E-5F,
      0.0175880883F,    0.0810402855F,    -0.00555240223F,  0.00108502351F,
      0.000125122868F,  -3.39730177E-5F,  -0.0699553564F,   -0.00210734666F,
      -0.0525985472F,   -0.000107097934F, -0.0342563614F,   -0.0157211497F,
      0.00173601473F,   -0.00051304989F,  0.0544312485F,    0.00811437797F,
      -0.018900929F,    -0.135869563F,    -0.0571029708F,   -0.0437265262F,
      0.00707540847F,   -0.00158187188F,  -0.00308906077F,  0.00630991766F,
      -0.0161475185F,   -0.0690191612F,   0.0574202612F,    0.00394807244F,
      0.0495631099F,    -0.0157042369F,   0.0249285623F,    -0.000105165091F,
      -0.120712452F,    -0.0247891229F,   0.0168040283F,    0.00164060458F,
      -0.000408605672F, 0.00235493411F,   -0.00100349297F,  7.07664949E-5F,
      0.0184525754F,    -0.000187459402F, -5.26555523E-5F,  0.000140940203F,
      -0.0220327433F,   -0.00104760681F,  -0.00261385157F,  0.108681545F,
      -0.110309467F,    7.07585277E-5F,   0.0111334566F,    -0.0679689571F,
      -0.00500246231F,  -0.0124117825F,   0.0327412859F,    -0.0835389122F,
      -0.000526413962F, 0.159951299F,     -0.00752336765F,  -0.0208410788F,
      0.00229832157F,   0.0124830678F,    0.0736222044F,    -0.0111552682F,
      0.0261130165F,    -0.0727323294F,   -0.00476233382F,  0.0416221507F,
      0.00396446371F,   -0.00164382055F,  0.00371036143F,   0.00885443762F,
      0.105830818F,     -0.026345877F,    -0.00408091396F,  0.0126476483F,
      0.00267303409F,   -0.000917271595F, 0.00120322313F,   -0.0162365586F,
      0.0785432905F,    0.000369540765F,  -0.035723988F,    0.0732646883F,
      0.00498895394F,   0.0147624193F,    -0.00029166587F,  0.000198140973F,
      -0.000256419968F, -0.0122586032F,   0.111641854F,     -0.0678989366F,
      -0.0371649861F,   -0.00771801267F,  -0.000501040544F, -0.100287527F,
      0.185100451F,     -0.0648422241F,   0.000642608094F,  -0.100356989F,
      -0.0477209501F,   0.00201037317F,   -0.0342678651F,   -0.0150050428F,
      -0.00428009639F,  0.00107277359F,   -0.0748442709F,   0.0433729105F,
      -0.00119746663F,  0.00266423426F,   0.0264526233F,    0.00490532722F,
      0.0287250578F,    0.0332983769F,    -0.0143790245F,   0.00466257939F,
      -0.0251652021F,   0.079649277F,     -0.00456481287F,  0.0421156846F,
      0.0263974424F,    -0.000299460138F, -0.0088561F,      0.0447028428F,
      -0.0672218949F,   -0.0154211242F,   0.00333628291F,   5.84233203E-5F,
      -0.00450219121F,  -0.00118868379F,  0.0101462118F,    -0.000110024986F,
      -0.0170727596F,   -0.00330413F,     -0.024722483F,    -0.000359495811F,
      -0.00159286836F,  0.00471497839F,   0.0553272925F,    0.0111930287F,
      -0.000884383451F, 0.0263150409F,    0.163908511F,     -0.00944588054F,
      -0.000171154956F, -0.00454119965F,  -0.000499709044F, 0.0134667465F,
      -0.000440325559F, 0.0176086798F,    -0.0342061594F,   -0.00119612133F,
      -0.000363196188F, -0.00798483565F,  0.0127690993F,    0.0192808285F,
      0.000219102061F,  -0.055264283F,    -0.00378783233F,  0.0125577087F,
      -0.000178362679F, 0.0216966979F,    -0.0667936131F,   0.00152251404F,
      0.0125769526F,    0.027820304F,     -0.0377875194F,   -0.509855747F,
      0.00299466704F,   -0.000849016244F, 0.00360381859F,   -0.000661532744F,
      -0.000200376715F, 0.000897653168F,  -0.0583989434F,   -0.00769066811F,
      -0.000593957317F, -0.00034414322F,  -0.00895662326F,  -0.00700473692F,
      0.000643535808F,  -0.619575F,       0.0454060473F,    0.00191342761F,
      0.0597218089F,    -0.0177612975F,   -0.000311494834F, -0.000255559629F,
      0.00616355799F,   0.00312556513F,   0.00221493444F,   0.000314169389F,
      -0.0240353718F,   -0.0554185808F,   0.000404248422F,  0.00221255864F,
      -0.000512506464F, -0.0104709575F,   -0.0228308439F,   0.000475795066F,
      -0.0174685791F,   -0.000364491192F, 0.176135778F,     -0.00956498273F,
      0.0962645784F,    0.0189978722F,    0.000419633172F,  0.000951791706F,
      -0.0058047343F,   -0.0550180934F,   -0.00820212811F,  -0.00233529694F,
      -0.000750485167F, 0.000197994654F,  0.000971945294F,  0.190576449F,
      0.0168904737F,    0.00015177099F,   0.00674569421F,   0.00735155819F,
      0.0216228645F,    -0.0508911274F,   0.00305007841F,   0.00736936787F,
      0.0001854617F,    0.00439437525F,   0.0748797208F,    0.0105144801F,
      -0.0547153316F,   -0.030428037F,    0.00136798015F,   -0.000810632482F,
      6.85689811E-5F,   -0.000788519334F, -0.0139843104F,   -0.0117141912F,
      -0.0148905916F,   0.127972901F,     0.000565779104F,  0.0276320968F,
      -0.0384831615F,   0.0106597561F,    0.0540800728F,    0.00370144937F,
      0.019245239F,     -0.000123381746F, -0.000114747832F, 0.000238741501F,
      0.0776960179F,    -0.0654043108F,   0.00128044316F,   -0.00813475717F,
      0.00015422047F,   -0.00593229895F,  -0.000196560795F, 0.000666226842F,
      -0.000244986F,    0.00020843184F,   -0.000141101278F, -0.000196282781F,
      0.0012501945F,    -0.00131098484F,  0.000141836761F,  -0.00199860684F,
      0.00928902905F,   -0.00152037351F,  0.0123434942F,    -0.0029592F,
      -0.0384065695F,   0.00741938269F,   -0.0327212177F,   -0.00169086515F,
      -0.00695581222F,  -0.0163319428F,   0.000581268454F,  0.00342250895F,
      -0.000119021395F, 0.0776363F,       -0.0190722421F,   -0.0252555907F,
      -0.0165664647F,   0.0250557624F,    0.000952373F,     0.00546899F,
      -0.000210802216F, 0.0410653949F,    0.000333183183F,  0.0033800106F,
      0.000686915126F,  0.00322990236F,   0.000253222563F,  -0.00127998483F,
      0.0174569786F,    -0.00230020308F,  0.000166432175F,  -0.0040742103F,
      -0.00747730397F,  -0.000405795785F, 0.00135198585F,   0.0258375332F,
      0.0191968773F,    -0.00685103564F,  -0.0599452034F,   -0.0062314868F,
      0.0342193805F,    -0.0242068842F,   -0.00328066503F,  0.028658988F,
      0.00549150351F,   -0.0532608442F,   0.000489419559F,  0.00177401595F,
      -0.0130196009F,   -0.00457429187F,  -0.00632991036F,  -0.00198090728F,
      -1.8432851E-5F,   0.000203262389F,  0.0169068538F,    -0.00269229035F,
      -0.0451242402F,   -0.0079566F,      0.0279497504F,    -0.00453462964F,
      -3.55253433E-5F,  -0.000994643779F, -0.0694023594F,   -6.46175E-5F,
      -0.00639632205F,  -0.000671178917F, -0.00105636159F,  0.00550519396F,
      -0.00082906679F,  -0.0145804547F,   0.000199181144F,  -0.0237885956F,
      -0.00382464193F,  0.0134124886F,    0.00164319901F,   0.000203237447F,
      0.00233633397F,   0.00351623748F,   -0.00483009452F,  -0.000728701125F,
      -0.011874848F,    -0.000693717273F, -0.0298165809F,   -0.00519095547F,
      0.000271435856F,  -0.00400570454F,  -0.0210768972F,   0.000627063389F,
      0.000815495208F,  0.000154721871F,  -0.00494252704F,  0.0001594442F,
      0.000897786929F,  -0.027819192F,    0.00702390168F,   -6.48058631E-5F,
      -0.000605302863F, 0.00249067275F,   0.00280356687F,   -0.00737546477F,
      -0.0320664495F,   -0.0603762157F,   -0.0691759959F,   0.000554855098F,
      -0.000309423398F, 0.0142100835F,    -0.0329258032F,   -4.92126201E-5F,
      0.165342078F,     0.00619262643F,   -0.0264353659F,   -0.0233777948F,
      -0.0471508652F,   -0.00420835335F,  -0.00490105944F,  -0.0221462045F,
      -0.000710495398F, -0.0196131766F,   0.0027684F,       -0.000522389368F,
      0.00725211808F,   -0.0157533344F,   -0.0142330844F,   0.000400500372F,
      -0.050233122F,    -0.00020735024F,  -0.00749767479F,  0.0162866749F,
      -0.00359015493F,  -0.000286229F,    -0.00848473888F,  0.000245987903F,
      -0.00116053945F,  -0.00131294224F,  0.000136662406F,  3.96386713E-5F,
      0.000545000366F,  -0.00407688F,     0.0230317693F,    -0.000443035155F,
      -0.000179164796F, -0.000448465405F, 0.0110382885F,    0.00240756851F,
      -0.0312978774F,   -0.00432281801F,  0.029111376F,     -0.0239046197F,
      0.000120181314F,  -0.000301706576F, 0.000108028202F,  0.00136372168F,
      -0.000925521366F, -0.0524285957F,   0.00505313324F,   0.00787255F,
      0.000135985625F,  0.0298998598F,    -0.0181634631F,   -0.000612892269F,
      -0.00470799766F,  0.0136214299F,    -0.00842949376F,  0.00661518378F,
      0.000447480968F,  -0.0262108445F,   -0.0051009953F,   -0.000261767826F,
      0.00414674915F,   -0.00369807798F,  -0.024204772F,    0.00022990501F,
      1.38243877E-6F,   -0.00807103F,     -0.000481183262F, -0.000498315843F,
      0.0567244925F,    0.000109512737F,  -0.000638066907F, 9.97132593E-5F,
      0.00303834141F,   -0.021506317F,    -0.000236542299F, -0.0691759661F,
      0.0690433607F,    -0.00417267252F,  -0.0119400453F,   0.0084587913F,
      0.00455519278F,   0.00167028187F,   -0.00521173235F,  0.015952399F,
      -0.000738578499F, 0.0260974951F,    0.00540008349F,   0.00218918594F,
      -0.00784129091F,  -0.00357461115F,  -0.0326011479F,   -0.00883111916F,
      0.019257633F,     -0.0309946705F,   0.00554635469F,   -0.0405604541F,
      -0.00694804033F,  -0.00138826785F,  0.0215149373F,    -0.00425733393F,
      0.0488502085F,    -0.0227380674F,   -0.00230037607F,  -0.000867218419F,
      -0.00251623476F,  -0.000201593226F, -0.000263017981F, 0.00748453336F,
      0.0035975303F,    -6.35836332E-5F,  -0.0170227606F,   -0.0820010379F,
      0.00804928F,      -0.036916595F,    -2.63193178E-5F,  8.49580101E-5F,
      -0.000244568451F, 0.189676046F,     0.0564136542F,    0.00913860742F,
      -0.00487486972F,  0.00484882295F,   -0.000338730228F, 0.0517137684F,
      0.0519011207F,    0.00887640473F,   0.000109827401F,  0.0448451F,
      -0.0259796F,      0.0048389812F,    -0.0230743F,      0.00432427879F,
      0.00113520084F,   -0.000743415556F, 0.0153859342F,    0.0241894331F,
      -0.000358915771F, 0.00252561946F,   -0.00896606501F,  -0.00770540861F,
      0.00229427195F,   0.011491783F,     0.00300636562F,   -0.0291112829F,
      0.00471667759F,   -0.00740354229F,  0.0411397F,       -0.0330031589F,
      -0.0059613036F,   1.72313557E-5F,   0.00284899236F,   0.0184357874F,
      0.0169975106F,    0.109620161F,     0.00105592515F,   -0.00329352636F,
      -0.0146436421F,   -0.0144183673F,   -0.0031701324F,   -1.30080116E-5F,
      -0.0228346828F,   -0.00584540796F,  0.0266078655F,    -0.00044504879F,
      0.000275519735F,  0.11350549F,      0.0204152148F,    0.00258243037F,
      -0.000122261423F, 0.00327991019F,   -0.0295557361F,   -0.0133186551F,
      -0.00066202489F,  -0.000158070092F, 0.00473620417F,   -0.000727552513F,
      -0.000464703422F, -0.0194806047F,   -0.0215001609F,   -0.0269179214F,
      -0.000224380245F, -0.00116730283F,  -0.000440106814F, -0.00326653593F,
      0.000401564175F,  -0.0278664511F,   -0.0087978933F,   -0.0241048299F,
      0.000186768302F,  0.0084572F,       -0.0335189775F,   0.000416661642F,
      -0.0064952F,      0.00698973099F,   0.0116506042F,    0.0141201206F,
      0.00648775324F,   -0.000332226948F, 0.0266502835F,    -0.000135489725F,
      -0.00334566436F,  -0.000807269593F, 0.00732720317F,   0.00145688164F,
      -0.00236210716F,  0.0048403223F,    -0.000290717348F, -0.0233775117F,
      -0.00019490438F,  -0.0424013287F,   0.00365513284F,   0.0126142958F,
      -0.00558633683F,  -0.0173754077F,   0.0127880126F,    2.58652544E-5F,
      0.00101930974F,   -0.00872594491F,  -0.000443125959F, 0.000208297613F,
      0.0521670207F,    -0.00767835928F,  0.000713706F,     0.0232192744F,
      -0.000774113054F, -0.00168361026F,  0.00576139847F,   0.000389983412F,
      0.0114722345F,    0.0447932705F,    -0.0347633921F,   0.0310543682F,
      -0.00865063537F,  -0.0013355118F,   0.000158982715F,  -0.000993498834F,
      -0.0140050659F,   0.00645087333F,   -5.46284828E-5F,  -0.000213275664F,
      -0.000119558143F, -0.00822392199F,  -0.000174074899F, -0.0263162814F,
      0.000818970788F,  -0.000422729F,    0.0193791222F,    0.00580308307F,
      -0.0531178638F,   -0.00363211543F,  -0.000171878302F, 0.00856660213F,
      0.00578257721F,   0.00483022397F,   -0.0137656042F,   0.000900739338F,
      -0.0150311692F,   0.000402260077F,  -0.00786450319F,  0.000663833111F,
      0.000345996901F,  -0.000137612587F, 0.0125843287F,    0.027283892F,
      -0.0083249F,      0.0381750204F,    0.000426513492F,  0.0155400168F,
      0.0242837425F,    -0.00019688037F,  0.0761946738F,    0.020138206F,
      0.0177373402F,    -0.000109931229F, -0.000126576138F, -6.08700284E-5F,
      -0.0228301827F,   0.00891434122F,   0.000273625745F,  -0.00669473736F,
      -0.00334317866F,  -0.00811995473F,  0.00111437973F,   -0.000526504591F,
      0.000158716604F,  0.000110757639F,  -0.000849792F,    -0.000437943847F,
      -0.000482622476F, -0.00029238229F,  0.000364155305F,  -0.0716130808F,
      -0.0160468016F,   0.0262522846F,    0.00984505191F,   -0.023198314F,
      0.0133708119F,    -0.0426772758F,   0.00974244159F,   0.0130398627F,
      -0.0068434421F,   -0.005546459F,    0.00033457F,      0.00529577583F,
      -0.000136300703F, -0.00652100146F,  0.0076515507F,    0.00853052642F};
  static const float A[1200] = {
      -0.0185063314F,   -0.0149371522F,   0.00959109422F,   0.00649663433F,
      -0.076242514F,    0.0792815089F,    -0.00803655572F,  0.0211292263F,
      0.0149814673F,    -0.00852664094F,  -0.0368006043F,   -0.0417494364F,
      -0.000242701979F, -3.81596656E-5F,  0.000238602632F,  -0.0913784653F,
      0.00527320337F,   0.0686500892F,    0.00033601516F,   -0.000225435142F,
      -7.67770325E-5F,  0.0576552078F,    -0.0060804449F,   -0.058956407F,
      -0.524561286F,    0.107117243F,     0.287897795F,     0.065357089F,
      -0.0416149646F,   -0.0414575748F,   0.0688584223F,    0.0625037327F,
      0.0756011382F,    0.116802678F,     -0.0304196961F,   -0.0726257041F,
      0.00179890066F,   -0.0399549119F,   0.0275109652F,    -0.226995915F,
      0.128213257F,     0.100191109F,     -0.245603427F,    -0.0887358487F,
      0.371579915F,     0.160462F,        -0.0196233019F,   -0.0947477445F,
      -0.118664794F,    -0.230598763F,    0.326343805F,     -0.000300469F,
      0.00103828579F,   0.000319280865F,  -0.0485580899F,   -0.024990065F,
      -0.024126336F,    -0.0726294F,      -0.178107172F,    0.20375748F,
      0.0763386339F,    0.0675410554F,    0.0381890722F,    -0.0758776218F,
      -0.0404417962F,   -0.0425950959F,   0.287152141F,     -0.163394392F,
      -0.0572818108F,   -0.136681467F,    -0.0744578093F,   0.0243694801F,
      -0.0381104909F,   0.0180003103F,    0.0299364571F,    -0.0390720814F,
      -0.0334786363F,   0.0410517417F,    0.0686472207F,    0.033361014F,
      0.0475490913F,    0.156129897F,     -0.102854744F,    -0.108010955F,
      0.111754365F,     0.0815763772F,    0.0848265216F,    0.137777373F,
      -0.054879535F,    -0.114213139F,    -0.00011136711F,  -0.00037642906F,
      0.000499779417F,  -0.263773948F,    0.0872296F,       0.100739531F,
      -0.0437816679F,   -0.0792673677F,   0.134304583F,     -0.205342695F,
      0.0285605919F,    0.113808773F,     -0.138413221F,    -0.0557854362F,
      0.111569241F,     -0.0331688747F,   -0.0167518035F,   -0.0201598573F,
      -0.000160955911F, -6.61616359E-5F,  0.000126534695F,  0.000446660473F,
      -0.000652738789F, 0.000549886434F,  0.156085387F,     -0.114830405F,
      -0.0523751602F,   -0.0295594577F,   -0.0298929457F,   0.00908883847F,
      0.161098942F,     0.311587363F,     -0.486503035F,    0.0760901F,
      0.0101279598F,    -0.051852826F,    0.0448114388F,    0.0231300127F,
      -0.0457665808F,   -0.0104811005F,   0.0267677493F,    0.0189444646F,
      0.585180461F,     0.276008636F,     -1.04097104F,     -0.504882336F,
      0.170029491F,     0.259462684F,     -0.0910549909F,   0.047388941F,
      0.0450361818F,    -0.45954749F,     -1.11887503F,     1.77747846F,
      -0.0536975227F,   -0.0799393877F,   0.108119488F,     -0.318017691F,
      -0.821926296F,    1.27830541F,      -0.0104194116F,   0.00341624022F,
      -0.00301313912F,  -0.0271209199F,   0.0774903223F,    -0.114261016F,
      0.00122787966F,   -0.000620592269F, -0.000325512374F, -0.130317733F,
      -0.131210327F,    0.214959085F,     -0.018251216F,    -0.0070883357F,
      0.000715656F,     -0.0880954787F,   -0.0318556949F,   -0.0179349314F,
      -0.0365022905F,   -0.0190397501F,   -0.0023315635F,   0.0337614939F,
      -0.0109152868F,   -0.00613890169F,  -0.0186525509F,   0.0634597F,
      0.0433536135F,    0.000113212052F,  -0.000155173315F, -0.000111268128F,
      0.0865241662F,    0.256456673F,     -0.360332131F,    -0.0888013F,
      0.000687508495F,  0.051150769F,     -0.0620672032F,   -0.0666265786F,
      -0.0617182925F,   0.0626490712F,    -0.0229375F,      -0.118592374F,
      -0.0627148896F,   -0.00885166507F,  0.0614186563F,    -0.697208643F,
      -0.246962905F,    0.955388546F,     -0.0371632464F,   0.0744449869F,
      0.0243972801F,    -0.295881093F,    0.106445134F,     0.111857936F,
      0.0406589F,       0.0135262F,       0.0113188177F,    0.147169456F,
      -0.0393066816F,   -0.0703343749F,   0.0502557047F,    0.161579862F,
      -0.127201393F,    0.000110735025F,  0.00030644497F,   0.000638392929F,
      -0.0221339874F,   0.0106407674F,    0.0430828705F,    0.726077378F,
      0.163677603F,     -0.851761222F,    -0.301997155F,    -0.142213225F,
      0.475195229F,     0.000373549294F,  -0.000269256416F, 9.77893724E-5F,
      -0.698015809F,    -0.0210949425F,   0.623363435F,     0.119296342F,
      -0.0715816319F,   -0.0303012282F,   0.288744777F,     -0.144549057F,
      -0.191340357F,    0.0005547576F,    -0.000279696775F, -0.000104532584F,
      0.668244F,        0.160479784F,     -0.826386154F,    -0.0793811828F,
      -0.0195725709F,   0.0546709374F,    -0.134957716F,    -0.051848609F,
      0.18506211F,      0.036296133F,     -0.00853568316F,  -0.0173192024F,
      0.0265396666F,    -0.028663598F,    -0.0286674723F,   0.0081309611F,
      0.00864312053F,   0.0164190196F,    0.0897066742F,    0.0274615586F,
      -0.0908611417F,   7.57064772E-5F,   3.15758152E-5F,   -0.000128685511F,
      -0.742372513F,    -0.344099909F,    1.23181593F,      -0.0944308862F,
      -0.0522959083F,   0.0416614451F,    -0.410481095F,    0.158586726F,
      0.138022378F,     -0.000303550594F, 0.000181153489F,  2.21591436E-5F,
      0.169230536F,     -0.130095437F,    -0.0811998F,      -0.0414792486F,
      0.272995412F,     -0.263415277F,    -0.00947134756F,  0.049010057F,
      -0.120777763F,    0.0407535024F,    0.130767152F,     -0.0220115799F,
      0.0472845286F,    0.0157479532F,    0.00490742829F,   -0.26726988F,
      0.112761833F,     0.106083736F,     -0.128591314F,    -0.104335837F,
      0.21809F,         -0.158159971F,    0.0338743739F,    0.0453475863F,
      -0.0760464817F,   -0.071532093F,    -0.0713020563F,   -0.173715875F,
      0.0269496683F,    0.0869380832F,    0.0165499467F,    -0.00375308702F,
      -0.0159374308F,   -0.537195623F,    0.180226251F,     0.173726946F,
      0.0327937F,       -0.0113618504F,   -0.0758859068F,   0.0332014896F,
      0.0115256133F,    -0.0154552488F,   0.118279718F,     0.0462243073F,
      -0.00394416647F,  -0.407246858F,    0.0836311355F,    0.369418085F,
      -0.033278212F,    0.0319151282F,    0.0964911878F,    -0.00442330586F,
      0.000279037078F,  -0.00175968779F,  -0.002947F,       -0.00441588275F,
      0.0180991702F,    0.0763534606F,    0.0288810395F,    -0.026601404F,
      0.0851891115F,    -0.0459488928F,   -0.057986889F,    -0.436191797F,
      -0.475663453F,    1.17012966F,      0.0856711417F,    -0.0540861338F,
      -0.0882113278F,   -1.07159853F,     1.3765502F,       -0.557946801F,
      -0.11464975F,     -0.0146697126F,   0.0025291231F,    -0.379162371F,
      -0.191405594F,    0.526586771F,     0.000421737612F,  -0.000615271856F,
      0.000322867592F,  0.000244625466F,  0.000492207415F,  -0.000854268F,
      0.0236168168F,    0.0560960434F,    -0.0568692312F,   0.0261652134F,
      -0.000359477126F, 0.122190945F,     0.0590622686F,    -0.0175931342F,
      -0.084912695F,    0.00441759545F,   0.0269106533F,    0.0129899364F,
      -0.000276617F,    4.43732715E-5F,   -0.000142517179F, 0.000740273565F,
      0.000751052226F,  0.000974055554F,  0.109137371F,     -0.0671208203F,
      -0.0804445744F,   0.0153503F,       0.00177686347F,   0.00918270368F,
      -0.215100706F,    0.126886621F,     0.125731409F,     0.133033484F,
      -0.0634788871F,   -0.0610607229F,   0.023158364F,     0.0601265505F,
      -0.019309992F,    -0.0345919654F,   0.000833126251F,  0.0265957285F,
      -0.783064246F,    -0.247396559F,    0.996139944F,     -0.00198913203F,
      -0.00165932928F,  -0.00221476774F,  0.209089234F,     -0.235965937F,
      0.0875831321F,    0.0381040275F,    -0.00971471F,     0.0179186985F,
      0.020586F,        -0.0417273231F,   -0.103930086F,    0.445219308F,
      -0.178501323F,    -0.234423235F,    0.435418725F,     -0.0439696386F,
      -0.256293327F,    -0.199610472F,    0.0850599557F,    0.0678680688F,
      -0.584433317F,    0.163783014F,     0.125636145F,     -0.024155695F,
      -0.0422195382F,   0.0868774876F,    -0.084360607F,    0.00719788065F,
      0.000286525901F,  0.410103947F,     -0.22317639F,     -0.23959817F,
      -0.0444748066F,   -0.000295772625F, 0.0459589586F,    0.336604297F,
      0.112937838F,     -0.305065483F,    -0.422415912F,    -0.133943811F,
      0.39812392F,      0.0977553353F,    -0.138871461F,    -0.00881784782F,
      0.025233686F,     -0.0582508296F,   -0.0735794529F,   -0.0323073231F,
      -0.0202117F,      0.0321153328F,    -0.0362121798F,   -0.201739445F,
      0.257355124F,     0.00012426688F,   5.80598789E-5F,   -0.000176905131F,
      0.273584545F,     -0.0222843178F,   -0.152743697F,    0.0147664016F,
      0.0411803648F,    0.0693415329F,    0.0128369611F,    0.0566191636F,
      -0.0765666664F,   -0.790771365F,    -0.28117F,        1.18641543F,
      0.00102483819F,   -0.000335995108F, -0.000722596538F, -0.00195459719F,
      0.0313732475F,    0.047658585F,     -0.0077119912F,   -0.00777185848F,
      -0.00633896422F,  0.57221967F,      0.388236851F,     -1.19473875F,
      0.0488040969F,    -0.0463717878F,   -0.0362633765F,   0.000459317816F,
      0.000272250734F,  0.0007665148F,    0.00018143245F,   -0.000107768996F,
      1.76696321E-5F,   -0.000363005674F, 0.000440601783F,  -0.000226537581F,
      -0.235064059F,    0.125558734F,     0.08954034F,      -0.0184527356F,
      0.00293028983F,   0.0223360639F,    -0.54418093F,     -0.404378444F,
      1.00959885F,      0.212576345F,     -0.137023956F,    0.0148915313F,
      0.142201334F,     -0.0684925094F,   -0.0678593889F,   -0.204713985F,
      0.00454470655F,   0.119008847F,     0.0767824277F,    -0.0716513693F,
      -0.0787513703F,   0.116506107F,     0.360355765F,     -0.465648621F,
      0.0318739079F,    -0.014598689F,    -0.0583333224F,   -0.119416155F,
      -0.00743200025F,  0.0435269885F,    0.0190842748F,    -0.00661435165F,
      0.0100238696F,    0.166893512F,     -0.277739674F,    0.169495687F,
      0.816271722F,     0.322908431F,     -1.14659441F,     -0.0878818333F,
      0.0533098392F,    0.128648236F,     -0.0423653759F,   -0.0276637096F,
      0.0590458438F,    -0.000248414639F, 0.0432782061F,    0.0630733073F,
      -0.0322865061F,   -0.0521850623F,   0.0857822224F,    0.0451632626F,
      -0.0162474103F,   -0.0146965748F,   -0.037211556F,    0.0181242153F,
      -0.0160861555F,   -0.0484956279F,   -0.00440467428F,  0.0110566923F,
      -0.0565741025F,   0.0355815254F,    0.0602971539F,    -0.0346252881F,
      0.147159785F,     -0.135528818F,    0.038763091F,     0.0709861442F,
      0.0486921445F,    -0.0383530892F,   0.0192163754F,    0.0376171321F,
      0.0613788553F,    -0.0599686652F,   -0.0439339802F,   0.228982896F,
      -0.0175459515F,   -0.129245698F,    0.403784603F,     -0.187048137F,
      -0.0624138974F,   0.0419738516F,    -0.0931839645F,   -0.12209931F,
      -0.148892701F,    -0.0327321F,      0.171641439F,     0.0430359058F,
      0.0854147226F,    -0.109898254F,    -0.417430699F,    0.0479255468F,
      0.016689973F,     -0.0216069315F,   -0.014878368F,    -0.0215174332F,
      -0.237952963F,    0.008389147F,     0.248903587F,     0.611206651F,
      0.671706736F,     -1.30813873F,     -0.00174554344F,  0.0010640343F,
      0.000142838675F,  -0.311791271F,    0.0194967873F,    0.359942168F,
      -0.128081158F,    -0.261490375F,    0.381476551F,     -4.86919089E-5F,
      -0.000201605653F, 0.000196513152F,  0.0588798523F,    0.0338418782F,
      -0.0223606937F,   0.0812725797F,    -0.00958260708F,  -0.00657183817F,
      -0.0850540251F,   0.0454826877F,    0.00527690165F,   0.0214367788F,
      0.103111461F,     0.100884445F,     -0.000108704167F, 0.000209772115F,
      -0.000228089819F, 3.61512648E-5F,   -0.000461667107F, 0.000404061866F,
      -0.204728574F,    1.19009578F,      -1.18698204F,     0.111308232F,
      -0.0422458686F,   -0.00206379173F,  -0.0623704866F,   0.044391159F,
      0.0573304854F,    0.101131953F,     -0.027813198F,    -0.058901757F,
      -0.647892892F,    0.264845222F,     0.182288527F,     -0.0502553359F,
      0.0113283126F,    0.0422547236F,    0.705478907F,     0.331881911F,
      -1.21276009F,     -0.0735484511F,   0.0735206529F,    0.0857536867F,
      -0.187923804F,    0.0875167698F,    0.152866647F,     0.150190949F,
      0.264171481F,     -0.30657959F,     0.00188208802F,   -0.00139914558F,
      0.000144077538F,  0.113094389F,     -0.00261216261F,  -0.025781069F,
      0.0305141974F,    0.0673030391F,    -0.0179598443F,   -0.734919548F,
      1.03924072F,      -0.482397616F,    0.0260498915F,    0.0855019316F,
      -0.10277652F,     0.278559893F,     0.0681780651F,    -0.166207865F,
      0.121414557F,     -0.0179571491F,   -0.075797461F,    0.571920156F,
      0.278841645F,     -1.08576035F,     -0.189660937F,    0.222042963F,
      -0.03184608F,     0.0142237786F,    0.0473338328F,    0.056984216F,
      0.603169322F,     0.365318596F,     -1.04289389F,     0.0176378284F,
      0.00273562851F,   -0.000291650416F, 0.0263348222F,    0.0433731601F,
      0.0569455288F,    0.0144555494F,    -0.0371674225F,   -0.0597714F,
      0.115731552F,     0.0879441649F,    -0.155257508F,    -0.0618656911F,
      -0.0337029211F,   -0.0175396465F,   -0.32976687F,     0.163599178F,
      0.223501861F,     -0.088777937F,    0.013149688F,     0.0384311043F,
      -0.0398391932F,   -0.0591403805F,   -0.0787535235F,   -0.0249722619F,
      -0.282332361F,    0.260487497F,     0.0938145369F,    -0.0433192067F,
      -0.0793823898F,   -0.0597757585F,   0.0376830287F,    0.0862968564F,
      0.284054786F,     -0.160854697F,    -0.138043508F,    -0.00117521232F,
      0.00046175014F,   0.000210775121F,  -0.24404341F,     0.0750361606F,
      0.0983055755F,    0.0550197661F,    -0.0461952575F,   -0.0795339495F,
      -0.0581334718F,   0.0413992107F,    -0.00994035136F,  0.0162769705F,
      -0.0546957329F,   -0.0827825889F,   -0.503438592F,    0.222842455F,
      0.229477331F,     -0.069400534F,    -0.0476446114F,   -0.0481714159F,
      -0.2167961F,      0.049257271F,     0.0957080945F,    0.0403366275F,
      0.0226387978F,    -0.0771635771F,   0.0166972429F,    -0.00938277319F,
      -0.012227593F,    -0.000164745274F, 5.44228933E-5F,   -9.67087E-6F,
      0.040314544F,     0.0692793429F,    -0.0669015422F,   -0.0388383716F,
      0.0556890182F,    0.0743200257F,    0.0514866039F,    -0.0928579F,
      -0.156887814F,    0.000582668814F,  -0.000392655085F, -0.000153240981F,
      -0.673658192F,    -0.318695515F,    1.01219606F,      0.104245745F,
      -0.0163887423F,   -0.0986753106F,   -0.0965564623F,   0.0271260533F,
      0.0812733695F,    0.168985337F,     0.0384676717F,    -0.151781559F,
      -0.704460561F,    -0.355701506F,    1.22724175F,      0.210338891F,
      -0.010440357F,    -0.0535013899F,   -0.18741332F,     0.0973858535F,
      0.0956284478F,    -0.0268621705F,   0.037899591F,     -0.00102181989F,
      -0.000217149951F, 0.000149656436F,  7.66640296E-5F,   0.0271109454F,
      0.0142492028F,    0.00102584064F,   0.0320955887F,    -0.0266867038F,
      -0.0646454543F,   0.036111258F,     0.00408272864F,   0.00391116366F,
      0.717572749F,     0.320826054F,     -1.23400331F,     -0.0784121454F,
      0.0354334153F,    0.0700521693F,    0.0366209298F,    -0.000685016101F,
      0.0111233369F,    0.0647566691F,    0.00286021177F,   -0.00347586931F,
      0.000371168659F,  1.23869704E-5F,   -0.000307855487F, 0.0300111771F,
      0.00374821946F,   0.0130192135F,    0.163204789F,     0.0218390059F,
      -0.193344012F,    0.410065F,        -0.166317627F,    -0.232670605F,
      0.00129024195F,   -0.00105626893F,  9.6931908E-5F,    0.0125242984F,
      0.0106748138F,    -0.0580485202F,   0.0715380833F,    0.0386607423F,
      0.0506493822F,    0.0485222451F,    -0.0729724F,      0.0210845303F,
      8.07656179E-5F,   -0.000215206179F, 0.000229169731F,  0.0795771182F,
      0.0344266519F,    -0.0211059861F,   -0.0131076379F,   0.0457328F,
      -0.0566913933F,   -0.84305644F,     -0.36764431F,     1.22375178F,
      0.0925349072F,    -0.0913005248F,   -0.0699738115F,   0.100971289F,
      -0.101623885F,    -0.0257493909F,   0.0308234356F,    0.113807358F,
      -0.131112844F,    0.342144221F,     -0.0272408444F,   -0.191934466F,
      -0.0177174732F,   -0.0107028056F,   -0.0143437479F,   -0.0107023222F,
      -0.00180856523F,  -0.0050410293F,   0.0152977016F,    -0.0482494384F,
      0.0263554472F,    -0.715217769F,    -0.379558414F,    1.25080645F,
      -0.066454187F,    -0.035986241F,    -0.00571531523F,  0.675622165F,
      0.0209170114F,    -0.515284598F,    0.0645422265F,    0.180363178F,
      -0.224382639F,    0.438318F,        -0.12083035F,     -0.185375318F,
      0.0122521622F,    0.00708936341F,   -0.022736406F,    0.00746905524F,
      -0.0244964752F,   0.0174196567F,    0.010602477F,     0.023571698F,
      -0.00836411212F,  -0.246988356F,    0.0909586176F,    0.0560376F,
      -0.50691855F,     -0.381806225F,    1.07133389F,      0.290391058F,
      -0.052541405F,    -0.203004032F,    -0.169116259F,    -0.147849023F,
      0.319286078F,     0.0883288682F,    -0.0145479441F,   -0.0541032627F,
      -0.00567818619F,  -0.0162139665F,   0.0521662049F,    -0.0459707975F,
      0.18963775F,      -0.132470369F,    -0.115600713F,    0.0386183485F,
      -0.0495529249F,   -0.000463845732F, 7.07569197E-5F,   0.000119566961F,
      0.0275602639F,    0.0179720633F,    -0.0505289845F,   -0.0420723483F,
      -0.0263366718F,   -0.0209046528F,   0.476318538F,     0.269101769F,
      -0.92183876F,     0.000209588412F,  0.000352240982F,  -0.000551821722F,
      0.0430607423F,    0.00390610774F,   -0.00705087045F,  0.0302441269F,
      0.000544078881F,  -0.0259381272F,   -0.396609426F,    -0.392353714F,
      0.847975075F,     0.0522582233F,    0.00534800487F,   -0.0482308641F,
      -0.000172466622F, 0.000430793676F,  -0.000244421914F, -0.0397230238F,
      -0.00893396F,     0.0382047929F,    0.0749669299F,    0.0573952869F,
      0.0695048273F,    0.00022128511F,   -4.26229599E-5F,  -7.32342742E-5F,
      0.0698986873F,    0.0655535758F,    0.0846461132F,    0.0474063419F,
      -0.114313118F,    0.0757681727F,    -0.0911709294F,   -0.0202360563F,
      0.0297349989F,    -0.0257139374F,   -0.0361089706F,   0.0179765578F,
      0.0728144571F,    0.0939310491F,    0.114781089F,     -0.0732131451F,
      -0.0626562387F,   0.0485F,          -0.853588164F,    -0.410196275F,
      1.37791836F,      0.686111569F,     0.0440415405F,    -0.72017F,
      -0.0562374182F,   -0.0704142377F,   -0.0575962216F,   -0.30310604F,
      0.158152819F,     0.16432929F,      0.559832156F,     -0.224236578F,
      -0.225601092F,    -0.68752557F,     -0.372567177F,    1.18023872F,
      -0.520288467F,    -0.618491709F,    1.4398421F,       0.0170276631F,
      -0.0223206859F,   -0.0371255316F,   0.391462624F,     1.10421729F,
      -1.56560814F,     -0.246352255F,    0.0327998474F,    0.0919773877F,
      -0.103988022F,    -0.053406544F,    0.0711082816F,    -0.469975293F,
      -1.07484F,        1.79736114F,      0.0151338894F,    0.0253107082F,
      0.0723376945F,    0.0137910917F,    0.0186722968F,    -0.0475886837F,
      -0.250046313F,    0.0876212F,       0.115161665F,     0.137457311F,
      0.0777565613F,    0.0728609636F,    0.341412276F,     0.467387259F,
      -0.953884363F,    0.0119764861F,    0.00321631902F,   -0.0422189683F,
      0.295392901F,     -0.190665796F,    -0.0620150864F,   -0.0366062298F,
      -0.0478814766F,   -0.0460813381F,   0.0152617525F,    -0.0352801792F,
      0.00479182322F,   0.0263294335F,    -0.0417780653F,   -0.0384896956F,
      -0.0626572818F,   -0.0505435355F,   -0.0580558442F,   -0.0360956639F,
      -0.0170971137F,   -0.0275967587F,   0.0331380963F,    -0.0225325134F,
      -0.0585300215F,   -1.05620515F,     1.46275258F,      -0.692140877F,
      0.0075182179F,    0.00791792758F,   0.00844302215F,   -0.00139700982F,
      -0.000396846328F, -0.000189000333F, 0.0173354466F,    0.0211281944F,
      -0.0426892F,      0.0385014229F,    -0.0177992713F,   -0.0592359938F,
      -0.115884006F,    -0.0606935844F,   -0.0710758F,      -0.105612881F,
      0.039330516F,     0.130406916F,     -0.644341826F,    -0.0612633415F,
      0.711913F,        -0.00150478922F,  0.00602046261F,   -0.11093422F,
      -0.176470473F,    0.0838371F,       0.0627869889F,    -0.0189049393F,
      0.0486989841F,    0.0837780088F,    0.129290417F,     -0.0488707162F,
      -0.036008466F,    0.0828831F,       0.0281706173F,    -0.0185911376F,
      0.0429373793F,    0.00567406323F,   -0.0543261506F,   0.000136407732F,
      4.44588586E-5F,   -0.000176853486F, 0.000535788829F,  -0.000367617118F,
      5.60117915E-5F,   -0.000253974576F, 0.000253256876F,  -0.000100787809F,
      0.241652131F,     -0.140328541F,    -0.133708298F,    0.180344105F,
      0.308815122F,     -0.413556725F,    -0.000305484835F, -5.12668812E-5F,
      0.000237205182F,  0.0380960517F,    0.00541108754F,   -0.0340177119F,
      -0.0208717156F,   -0.00342592061F,  0.0084836632F,    -0.0663045198F,
      -0.0305126291F,   0.112125F,        -0.445950031F,    -0.111620493F,
      0.58953011F,      -0.000212072686F, 0.0005435606F,    -0.000485271128F,
      0.727540731F,     0.313749492F,     -1.22549856F,     0.000466902362F,
      -0.000679903664F, 0.000373286894F,  -0.00059999F,     0.000305893365F,
      -1.71834472E-5F,  -0.00347050466F,  -0.00238134246F,  0.00429680943F,
      0.561529279F,     0.205282032F,     -0.830006957F,    -0.0115728024F,
      -0.0235563926F,   -0.000744821737F, -0.000951747177F, -3.06442416E-5F,
      0.000840021879F,  -0.0181245934F,   -0.0320719257F,   -0.0576784723F,
      -0.118108042F,    -0.00142924837F,  0.0429231785F,    -0.0125907455F,
      -0.0610669553F,   0.127179638F,     -0.0460905656F,   -0.0191006754F,
      -0.0152855292F,   0.0201769304F,    0.0313777886F,    -0.0997237414F,
      -0.358287543F,    0.174827367F,     0.224974856F,     -0.0630359277F,
      -0.00190154067F,  0.0368576795F,    0.00583004626F,   -0.02432926F,
      -0.0769463554F,   0.0207987726F,    0.0208918341F,    0.03413333F,
      -0.0971897691F,   -0.045064941F,    -0.0497033782F,   -0.0967248F,
      0.0838195756F,    0.0733165592F,    0.00578897214F,   0.000160839976F,
      -0.000296271843F, -0.0318355076F,   0.0769134164F,    0.068842113F,
      0.613938034F,     0.315026581F,     -1.05547535F,     0.0197591223F,
      -0.235806271F,    0.201305151F,     -0.182288915F,    -0.00370216835F,
      0.163586944F,     0.0587238297F,    0.0876326784F,    0.0374986157F};
  static const float gateBias[1200] = {
      -0.0790321752F,  0.0141005274F,    -0.168436319F,   0.42600888F,
      -1.12509298F,    -0.202866241F,    -1.14119339F,    0.614409745F,
      0.554537177F,    -0.00118859462F,  -0.025481157F,   0.0731275529F,
      0.089318715F,    0.240239039F,     0.387902975F,    0.203273118F,
      0.423479438F,    -0.287737697F,    0.282625914F,    0.256284922F,
      0.593569219F,    0.368146181F,     0.0846514553F,   0.0757937953F,
      0.0450351909F,   0.0590400845F,    -0.0360485725F,  0.0438035242F,
      0.265440136F,    0.11768014F,      -0.167984411F,   0.608572483F,
      0.20700106F,     0.343822896F,     0.132332936F,    -0.0867883712F,
      -0.501745403F,   -0.641356349F,    -0.079237543F,   0.0619565956F,
      0.471312553F,    0.0809629F,       0.0432426967F,   -0.0582616925F,
      0.25622F,        -0.0210454315F,   0.41972959F,     0.98646F,
      0.149820119F,    0.231268063F,     -0.137458131F,   0.927105248F,
      -0.552180529F,   0.252653331F,     -0.234470218F,   0.0199261978F,
      -0.123127334F,   -0.0605359934F,   0.044449538F,    -0.207410291F,
      0.807678223F,    0.060520798F,     0.0251374412F,   -0.217038423F,
      0.0672033057F,   1.09128702F,      -0.0162570737F,  0.18959108F,
      -0.0274123885F,  -0.17459254F,     0.310041904F,    -0.123127118F,
      -0.360046029F,   1.03480518F,      0.300535F,       -0.887191534F,
      1.21458721F,     0.0322799683F,    0.0423767082F,   -0.653758764F,
      1.27191389F,     0.0541249588F,    -0.315187484F,   -0.206606403F,
      0.0538008846F,   -0.0582740121F,   0.0980005786F,   -0.287911594F,
      0.611212075F,    0.163904727F,     -0.000685226F,   -0.551142514F,
      0.0423555449F,   0.624259889F,     0.108532041F,    0.666205585F,
      0.139417261F,    0.174536929F,     0.292212F,       0.144097105F,
      -0.0585080571F,  0.115652993F,     -0.148829892F,   0.458797097F,
      0.0247824676F,   0.00786283705F,   0.184117272F,    0.254439592F,
      0.041556567F,    -0.28951925F,     0.0275634304F,   0.0528764911F,
      0.201193184F,    0.28944388F,      -0.0719915628F,  -0.292362154F,
      0.213642985F,    0.531683803F,     -1.21947765F,    -0.873364925F,
      0.0227244068F,   0.780486643F,     0.0651040673F,   -0.127787054F,
      -0.443174481F,   -0.273549706F,    0.195082143F,    -0.0675825477F,
      -0.159140661F,   -0.172833189F,    0.799283862F,    0.176939428F,
      0.907233715F,    -0.301915675F,    0.113045186F,    -0.0621651337F,
      -0.236542046F,   0.296142399F,     0.615756869F,    0.133467883F,
      0.207525149F,    0.117558591F,     -0.0110919531F,  0.426760495F,
      0.0190624204F,   0.29264459F,      0.357200086F,    0.344687223F,
      0.323282033F,    -0.00492559699F,  0.281639457F,    -0.23315005F,
      0.240047127F,    -0.250667483F,    0.0068882457F,   1.42002773F,
      -0.260693282F,   0.443476886F,     0.236076891F,    0.273177058F,
      0.0220468231F,   -0.104886211F,    -0.43490544F,    -0.481044054F,
      0.320485026F,    -0.0260774083F,   0.278478831F,    0.0499890447F,
      0.0524972714F,   0.189473912F,     -0.00536097F,    0.506318867F,
      -0.18701452F,    0.327275217F,     -0.096524775F,   0.181827068F,
      0.883208513F,    0.179616362F,     0.148421556F,    0.572085381F,
      0.269627035F,    -0.0410539247F,   0.025895793F,    0.542962372F,
      0.0446818732F,   0.213866308F,     -0.117382325F,   0.110360608F,
      -0.0342725553F,  0.181487009F,     0.1902031F,      0.364336282F,
      0.23796618F,     0.169547185F,     0.410188884F,    -0.128759235F,
      0.27187711F,     -0.0673881322F,   -0.745250106F,   0.479578435F,
      0.265242726F,    -0.668425143F,    0.03577701F,     0.0470794216F,
      0.106663905F,    0.193006918F,     -1.00490487F,    -0.756424069F,
      0.0196593162F,   0.00369984098F,   0.0298457574F,   0.0307789035F,
      0.239761755F,    0.816684544F,     0.571181178F,    -0.129732236F,
      0.131058231F,    0.593863308F,     -0.749651194F,   0.0512646809F,
      0.102341473F,    0.313887447F,     0.106204689F,    0.185676977F,
      0.00627188291F,  0.965909898F,     -0.191527039F,   0.201669902F,
      0.464972973F,    -0.0721799F,      0.301421672F,    0.157233551F,
      0.38152048F,     0.113625847F,     0.383909732F,    0.186366409F,
      -0.1181679F,     0.536965251F,     0.0159919336F,   0.104253471F,
      0.150566086F,    -0.539137244F,    0.196617365F,    0.0410451144F,
      0.0678505525F,   -0.0123571688F,   0.237185046F,    0.533912361F,
      0.215003639F,    0.263886422F,     -0.0776872262F,  -0.212334603F,
      0.00919762347F,  0.224880889F,     0.430611134F,    -0.478675336F,
      0.324978173F,    -0.0377874821F,   0.129552335F,    0.0887583F,
      0.476260036F,    0.394958436F,     -0.169663414F,   0.0538865887F,
      -0.699017525F,   0.0162888747F,    -0.00726912636F, -0.0247627869F,
      0.517477632F,    0.162742212F,     0.0872455239F,   0.0337559208F,
      -0.843918264F,   -0.117661439F,    0.154303789F,    0.20260407F,
      -0.335387F,      0.155367181F,     -0.00546656549F, 0.0303015765F,
      -1.01600599F,    -0.233078092F,    0.0712637305F,   0.97761184F,
      0.0239913557F,   0.0438982509F,    0.307702303F,    0.101171777F,
      0.121292956F,    -0.180446208F,    -0.0305901449F,  0.391886979F,
      -0.076988481F,   1.0480305F,       0.468116F,       0.113771506F,
      -0.0411241539F,  -0.112057067F,    -0.143320605F,   0.182067946F,
      0.245718613F,    0.0300512668F,    0.351083666F,    0.431001F,
      -0.171561241F,   0.14660953F,      0.105844803F,    -0.380426F,
      0.408160567F,    0.0572711229F,    0.383335799F,    -1.04496217F,
      -0.0723926798F,  1.09851718F,      0.448274285F,    0.0814931616F,
      -0.638614297F,   -0.0334848873F,   -0.100831456F,   -0.929008603F,
      0.0316732079F,   0.0701776296F,    0.0787612721F,   0.0272244737F,
      1.57695043F,     0.0954825357F,    0.450734764F,    0.755732596F,
      -0.122160353F,   0.317746639F,     0.47026062F,     0.46159181F,
      0.870429397F,    0.198976234F,     1.09287667F,     0.0687557608F,
      -0.143724337F,   0.939091206F,     0.244067773F,    0.12680535F,
      0.19328472F,     0.614573777F,     0.2628F,         0.0826464593F,
      0.170520276F,    0.121072993F,     -0.122422703F,   0.758471847F,
      -0.00324818282F, -0.0144281397F,   0.233541638F,    0.667597055F,
      -0.298393726F,   -0.294388026F,    0.647620618F,    0.0876321122F,
      0.261276782F,    0.109275676F,     1.34051156F,     0.136613116F,
      0.0551797F,      0.0516299792F,    0.092758365F,    0.074312523F,
      -0.0423643477F,  -0.833191276F,    -0.726633251F,   -0.280544251F,
      -0.0454872735F,  0.462711304F,     -0.892533302F,   0.0755794272F,
      -0.116437323F,   0.0388766192F,    0.761632144F,    -1.03178656F,
      0.895898879F,    -0.843708813F,    -0.580566347F,   -0.0448042825F,
      0.402565092F,    -0.080034F,       -1.35243F,       0.18723543F,
      0.212126791F,    0.172991082F,     0.10638196F,     -0.0416163318F,
      -0.0177275334F,  -0.0629761145F,   -0.339087427F,   0.111292377F,
      0.150141731F,    0.2212843F,       -0.241489694F,   0.0697369948F,
      0.432065278F,    0.902476668F,     0.45992893F,     -0.091967456F,
      0.960106432F,    0.95705086F,      0.906095564F,    0.862216532F,
      0.838277578F,    0.838413179F,     0.841836154F,    0.90588F,
      0.951579332F,    0.928959608F,     0.866790235F,    0.961853147F,
      1.01845801F,     1.16119075F,      0.932738423F,    0.948005855F,
      1.10169208F,     0.885723829F,     0.857056499F,    1.00312078F,
      0.886824727F,    0.932410061F,     0.955854595F,    0.932296634F,
      1.01833761F,     1.04283249F,      0.921210527F,    1.02764297F,
      0.874603093F,    0.981151938F,     0.984685659F,    0.611126304F,
      1.15482044F,     0.825549066F,     0.861283183F,    0.852577F,
      0.995672047F,    0.92314297F,      1.0191257F,      0.90457195F,
      0.949436665F,    0.886380613F,     1.05650854F,     0.895279288F,
      0.93153F,        0.886161625F,     0.747809887F,    1.11702275F,
      0.947779F,       0.902131617F,     0.944754481F,    1.31214964F,
      0.837353408F,    0.941805482F,     0.955858111F,    1.02226329F,
      1.04616916F,     0.988247335F,     0.892672181F,    0.909467518F,
      1.12927473F,     0.951045F,        0.939490795F,    0.894724309F,
      0.988688827F,    0.935397923F,     0.951995075F,    0.914486885F,
      0.900701463F,    0.977878034F,     1.0779953F,      0.816423237F,
      0.882923484F,    0.946516395F,     0.95207274F,     0.816074967F,
      0.974269688F,    1.02908039F,      0.911354661F,    0.897592962F,
      0.791170895F,    0.938192666F,     1.0192275F,      0.838884652F,
      0.821765661F,    0.921384513F,     1.078933F,       0.932317436F,
      0.812781096F,    0.956124067F,     1.09867978F,     0.993680894F,
      0.88465637F,     1.14997542F,      1.00909936F,     1.11557245F,
      1.07398605F,     1.0068115F,       0.888974F,       0.943429828F,
      0.807707F,       1.02125907F,      0.876318932F,    1.23647618F,
      1.01773083F,     0.902329266F,     0.904016614F,    1.17850256F,
      0.997405946F,    0.90304F,         0.988996148F,    0.914558589F,
      0.934586883F,    1.0528729F,       1.09696317F,     0.936310649F,
      0.987189531F,    0.870045F,        0.806435823F,    0.76756984F,
      1.07333F,        0.959156394F,     0.961848497F,    0.958821F,
      0.874361575F,    0.825384F,        0.863994539F,    1.03957558F,
      1.00066757F,     0.915043116F,     1.34348786F,     0.863556087F,
      0.896915F,       0.887526035F,     0.851264894F,    0.891592205F,
      0.89653486F,     1.03889F,         0.792457104F,    0.880553365F,
      1.2232542F,      0.994815528F,     0.937377751F,    1.11965537F,
      0.894897044F,    0.962495446F,     0.952475429F,    0.58386606F,
      0.922640204F,    0.975831389F,     0.941857517F,    0.878302872F,
      0.940841496F,    0.884394348F,     1.04053366F,     1.03731406F,
      0.880948305F,    0.91936481F,      0.904538393F,    0.862465441F,
      0.958227694F,    0.909650862F,     0.885100424F,    0.977844298F,
      0.792992413F,    0.945910573F,     0.957345247F,    0.957394063F,
      0.975794435F,    0.85384357F,      0.861218333F,    1.10162437F,
      0.923621595F,    0.910913348F,     0.790857136F,    1.04447424F,
      1.10293114F,     0.842267F,        1.07985198F,     0.832337081F,
      0.935829401F,    0.851196289F,     0.974302471F,    0.794970095F,
      1.00791264F,     0.9198066F,       0.919773638F,    0.848312438F,
      0.910352F,       1.03683543F,      0.937734485F,    0.822969556F,
      1.00051F,        1.08976102F,      0.952739716F,    0.943589389F,
      0.870387733F,    0.983715236F,     0.825886607F,    0.888743222F,
      0.993087947F,    0.863982856F,     0.886427104F,    0.931790948F,
      0.889108F,       0.932529867F,     0.823124349F,    0.874493718F,
      0.981136F,       1.01564848F,      0.922092617F,    0.96847558F,
      0.875515759F,    0.743196309F,     0.988799036F,    0.790688753F,
      1.18602753F,     1.11198342F,      0.796592236F,    0.941343248F,
      0.946298182F,    0.976827502F,     1.07966626F,     0.934223413F,
      0.928446651F,    0.946527183F,     0.897596717F,    0.802042663F,
      0.873630047F,    0.91239506F,      0.816420317F,    0.788462102F,
      1.12612069F,     0.914227843F,     0.929764628F,    0.916368842F,
      0.973728597F,    1.06681824F,      0.975928366F,    0.955243707F,
      0.859362125F,    0.879903376F,     0.961439133F,    0.946296275F,
      0.939440846F,    0.997548878F,     1.11342943F,     0.890396416F,
      1.01475048F,     1.03219914F,      0.860998809F,    1.00736594F,
      0.918809056F,    0.799307346F,     1.04100585F,     0.862045169F,
      0.97694695F,     0.938678324F,     0.912828505F,    1.10996091F,
      0.895494401F,    0.948866785F,     1.02391601F,     1.04304898F,
      0.900392771F,    0.887809396F,     0.911795676F,    0.883735061F,
      0.818106115F,    0.950095296F,     0.844420612F,    0.951589763F,
      0.837153F,       0.923431933F,     0.96109575F,     0.968134582F,
      0.901893318F,    0.942753553F,     0.902131F,       1.06391358F,
      0.84443748F,     0.915730059F,     0.913780749F,    1.19514847F,
      0.876676142F,    0.903627276F,     0.833706379F,    1.02244067F,
      0.833423316F,    0.99124974F,      0.946102142F,    0.829581738F,
      0.889579713F,    0.984988868F,     1.05351472F,     1.09377897F,
      0.972744703F,    0.982434332F,     0.976970196F,    0.938302517F,
      0.908320069F,    0.859108F,        1.03429747F,     0.711908102F,
      0.888820171F,    1.01242685F,      1.02280891F,     0.956657708F,
      0.927860498F,    0.94312048F,      0.894460857F,    0.87084806F,
      1.10599053F,     0.736487269F,     1.01162398F,     1.08566368F,
      0.805399597F,    0.973915339F,     0.927041888F,    0.814889133F,
      0.975278676F,    0.985330045F,     0.931632459F,    0.965500355F,
      0.787575662F,    0.949271202F,     0.912249506F,    1.0062815F,
      0.781042695F,    0.919925F,        1.34912336F,     0.868786216F,
      0.807931423F,    0.872944534F,     1.19928968F,     1.0907F,
      0.945064723F,    1.04022622F,      0.932594061F,    1.0306921F,
      0.991927266F,    0.88299942F,      1.03824246F,     0.946053922F,
      0.919889271F,    0.92087F,         0.910266817F,    0.716958523F,
      0.898022F,       0.937896967F,     0.809169233F,    0.948136151F,
      0.927504539F,    0.912947118F,     1.12790394F,     0.931124091F,
      0.918827534F,    0.989981115F,     0.971396923F,    1.05399966F,
      0.801212907F,    0.904776156F,     0.896874726F,    0.94449842F,
      1.03472149F,     0.845925093F,     0.899981499F,    0.93438983F,
      1.09838152F,     1.1159848F,       0.82390964F,     0.852247834F,
      0.85578835F,     0.945648372F,     0.90151608F,     0.835381925F,
      0.822595716F,    0.833830357F,     0.890754282F,    0.924410284F,
      0.898759425F,    0.92266947F,      0.8172068F,      0.90121007F,
      0.896629691F,    1.07547271F,      0.838228285F,    0.983668447F,
      0.824096799F,    0.95225656F,      0.949434519F,    0.711838961F,
      0.844419837F,    0.859657466F,     1.01489687F,     0.89432621F,
      0.887916386F,    1.27266765F,      1.05784082F,     0.990758657F,
      -0.167772755F,   0.050235074F,     -0.182471573F,   0.255905837F,
      -1.14730823F,    -0.0842738822F,   -1.03656602F,    0.123318754F,
      0.339924723F,    -0.0026557215F,   -0.070375F,      -0.0920568332F,
      -0.0654447824F,  -0.131522834F,    0.260476351F,    0.126228407F,
      0.339513779F,    -0.300448F,       0.158196121F,    0.382495522F,
      0.193165556F,    0.806875587F,     0.178576082F,    0.0144095067F,
      -0.037641231F,   0.0218072813F,    -0.00868367683F, 0.0364467278F,
      0.238487408F,    0.087493211F,     -0.161503449F,   0.343386382F,
      0.126014203F,    0.191358849F,     0.176195055F,    -0.0542749129F,
      -0.54238385F,    -0.592496514F,    0.0254813917F,   0.188626423F,
      0.397057682F,    -0.00666273F,     -0.00556852063F, -0.0171657205F,
      0.223989606F,    0.219359845F,     0.0154556334F,   0.362697124F,
      0.0952301F,      0.740374923F,     -0.141456574F,   0.144229382F,
      -0.607324183F,   0.286981434F,     -0.247633547F,   0.0438861884F,
      -0.13466759F,    -0.0433421545F,   0.0522859842F,   -0.23907198F,
      0.316515237F,    -0.0154936984F,   -0.0414051376F,  -0.27192995F,
      0.0350574441F,   0.510870039F,     0.109756567F,    0.0922761038F,
      -0.0134260729F,  0.0174043346F,    0.164631903F,    -0.243160501F,
      -0.312000573F,   0.605097473F,     0.455679864F,    -0.882435322F,
      0.934483886F,    -0.00816096086F,  -0.0772722214F,  -0.674862146F,
      1.06302917F,     0.0447643846F,    0.00883842073F,  -0.22373125F,
      0.0165478699F,   -0.0734626874F,   0.0711589903F,   -0.396961391F,
      0.318187267F,    0.0899260193F,    0.144247219F,    -0.629972637F,
      -0.025148673F,   0.275706649F,     0.0790922716F,   0.183798552F,
      -0.0472177081F,  -0.0893307552F,   0.406946838F,    0.257619649F,
      -0.0207007807F,  0.305288017F,     -0.211484686F,   0.410077691F,
      0.0472648665F,   -0.00231603067F,  0.103438996F,    0.189004093F,
      0.0370718911F,   -0.341873139F,    0.0161695685F,   0.021563299F,
      0.199952647F,    0.0232190508F,    -0.119898282F,   0.424328893F,
      0.0492243022F,   0.498944342F,     -1.23189604F,    -0.877648532F,
      -0.0152666904F,  0.166068301F,     0.0438912846F,   -0.147943079F,
      -0.476065606F,   -0.292126954F,    0.325056404F,    -0.168407544F,
      0.115267567F,    0.0397179723F,    0.22261934F,     0.049247805F,
      0.783859909F,    -0.251935422F,    0.212176427F,    0.00393313775F,
      -0.299771816F,   0.0272866692F,    0.343088835F,    0.164154038F,
      0.087906979F,    0.0126287155F,    0.175418049F,    0.298124939F,
      -0.00495501701F, 0.239099145F,     0.366405696F,    0.155251339F,
      0.448239356F,    -0.052816458F,    0.217681095F,    -0.442730039F,
      0.0837830752F,   -0.213978693F,    0.00548931351F,  0.992679298F,
      -0.388364226F,   0.111932345F,     -0.0757898465F,  0.0973213464F,
      0.0992756486F,   -0.208624184F,    -0.432575375F,   -0.464167267F,
      0.292214692F,    -0.0629993156F,   0.001853019F,    0.0242606178F,
      0.311009735F,    -0.0235452596F,   0.104749836F,    0.375994235F,
      -0.17243439F,    0.298072755F,     -0.108441971F,   0.214614198F,
      0.700758398F,    0.0454863869F,    0.0224057715F,   0.0829284117F,
      0.0996522382F,   0.0405012853F,    -0.0600756891F,  0.146571219F,
      0.0901494101F,   0.035325937F,     -0.118204825F,   0.375635564F,
      -0.0814069733F,  -0.00332949078F,  0.308853209F,    0.302655548F,
      0.160925552F,    0.121465862F,     1.00559294F,     -0.137891531F,
      0.298918128F,    0.669958293F,     -0.636432052F,   0.295713603F,
      0.271575153F,    -0.759487391F,    0.00763507467F,  -0.0135729909F,
      0.0766565874F,   0.603044629F,     -1.02650011F,    -0.754618883F,
      0.424126506F,    -0.174037144F,    -0.0565970764F,  -0.0607551113F,
      0.190213293F,    0.135043904F,     0.410972089F,    -0.221425638F,
      0.146402657F,    0.420819491F,     -0.795169413F,   0.00960396696F,
      0.0299989935F,   0.369756758F,     0.112863168F,    0.307549208F,
      0.00269142888F,  0.342275977F,     -0.154358774F,   0.0416299403F,
      0.131328806F,    -0.0654349551F,   0.0817687437F,   0.0188052822F,
      0.143744573F,    0.0651056617F,    0.0413832739F,   0.125345707F,
      -0.137600631F,   0.220464647F,     -0.014726745F,   0.0250087F,
      0.265958875F,    -0.515117764F,    0.249486431F,    0.0198834818F,
      -0.110331178F,   -0.17954427F,     0.24717088F,     0.242833853F,
      0.348093867F,    0.133874446F,     -0.114583679F,   -0.269770443F,
      0.038708698F,    0.0348364376F,    0.177008674F,    -0.526227355F,
      0.416508347F,    -0.180069461F,    0.084044151F,    0.0836647898F,
      0.160341933F,    0.40371111F,      0.165471241F,    -0.0505398698F,
      -0.658738792F,   0.0645545721F,    -0.0946731716F,  -0.0326265432F,
      0.36973983F,     0.294985473F,     -0.00370010664F, 0.0246887114F,
      -0.838050127F,   -0.0516633913F,   0.149742395F,    -0.056687396F,
      -0.495754272F,   -0.0100054611F,   -0.0424069F,     0.00323242205F,
      -1.02988625F,    -0.28956449F,     0.00470431335F,  0.449456394F,
      0.0561158694F,   0.278410912F,     0.21559003F,     0.0871143565F,
      0.0107088769F,   -0.27044785F,     -0.0373626947F,  0.243187696F,
      -0.144917339F,   0.697903275F,     0.199041575F,    0.0571259558F,
      -0.0248904899F,  -0.0796185434F,   -0.179429024F,   1.14135873F,
      -0.00845696777F, 0.035143882F,     0.379369944F,    0.100087948F,
      -0.186802208F,   -0.0167249795F,   0.0877677798F,   -0.433974057F,
      -0.0216692053F,  0.0153705031F,    0.109474711F,    -1.06668437F,
      -0.100437112F,   0.237491429F,     0.0619618334F,   0.0467277579F,
      -0.629076183F,   -0.0784472078F,   -0.113414116F,   -0.943143189F,
      -0.0082419645F,  0.034004733F,     0.0352855213F,   -0.0158829484F,
      1.58103681F,     -0.0279034413F,   0.2684834F,      0.689715683F,
      -0.112793848F,   0.237669602F,     0.7017501F,      0.203925282F,
      -0.132763132F,   -0.055551976F,    0.619030952F,    0.19201827F,
      -0.0371397696F,  0.0821021497F,    0.102402188F,    0.0453967378F,
      -0.0895129144F,  0.331370622F,     0.0451294556F,   -0.0976474434F,
      0.368125647F,    0.0430431142F,    -0.0913531706F,  0.237670392F,
      -0.0114337066F,  0.00852338411F,   0.0274815056F,   0.392632097F,
      -0.373556346F,   -0.286817968F,    0.201574415F,    0.0470800735F,
      0.885477245F,    -0.000748385617F, 1.19005048F,     0.0609361678F,
      -0.124743387F,   0.0505551286F,    0.0660053492F,   0.0200117268F,
      -0.00299887802F, -0.900257945F,    -0.71710217F,    -0.269961715F,
      -0.0234408565F,  0.501320124F,     -0.992201328F,   -0.00535441097F,
      -0.108619191F,   0.104421228F,     0.680802524F,    -1.04835451F,
      0.702401638F,    -0.875879943F,    -0.578085601F,   -0.0372556187F,
      0.430793166F,    -0.184866309F,    -1.37217891F,    0.00999766868F,
      0.136564702F,    0.0779872462F,    0.00922749098F,  -0.138511553F,
      0.188112214F,    -0.239430279F,    -0.371088415F,   -0.03123625F,
      0.526792347F,    0.76443857F,      -0.250765204F,   0.103226878F,
      0.236611277F,    0.22589241F,      0.169456646F,    -0.120205782F};
  static const float stateBias[400] = {
      0.104048461F,     -0.0761412829F,   -0.232239112F,    0.342873931F,
      -0.000975009636F, 0.0688102916F,    6.49523718E-5F,   0.235507011F,
      0.00195251068F,   -0.0467984937F,   0.114465274F,     0.198269293F,
      -0.176635459F,    -0.045360323F,    0.0619256981F,    0.138484374F,
      -0.204644069F,    -0.000131502631F, -0.169491932F,    0.024638949F,
      0.31261465F,      -0.409777433F,    0.0474973693F,    0.0883291364F,
      -0.0524479039F,   -0.241572052F,    0.166343912F,     0.108027F,
      0.281149328F,     0.00193988939F,   0.00192181347F,   -0.104164347F,
      -0.260359794F,    -0.125209764F,    0.060414806F,     -0.295993328F,
      0.000788049889F,  0.0015770559F,    -0.0121091167F,   -0.341339886F,
      0.0106629916F,    0.2090092F,       0.1998339F,       -0.348827511F,
      -0.0582826361F,   0.0810302496F,    -0.0843943879F,   0.0365539938F,
      0.002454784F,     0.0334644243F,    -0.046164535F,    0.362081F,
      -0.00186490617F,  -0.0360834487F,   -0.0780937895F,   -0.0807999894F,
      -0.100676239F,    0.0278875977F,    -0.0544118211F,   -0.000133354653F,
      0.195994377F,     -0.195237935F,    0.289029479F,     -0.0315730609F,
      -0.200734362F,    0.0255534314F,    -0.010702976F,    -0.00735088F,
      0.366673321F,     0.156369165F,     0.215092152F,     0.00175695273F,
      0.100393809F,     -0.0264283083F,   0.0529408716F,    0.00145998783F,
      0.0107766083F,    -0.0484384596F,   -0.0503902175F,   0.000333679782F,
      -0.0186391026F,   -0.253929257F,    -0.130302072F,    0.0592749901F,
      0.0387604497F,    0.199462876F,     0.0733283609F,    0.00145081338F,
      0.0446812324F,    -0.191940784F,    -0.107470654F,    -0.00021448062F,
      -0.030237183F,    0.228780866F,     0.28475666F,      0.309271425F,
      0.0225542765F,    -0.0156757757F,   -0.00951651763F,  -0.0805095807F,
      -0.164557561F,    -0.181319311F,    0.0974652916F,    -0.00448253378F,
      0.224103391F,     0.139196739F,     0.0586323F,       0.0308150351F,
      -0.0810154751F,   -0.0140792085F,   0.0592546F,       0.0574058518F,
      0.136019513F,     0.0508504435F,    -0.0296914335F,   -0.0477596335F,
      -0.141964048F,    0.0503985882F,    0.000686414714F,  -0.000302809261F,
      0.185021147F,     -0.366702795F,    0.235790506F,     -0.0678399056F,
      7.06804567E-5F,   -0.000634692493F, 0.00937621854F,   -0.0586204F,
      0.0935753658F,    0.174699157F,     0.365818262F,     -0.0249194298F,
      0.0223717056F,    -0.000792799F,    0.0630756393F,    0.262100905F,
      -0.0698117241F,   0.0429682136F,    0.137910321F,     0.0368783809F,
      -0.0138219493F,   -0.296291232F,    -0.068014361F,    -0.0309682F,
      -0.101011641F,    0.135358408F,     -0.154723376F,    0.176287726F,
      0.204112947F,     -0.17271997F,     -0.19276154F,     -0.000440017058F,
      0.0241195504F,    0.179854557F,     0.191054314F,     0.0077241594F,
      0.00120672875F,   -0.241553053F,    -0.000165931255F, -0.0569644123F,
      0.0066308328F,    0.00136826257F,   -0.00188742043F,  0.000355802884F,
      -0.101741552F,    0.198630944F,     0.0573820919F,    -0.121685639F,
      0.0195783861F,    0.138951018F,     0.1036103F,       0.18656221F,
      -0.105236866F,    -0.167946443F,    0.210929945F,     0.0347344205F,
      -0.0302785169F,   0.0626201108F,    -0.221739829F,    -0.191136792F,
      0.00916037802F,   0.107348733F,     -0.0398608483F,   -0.266649395F,
      -0.0781506747F,   0.208947822F,     -0.0718607903F,   -0.0786139518F,
      0.0223614536F,    0.141084194F,     0.0985953063F,    0.180785432F,
      -0.0424256F,      0.247841164F,     -0.122325569F,    -0.137574449F,
      0.030968355F,     -0.0478197F,      -0.00167171541F,  0.0422613F,
      -0.164235353F,    -0.000563430309F, 0.139033452F,     -0.0461723544F,
      0.124062292F,     -0.0812684372F,   -0.000609176699F, -7.73743232E-5F,
      -0.0145842396F,   -0.101897255F,    -0.0906750187F,   0.00557193579F,
      0.0440317802F,    -0.13561365F,     -0.0371860564F,   0.0713308454F,
      -0.0245366655F,   0.21939148F,      0.000366997352F,  0.0704934821F,
      0.161902457F,     -0.0489316285F,   0.276109338F,     -0.0361038037F,
      0.12856473F,      -0.0331462063F,   0.0518835634F,    -0.00816147588F,
      -0.0544778369F,   -0.130084008F,    -0.122095011F,    0.0455697514F,
      0.219196469F,     -0.0915518478F,   0.113890693F,     -0.125071719F,
      -0.209175214F,    -0.218215629F,    0.0545807108F,    -0.0998134688F,
      -0.0598984696F,   0.00135708868F,   -0.0995640382F,   0.0616916902F,
      0.096117489F,     -0.0947800279F,   0.00764789525F,   -0.288739026F,
      -0.123744018F,    0.324823141F,     0.359524429F,     0.000430202286F,
      0.234553903F,     -0.149082959F,    0.163982853F,     0.000249010365F,
      0.03981198F,      -0.0781321F,      -0.0566770025F,   -0.0312611461F,
      0.0571559295F,    0.184633493F,     -0.145166546F,    0.122505181F,
      -0.00142252783F,  0.282994241F,     -0.057230588F,    0.385203928F,
      -0.0372500829F,   -0.107206799F,    0.0726640224F,    0.0457907617F,
      -0.000274225575F, 0.178014472F,     -0.056240689F,    -0.0533011444F,
      0.00135842164F,   0.173348725F,     0.216118991F,     -0.174579456F,
      0.000114926414F,  -0.153867871F,    0.161101028F,     0.0271121077F,
      0.00130190828F,   0.154399157F,     -0.00131946919F,  0.0746037439F,
      -0.0783764422F,   -0.0276896507F,   -0.031335596F,    0.0517627F,
      0.135367036F,     0.000667774584F,  0.236381888F,     0.0342263095F,
      0.0501087606F,    -0.0973698646F,   -0.190459833F,    -0.0699870586F,
      0.0578431934F,    0.0457646586F,    -0.1441935F,      0.193921879F,
      0.144730419F,     0.0533681251F,    0.137984842F,     0.000931386254F,
      0.189412966F,     -0.084781073F,    -0.057004258F,    -0.000604897563F,
      -0.116203539F,    0.159025744F,     0.0410725176F,    0.172092915F,
      -0.00123649836F,  -0.152446479F,    0.105236925F,     0.000702245045F,
      0.0856303349F,    0.0351231359F,    -0.080520153F,    -0.136352956F,
      -0.482311F,       -0.24442701F,     0.043281626F,     -0.0112866974F,
      0.251540661F,     0.0585129447F,    -0.00857528672F,  0.0491058417F,
      0.0387075879F,    -0.114040785F,    -0.0337748788F,   -0.192918226F,
      0.0856347084F,    0.05089847F,      -0.353909433F,    0.257296473F,
      -0.0841496065F,   0.359268785F,     -0.0676469728F,   0.20263581F,
      0.063291207F,     0.283381134F,     0.0293648187F,    0.235825032F,
      -0.113269709F,    0.0133708827F,    0.208997488F,     -0.0274674408F,
      0.00102248474F,   -2.43457489E-5F,  0.344379902F,     0.0865588859F,
      -0.323001564F,    0.0542085841F,    0.00613147626F,   0.128314748F,
      -0.0895234272F,   -0.060851F,       0.0960444808F,    0.0788563266F,
      0.162114114F,     -0.000168587198F, -0.000408729771F, 0.000698294258F,
      -0.0176401082F,   0.186227351F,     -0.00131730398F,  0.10283114F,
      -0.0495281853F,   0.0829151273F,    0.0297670513F,    -0.00196181657F,
      -0.0324171335F,   0.000884046138F,  -0.00122172933F,  -0.00350201572F,
      -0.0484138317F,   -0.114702232F,    0.00188085821F,   0.0035473255F,
      -0.157313228F,    -0.270324826F,    -0.0369702801F,   0.0464711413F,
      0.0736103281F,    0.127652794F,     -0.132167846F,    -0.0840467215F,
      -0.251417875F,    -0.0973516926F,   0.0263930149F,    -0.126744121F,
      -0.0497680455F,   -0.256805599F,    0.0114933755F,    -0.091192767F};
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
        "../scripts/largeDnnConstants_1551436.bin", 480000);
    readDnnConstants(
        &recurrentStateWeights[0],
        "../scripts/largeDnnConstants_1551441.bin", 160000);
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
  layerOutput[0] -= 0.000501449395F;
  layerOutput[1] += 0.0780053213F;
  layerOutput[2] -= 0.0883610174F;
  SoftmaxLayer_predict(layerOutput, outputData);
}

/*
 * File trailer for predictForRNN.c
 *
 * [EOF]
 */
