#ifndef __AABB_H__
#define __AABB_H__

#include "mth/vector.h"
#include "aliases.h"

// 3D AABB, the fourth component of the v4f is disabled.
typedef struct {
  v4f lower;
  v4f upper;
} AABB_t;

void aabb_getAllVertices(AABB_t *aabb, v4f points[8]);

#endif
