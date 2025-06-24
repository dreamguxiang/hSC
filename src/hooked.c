#include <math.h>
#include "MinHook.h"

#include "offsets.h"
#include "types.h"
#include "gui.h"
#include "fpv.h"
#include "hooked.h"
#include "log.h"

// Defines.
#define MH_SUCCESSED(v, s) ((v) |= (!(s)))
#define OVERRIDE_2(cond, v1, v2) ((cond) ? ((v1) = (v2)) : ((v2) = (v1)))
#define OVERRIDE_3(cond, v11, v12, v2) ((cond) ? ((v11) = ((v12) = (v2))) : ((v2) = (v11)))

// Typedefs.
typedef u64 (__fastcall *FnSkyCamera_update)(SkyCamera *, u64);
typedef u64 (__fastcall *FnSkyCamera__updateParams)(SkyCamera *, u64);
typedef u64 (__fastcall *FnSkyCamera_updateUI)(
  SkyCamera *, u64, u64, u64, f32 *, f32 *, f32 *, u64, i08);
typedef u64 (__fastcall *FnWorld_interactionTest)(
  u64, v4f *, v4f *, float, v4f *, i08 *);

// External variables.
// gFpv defined in fpv.c
extern FPV_t gFpv;
// gGui defined in gui.cpp
extern GUI_t gGui;

// Static variables.
static const v4f gravity = {-9.8f, 0.0f, 0.0f, 0.0f};
static u64 savedContext = 0;
static LPVOID origin_SkyCamera_update
  , origin_SkyCamera_updateUI
  , origin_SkyCamera__updateParams
  , origin_World_interactionTest
  // The pointer passed into this function must be a local variable address.
  , fn_World_interactionTest;

/**
 * Encapsulation for invocations of World::interactionCheck().
 * 
 * No need to explicitly pass in the context parameter.
 * 
 * The pointer passed into this function must be a local variable address.
 */
static i08 fpvCheckCollision(
  v4f *origin,
  v4f *dir,
  f32 len,
  v4f *a5,
  i08 *buf
) {
  if (savedContext && fn_World_interactionTest) {
    return ((FnWorld_interactionTest)fn_World_interactionTest)(
      savedContext, origin, dir, len, a5, buf);
  } else
    return 0;
}

static void updateCameraSet(SkyCamera *this) {
  v4f *pos, *dir;
  v2f *gsRot, rot;

  if (!(this->cameraType == gGui.state.cameraMode + 1))
    return;

  pos = (v4f *)((i08 *)this->unk_2_3[2] + 0x130);
  dir = (v4f *)((i08 *)this->unk_2_3[2] + 0x140);

  //((u64 (__fastcall *)(u64, v4f *))baseAddr + offset_Player_getPos)(this->player, &pp);

  gsRot = &gGui.state.rot;

  // Override coodinates or sync original camera pos to overlay.
  OVERRIDE_2(gGui.state.overridePos, *pos, gGui.state.pos);

  if (gGui.state.overrideDir) {
    // Override facing directions.
    rot.x = PI_F * gsRot->x / 180.0f;
    rot.y = PI_F * gsRot->y / 180.0f;

    // Forcely reset the rotate status.
    this->rotateSpeedX = this->rotateSpeedY = 0;
    this->rotateXAnim = this->rotateX = rot.x;
    this->rotateYAnim = this->rotateY = rot.y;

    dir->x = sinf(rot.x) * cosf(rot.y);
    // When this->rotateY is increasing, the camera actually turned down, so
    // there's a negative sign here.
    dir->y = -sinf(rot.y);
    // When this->rotateX == 0, dir->z will be set to 1, so we put 
    // cosf(rot.x) on dir->z.
    dir->z = cosf(rot.x) * cosf(rot.y);
  } else {
    // Sync original facing directions to overlay.
    gsRot->y = -asinf(dir->y) / PI_F * 180.0f;

    if (dir->x == 0 && dir->z == 0)
      gsRot->x = 0;
    else {
      // There will be some floating point errors here, but it's fine.
      gsRot->x = atan2f(dir->x, dir->z) / PI_F * 180.0f;
      gsRot->x += gsRot->x < 0 ? 360.0f : 0;
    }
  }

  // Override or sync camera params.
  OVERRIDE_3(
    gGui.state.overrideScale,
    this->scaleAnim,
    this->scale,
    gGui.state.scale);
  OVERRIDE_3(
    gGui.state.overrideFocus,
    this->focusAnim,
    this->focus,
    gGui.state.focus);
  OVERRIDE_3(
    gGui.state.overrideBrightness,
    this->brightnessAnim,
    this->brightness,
    gGui.state.brightness);
}

static void updateCameraFreecam(SkyCamera *this) {
  v4f *pos, *dir
    , lastPos, delta;
  v2f tmp;
  InteractionResult ir;
  f32 dist;

  if (!(this->cameraType == gGui.state.cameraMode + 1))
    return;

  pos = (v4f *)((i08 *)this->unk_2_3[2] + 0x130);
  dir = (v4f *)((i08 *)this->unk_2_3[2] + 0x140);

  if (gGui.state.resetPosFlag) {
    gGui.state.pos = *pos;
    gGui.state.resetPosFlag = 0;
    return;
  }

  // Calculte the direction vector parallel to xOz plane.
  lastPos = gGui.state.pos;
  tmp.x = sinf(this->rotateXAnim);
  tmp.y = cosf(this->rotateXAnim);

  if (gGui.state.freecamAxial) {
    // Rotate it by the movement input.
    delta.x = tmp.x * gGui.state.movementInput.z + tmp.y * gGui.state.movementInput.x;
    delta.y = gGui.state.movementInput.y;
    delta.z = tmp.y * gGui.state.movementInput.z - tmp.x * gGui.state.movementInput.x;
  } else {
    // Combine forward vector and left vector.
    delta = v4fnew(
      gGui.state.movementInput.x * +tmp.y,
      gGui.state.movementInput.y,
      gGui.state.movementInput.x * -tmp.x,
      0.0f);
    delta = v4fadd(delta, v4fscale(*dir, gGui.state.movementInput.z));
  }
  delta = v4fscale(delta, gGui.state.freecamSpeed * gGui.timeElapsedSecond);

  if (gGui.state.freecamCollision ) {
    dist = v4flen(delta) * 2.0f;
    if (
      fpvCheckCollision(
        &lastPos,
        &delta,
        dist < 0.1 ? 0.1 : dist,
        NULL,
        (i08 *)&ir)
    )
      delta = v4fsub(delta, v4fprojection(delta, ir.normalize));
  }

  // Multiply by speed.
  gGui.state.pos = v4fadd(
    lastPos,
    delta);
  *pos = gGui.state.pos;
}

static void updateCameraFPV(SkyCamera *this) {
  v4f *pos;//, *dir;

  if (this->cameraType != gGui.state.cameraMode + 1)
    return;

  pos = (v4f *)((i08 *)this->unk_2_3[2] + 0x130);
  //dir = (v4f *)((i08 *)this->unk_2_3[2] + 0x140);

  if (gGui.state.resetPosFlag) {
    fpv_init(fpvCheckCollision, *pos, gravity);
    gGui.state.resetPosFlag = 0;
  }

  fpv_update(gGui.timeElapsedSecond);

  *pos = gFpv.pos;
}

/**
 * Detour function for SkyCamera::update().
 * 
 * This is the main update function of the camera prop.
 */
static u64 SkyCamera_update_Listener(SkyCamera *this, u64 context) {
  u64 result;
  // NOTE: We should NOT save the SkyCamera *this variable due to it may vary
  // whenever. Every frame the update should be presented in the detour
  // function only.
  result = ((FnSkyCamera_update)origin_SkyCamera_update)(this, context);
  return result;
}

/**
 * Detour function for SkyCamera::_updateParams().
 * 
 * The original function calculates the camera position and facing direction.
 */
static u64 SkyCamera__updateParams_Listener(SkyCamera *this, u64 context) {
  u64 result;
  i64 qpc, inteval;
  GUIState_t *guiState = &gGui.state;

  result = ((FnSkyCamera__updateParams)origin_SkyCamera__updateParams)(
    this, context);

  if (!guiState->enable)
    return result;
  
  // Calculate time elapsed since last frame.
  if (!gGui.lastFrameCounter)
    QueryPerformanceCounter((LARGE_INTEGER *)&gGui.lastFrameCounter);
  else {
    QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
    inteval = qpc - gGui.lastFrameCounter;
    gGui.lastFrameCounter = qpc;
    gGui.timeElapsedSecond = (f32)inteval / (f32)gGui.performFreq;
  }

  if (guiState->overrideMode == 0)
    updateCameraSet(this);
  else if (guiState->overrideMode == 1)
    updateCameraFreecam(this);
  else if (guiState->overrideMode == 2)
    updateCameraFPV(this);

  return result;
}

/**
 * Detour function for SkyCamera::updateUi().
 * 
 * The original function renders the camera UI on the screen.
 */
static u64 SkyCamera_updateUI_Listener(
  SkyCamera *this,
  u64 a2,
  u64 a3,
  u64 a4,
  f32 *scale,
  f32 *focus,
  f32 *brightness,
  u64 a8,
  i08 a9
) {
  u64 result;
  if (gGui.state.noOriginalUi)
    // Disable original camera ui.
    return 0;
  result = ((FnSkyCamera_updateUI)origin_SkyCamera_updateUI)(
    this, a2, a3, a4, scale, focus, brightness, a8, a9);
  return result;
}

/**
 * Detour function for World::interactionCheck().
 * 
 * The original function is executing the interaction check for a line and
 * current level.
 * 
 * Param a5 and a6 is missed when use IDA to decompile.
 */
static u64 World_interactionTest_Listener(
  u64 a1,
  v4f *origin,
  v4f *direction,
  float a4,
  v4f *a5,
  i08 *a6
) {
  u64 result;
  if (savedContext != a1) {
    // Save pointer to current context.
    savedContext = a1;
    LOGI("World::interactionTest(): context: %p\n", (void *)savedContext);
  }
  result = ((FnWorld_interactionTest)origin_World_interactionTest)(
    a1, origin, direction, a4, a5, a6);
  return result;
}

/**
 * Detour function for WhiskerCamera::update().
 * 
 * The original function updates the camera for rendering a frame. The detour
 * function only modifies the rotation matrix and position of the render
 * camera.
 */
static u64 WhiskerCamera_update_Listener(
  WhiskerCamera *this,
  u64 *context
) {
  return 0;
}

/**
 * Hook all the functions that we need.
 */
i08 createAllHooks(void *baseAddr) {
  MH_STATUS s;
  i08 r = 0;

  s = MH_CreateHook(
    baseAddr + offset_SkyCamera_update,
    SkyCamera_update_Listener,
    &origin_SkyCamera_update);
  MH_SUCCESSED(r, s);

  s = MH_CreateHook(
    baseAddr + offset_SkyCamera_updateUI,
    SkyCamera_updateUI_Listener,
    &origin_SkyCamera_updateUI);
  MH_SUCCESSED(r, s);

  s = MH_CreateHook(
    baseAddr + offset_SkyCamera__updateParams,
    SkyCamera__updateParams_Listener,
    &origin_SkyCamera__updateParams);
  MH_SUCCESSED(r, s);

  fn_World_interactionTest = baseAddr + offset_World_interactionTest;
  s = MH_CreateHook(
    fn_World_interactionTest,
    World_interactionTest_Listener,
    &origin_World_interactionTest);
  MH_SUCCESSED(r, s);

  return !s;
}
