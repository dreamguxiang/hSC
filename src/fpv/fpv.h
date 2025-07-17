#ifndef __FPV_H__
#define __FPV_H__

#include "mth/vector.h"
#include "aliases.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FPVRST_POS (0x01)
#define FPVRST_ROT (0x02)

// The type of the fpv update functions must match the definition below.
typedef i08 (__fastcall *FnFpvUpdate)(
  v4f mDelta, v4f fDelta, f32 timeElapsed);
typedef i08 (__fastcall *FnFpvReset)(
  v4f pos, v4f rot, i32 flags);

typedef struct {
  // Movement. Only the x, y and z components are used.
  v4f pos;
  v4f vel;
  v4f acc;

  // Rotation. Only the x and y components are used.
  v4f rot;
  v4f avel;
  v4f aacc;
} FPV_t;

i08 fpv_checkCollision();

#ifdef __cplusplus
}
#endif

#endif
