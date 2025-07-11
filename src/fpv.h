#ifndef __FPV_H__
#define __FPV_H__

#include "mth/vector.h"
#include "aliases.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef i08 (__fastcall *InteractionCheckFn)(
  v4f *, v4f *, f32, v4f *, i08 *);

typedef struct {
  InteractionCheckFn check;

  // Movement.
  v4f pos;
  v4f vel;
  v4f acc;

  // Rotation.
  v2f rot;
  v2f avel;
  v2f aacc;

  // Configure.
  v4f gravity;
  v4f friction;
  v2f rFriction;
} FPV_t;

FPV_t *fpv_init(
  InteractionCheckFn fn,
  v4f pos,
  v4f gravity
);
FPV_t *fpv_input();
FPV_t *fpv_update(f32 timeElapsed);

#ifdef __cplusplus
}
#endif

#endif
