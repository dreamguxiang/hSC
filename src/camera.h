#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "aliases.h"
#include "mth/vector.h"
#include "types.h"

void updatePropMain(SkyCameraProp *);
void updateCameraMain(SkyCamera *);

void updateMouseDelta(v4f);

#endif
