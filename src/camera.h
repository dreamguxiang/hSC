#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "aliases.h"
#include "mth/vector.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void updatePropMain(SkyCameraProp *);
void updateCameraMain(SkyCamera *);

void updateMouseDelta(v4f);

#ifdef __cplusplus
}
#endif

#endif
