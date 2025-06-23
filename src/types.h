#ifndef __TYPES_H__
#define __TYPES_H__

#include <windows.h>

#include "aliases.h"
#include "vector.h"

typedef enum {
  FIRST = 1,
  FRONT = 2,
  PLACE = 3
} SkyCameraType;

typedef struct {
  i64 lpVtbl;
  i32 field_8;
  i08 field_C;
  i64 field_D;
  i64 field_E;
  i08 field_15;
  i32 field_16;
  i08 field_1E;
  i32 unk_1;
  i32 field_1F;
  i32 field_23;
  i08 cameraType;
  i08 unk_2[15];
  u64 player;
  i08 unk_2_2[56];
  u64 *unk_2_3;
  u64 *unk_2_4;
  u64 *unk_2_5;
  i08 unk_2_6[13];
  v4f cameraPos;
  i08 unk_3[64];
  f32 rotateXAnim;
  f32 rotateYAnim;
  i08 unk_4[8];
  f32 rotateSpeedX;
  f32 rotateSpeedY;
  i08 unk_4_2[8];
  f32 rotateX;
  f32 rotateY;
  u64 unk_4_3;
  f32 scale;
  f32 scaleAnim;
  i08 unk_4_4[24];
  f32 focus;
  f32 focusAnim;
  u64 unk_4_5;
  f32 brightness;
  f32 brightnessAnim;
  i08 unk_5[88];
  v4f lookAt;
  i08 unk_6[40];
  f32 unk_7;
  i08 unk_8[20];
  i32 *unk_8_2;
  i08 unk_8_3[5];
} SkyCamera;

// The size of this struct is incorrect. This definition only contains 
// components what we need.
typedef struct {
  u64 lpVtbl;
  char unk_1[56];
  v4f mat1;
  v4f mat2;
  v4f mat3;
  v4f pos;
} RenderCamera;

typedef struct {
  v4f intersection;
  v4f normalize;
  i08 unk[0x40];
} InteractionResult;

#endif
