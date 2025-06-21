#include "fpv.h"

#define clamp(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))

FPV_t gFpv = {0};

FPV_t *fpv_init(
  InteractionCheckFn fn,
  v4f pos,
  v4f gravity
) {
  memset(&gFpv, 0, sizeof(FPV_t));
  gFpv.check = fn;
  gFpv.gravity = gravity;
  gFpv.pos = pos;
  gFpv.friction.y = 0.2f;
  return &gFpv;
}

FPV_t *fpv_input() {
  return &gFpv;
}

FPV_t *fpv_update(f32 timeElapsed) {
  v4f pos, vel, acc;
  f32 speed, dist;
  InteractionResult ir;

  // x += v * dt;
  gFpv.pos = v4fadd(gFpv.pos, v4fscale(gFpv.vel, timeElapsed));
  // Gravity influence.
  acc = v4fadd(gFpv.acc, gFpv.gravity);
  // f = k * v => a = m * k * v; m is considered as 1.
  // v_terminal = a_input / k, for every components.
  acc = v4fsub(acc, v4fmul(gFpv.vel, gFpv.friction));
  // v += a * dt;
  gFpv.vel = v4fadd(gFpv.vel, v4fscale(acc, timeElapsed));

  // Check for collision.
  vel = gFpv.vel;
  speed = v4flen(vel);
  vel = v4fnormalize(vel);
  pos = gFpv.pos;
  dist = speed * timeElapsed;
  if (
    gFpv.check
    && gFpv.check(&pos, &vel, dist < 0.1 ? 0.1 : dist, NULL, (i08 *)&ir)
  ) {
    //gFpv.pos = ir.intersection;
    gFpv.vel = v4freflect(ir.normalize, gFpv.vel);
  }
  //  memset(&gFpv.vel, 0, sizeof(v4f));

  return &gFpv;
}
