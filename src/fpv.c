#include "fpv.h"

#include "log.h"

#define clamp(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))

FPV_t *fpv_init(
  FPV_t *fpv,
  InterractionCheckFn fn,
  v4f pos,
  v4f gravity
) {
  memset(fpv, 0, sizeof(FPV_t));
  fpv->check = fn;
  fpv->gravity = gravity;
  fpv->pos = pos;
  fpv->friction.y = 0.2f;
  return fpv;
}

FPV_t *fpv_input(FPV_t *fpv) {
  return fpv;
}

FPV_t *fpv_update(FPV_t *fpv, f32 timeElapsed) {
  v4f pos, vel, acc;
  f32 speed, dist;
  i08 buffer[0x60];

  // x += v * dt;
  fpv->pos = v4fadd(fpv->pos, v4fscale(fpv->vel, timeElapsed));
  // Gravity influence.
  acc = v4fadd(fpv->acc, fpv->gravity);
  // f = k * v => a = m * k * v; m is considered as 1.
  acc = v4fsub(acc, v4fmul(fpv->vel, fpv->friction));
  // v += a * dt;
  fpv->vel = v4fadd(fpv->vel, v4fscale(acc, timeElapsed));

  // Check for collision.
  vel = fpv->vel;
  speed = v4flen(vel);
  vel = v4fnormalize(vel);
  pos = fpv->pos;
  dist = speed * timeElapsed;
  if (fpv->check && fpv->check(&pos, &vel, dist < 0.1 ? 0.1 : dist, NULL, buffer))
    memset(&fpv->vel, 0, sizeof(v4f));

  return fpv;
}