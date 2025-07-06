#include "aabb.h"

void aabb_getAllVertices(AABB_t *aabb, v4f points[8]) {
  for (int i = 0; i < 8; i++) {
    points[i].x = i & 0x1 ? aabb->lower.x : aabb->upper.x;
    points[i].y = i & 0x2 ? aabb->lower.y : aabb->upper.y;
    points[i].z = i & 0x4 ? aabb->lower.z : aabb->upper.z;
  }
}
