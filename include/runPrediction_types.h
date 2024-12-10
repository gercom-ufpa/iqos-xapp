/*
 * File: runPrediction_types.h
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 09-Dec-2024 16:20:26
 */

#ifndef RUNPREDICTION_TYPES_H
#define RUNPREDICTION_TYPES_H

/* Include Files */
#include "rtwtypes.h"

/* Type Definitions */
#ifndef typedef_cell_wrap_0
#define typedef_cell_wrap_0
typedef struct {
  double f1[10];
} cell_wrap_0;
#endif /* typedef_cell_wrap_0 */

#ifndef struct_emxArray_char_T_1x1
#define struct_emxArray_char_T_1x1
struct emxArray_char_T_1x1 {
  char data[1];
  int size[2];
};
#endif /* struct_emxArray_char_T_1x1 */
#ifndef typedef_emxArray_char_T_1x1
#define typedef_emxArray_char_T_1x1
typedef struct emxArray_char_T_1x1 emxArray_char_T_1x1;
#endif /* typedef_emxArray_char_T_1x1 */

#ifndef typedef_cell_wrap_1
#define typedef_cell_wrap_1
typedef struct {
  emxArray_char_T_1x1 f1;
} cell_wrap_1;
#endif /* typedef_cell_wrap_1 */

#ifndef typedef_emxArray_cell_wrap_1_3
#define typedef_emxArray_cell_wrap_1_3
typedef struct {
  cell_wrap_1 data[3];
  int size[1];
} emxArray_cell_wrap_1_3;
#endif /* typedef_emxArray_cell_wrap_1_3 */

#ifndef typedef_categorical
#define typedef_categorical
typedef struct {
  unsigned char codes;
  emxArray_cell_wrap_1_3 categoryNames;
} categorical;
#endif /* typedef_categorical */

#endif
/*
 * File trailer for runPrediction_types.h
 *
 * [EOF]
 */
