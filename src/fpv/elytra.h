#ifndef __ELYTRA_H__
#define __ELYTRA_H__

#include "aliases.h"
#include "fpv/fpv.h"

#ifdef __cplusplus
extern "C" {
#endif

extern FPV_t gElytra;

FPV_t *fpvElytra_init(v4f pos, v4f rot, i32 flags);
FPV_t *fpvElytra_update(v4f mDelta, v4f fDelta, f32 timeElapsed);

#ifdef __cplusplus
}
#endif

#endif
