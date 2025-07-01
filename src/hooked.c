#include <math.h>
#include "MinHook.h"

#include "types.h"
#include "gui.h"
#include "fpv.h"
#include "hooked.h"
#include "log.h"
#include "setup.h"
#include "vector.h"
#include "matrix.h"

// Defines.
#define MH_SUCCESSED(v, s) ((v) |= (!(s)))
#define OVERRIDE_2(cond, v1, v2) ((cond) ? ((v1) = (v2)) : ((v2) = (v1)))
#define OVERRIDE_3(cond, v11, v12, v2) ((cond) ? ((v11) = ((v12) = (v2))) : ((v2) = (v11)))

// ----------------------------------------------
// [SECTION] Definitions and declarations.
// ----------------------------------------------

// Typedefs.
typedef u64 (__fastcall *FnSkyCameraProp_update)(SkyCameraProp *, u64);
typedef u64 (__fastcall *FnSkyCameraProp__updateParams)(SkyCameraProp *, u64);
typedef u64 (__fastcall *FnSkyCameraProp_updateUI)(
  SkyCameraProp *, u64, u64, u64, f32 *, f32 *, f32 *, u64, i08);
typedef u64 (__fastcall *FnWorld_interactionTest)(
  u64, v4f *, v4f *, float, v4f *, i08 *);
typedef u64 (__fastcall *FnWhiskerCamera_update)(
  WhiskerCamera *, u64 *);
typedef u64 (__fastcall *FnSkyCamera_update)(
  SkyCamera *, u64 *);
typedef u64 (__fastcall *FnMainCamera__getDelta)(
  u64, i08 *, u64, u64, u64, int, int);

// External variables.
// gFpv defined in fpv.c
extern FPV_t gFpv;
// gGui and gState defined in gui.cpp
extern GUI_t gGui;
extern GUIState_t gState;

// Static variables.
static const v4f gravity = {-9.8f, 0.0f, 0.0f, 0.0f};
static u64 savedContext = 0;
static SetupFunctions_t tramp = {0};

// Global variables.
static i08 gInit = 0;
static i08 gDoUpdate = 0;
static SetupFunctions_t gFunc;
static v4f gMouseDelta;

// ----------------------------------------------
// [SECTION] Camera update functions.
// ----------------------------------------------

/**
 * Calculate rotation matrix from euler angle, in the order of yaw, pitch,
 * roll.
 * 
 * @param euler The x, y and z component of this vector indicates the yaw,
 * pitch and roll angles.
 * @param matrix The 3x3 rotation matrix stored in three v4f in rows.
 */
static inline void eulerToRotationXYZ(v4f euler, v4f *matrix) {
  f32 cy = cos(euler.x)
    , sy = sin(euler.x)
    , cp = cos(euler.y)
    , sp = sin(euler.y)
    , cr = cos(euler.z)
    , sr = sin(euler.z);
  
  // R00
  matrix[0].x = cr * cy + sr * sp * sy;
  // R01
  matrix[0].y = sr * cp;
  // R02
  matrix[0].z = -cr * sy + sr * sp * cy;
  
  // R10
  matrix[1].x = cr * sp * sy - sr * cy;
  // R11
  matrix[1].y = cr * cp;
  // R12
  matrix[1].z = sr * sy + cr * sp * cy; 
  
  // R20
  matrix[2].x = cp * sy;
  // R21
  matrix[2].y = -sp;
  // R22
  matrix[2].z = cp * cy;
}

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
  if (savedContext && tramp.fn_Level_interactionTest) {
    return ((FnWorld_interactionTest)tramp.fn_Level_interactionTest)(
      savedContext, origin, dir, len, a5, buf);
  } else
    return 0;
}

static i08 updatePropSet(SkyCameraProp *this) {
  v4f *pos, *dir, *gsRot
    , rot;

  if (this->cameraType != gState.cameraMode + 1)
    return 0;

  dir = &((SkyCamera *)this->unk_2_3[2])->dir;
  pos = &((SkyCamera *)this->unk_2_3[2])->pos;
  gsRot = &gState.rot;

  // Override coodinates or sync original camera pos to overlay.
  OVERRIDE_2(gState.overridePos, *pos, gState.pos);
  gState.mat[3] = *pos;

  if (gState.overrideDir) {
    // Override facing directions.
    rot = v4fscale(gState.rot, PI_F / 180.0f);

    // Forcely reset the rotate status.
    this->rotateSpeedX = this->rotateSpeedY = 0;
    this->rotateXAnim = this->rotateX = rot.x;
    this->rotateYAnim = this->rotateY = rot.y;

    eulerToRotationXYZ(rot, gState.mat);
  } else {
    // Sync original facing directions to overlay.
    gsRot->y = -asinf(dir->y) / PI_F * 180.0f;
    gsRot->z = 0;

    if (dir->x == 0 && dir->z == 0)
      gsRot->x = 0;
    else {
      // There will be some floating point errors here, but it's fine.
      gsRot->x = atan2f(dir->x, dir->z) / PI_F * 180.0f;
      gsRot->x += gsRot->x < 0 ? 360.0f : 0;
    }
  }

  gState.usePos = gState.overridePos;
  gState.useMatrix = gState.overrideDir;

  // Override or sync camera params.
  OVERRIDE_3(
    gState.overrideScale,
    this->scaleAnim,
    this->scale,
    gState.scale);
  OVERRIDE_3(
    gState.overrideFocus,
    this->focusAnim,
    this->focus,
    gState.focus);
  OVERRIDE_3(
    gState.overrideBrightness,
    this->brightnessAnim,
    this->brightness,
    gState.brightness);
  
  return 1;
}

static i08 updatePropFreecam(SkyCameraProp *this, u64 *context) {
  v4f *pos, *dir
    , deltaRot = {0}
    , lastPos, delta;
  v2f tmp;
  f32 dist;
  m44 mat = {0};
  InteractionResult ir;

  if (this->cameraType != gState.cameraMode + 1)
    return 0;

  pos = &((SkyCamera *)this->unk_2_3[2])->pos;
  dir = &((SkyCamera *)this->unk_2_3[2])->dir;

  gState.usePos = 0;

  if (gState.resetPosFlag) {
    gState.pos = *pos;
    gState.rot = v4fnew(
      this->rotateXAnim,
      this->rotateYAnim,
      0,
      0);
    gState.resetPosFlag = 0;
    eulerToRotationXYZ(gState.rot, gState.mat);
    return 0;
  }

  // Calculte the direction vector parallel to xOz plane.
  lastPos = gState.pos;
  tmp.x = sinf(this->rotateXAnim);
  tmp.y = cosf(this->rotateXAnim);

  if (gState.freecamMode == FC_ORIENT) {
    // Combine forward vector and left vector.
    delta = v4fnew(
      gState.movementInput.x * +tmp.y,
      gState.movementInput.y,
      gState.movementInput.x * -tmp.x,
      0.0f);
    delta = v4fadd(delta, v4fscale(*dir, gState.movementInput.z));

    // Do not override the rotation matrix.
    gState.useMatrix = 0;
    gState.rot.z = 0;
  } else if (gState.freecamMode == FC_AXIAL) {
    // Rotate it by the movement input. Axial mode will ignore the roll angle.
    delta.x = tmp.x * gState.movementInput.z + tmp.y * gState.movementInput.x;
    delta.y = gState.movementInput.y;
    delta.z = tmp.y * gState.movementInput.z - tmp.x * gState.movementInput.x;

    // Do not override the rotation matrix.
    gState.useMatrix = 0;
    gState.rot.z = 0;
  } else if (gState.freecamMode == FC_FULLDIR) {
    // Calculate rotation delta from hooked data.
    if (gState.freecamRoll)
      deltaRot.z = gState.facingInput.z * gState.freecamRollSpeed;
    deltaRot.x = -gMouseDelta.x * gState.freecamSensitivity;
    deltaRot.y = gMouseDelta.y * gState.freecamSensitivity;
    deltaRot = v4fscale(deltaRot,
      gGui.timeElapsedSecond);

    // Calculate rotation matrix based on last frame.
    eulerToRotationXYZ(deltaRot, (v4f *)&mat);
    mat.row4 = v4fnew(0, 0, 0, 1);
    m44mul((m44 *)gState.mat, &mat, (m44 *)gState.mat);

    // Combine forward vector and left vector. The full-direction mode will use
    // the foward vector we calculated, instead of the game.
    delta = v4fscale(gState.mat[2], -gState.movementInput.z);
    delta = v4fadd(delta, v4fscale(gState.mat[0], -gState.movementInput.x));
    delta = v4fnormalize(delta);

    // Override the rotation matrix to enable roll angle.
    gState.useMatrix = 1;
  }
  delta = v4fscale(delta, gState.freecamSpeed * gGui.timeElapsedSecond);

  if (gState.freecamCollision) {
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
  gState.pos = v4fadd(
    lastPos,
    delta);
  *pos = gState.pos;

  return 1;
}

static i08 updatePropFPV(SkyCameraProp *this) {
  v4f *pos;//, *dir;

  if (this->cameraType != gState.cameraMode + 1)
    return 0;

  pos = &((SkyCamera *)this->unk_2_3[2])->pos;
  //dir = (v4f *)((i08 *)this->unk_2_3[2] + 0x140);

  if (gState.resetPosFlag) {
    fpv_init(fpvCheckCollision, *pos, gravity);
    gState.resetPosFlag = 0;
  }

  fpv_update(gGui.timeElapsedSecond);

  *pos = gFpv.pos;

  return 1;
}

/**
 * Calculate the rotation matrix and camera pos with gui data.
 */
static void updatePropMain(SkyCameraProp *this, u64 *context) {
  i64 qpc, inteval;
  GUIState_t *guiState = &gState;
  i08 doUpdate;

  if (!guiState->enable) {
    gDoUpdate = 0;
    return;
  }
  
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
    doUpdate = updatePropSet(this);
  else if (guiState->overrideMode == 1)
    doUpdate = updatePropFreecam(this, context);
  else if (guiState->overrideMode == 2)
    doUpdate = updatePropFPV(this);

  gDoUpdate = doUpdate;
}

/**
 * All the data is calculated in updateProp functions, this function only
 * copies them into the SkyCamera struct.
 */
static void updateCameraMain(SkyCamera *this) {
  GUIState_t *guiState = &gState;

  if (!gDoUpdate)
    return;
  gDoUpdate = 0;

  if (guiState->useMatrix) {
    this->super.context1.mat1 = guiState->mat[0];
    this->super.context1.mat2 = guiState->mat[1];
    this->super.context1.mat3 = guiState->mat[2];
  }
  if (guiState->usePos) {
    this->super.context1.cameraPos = guiState->mat[3];
    this->super.context1.cameraPos.w = 1.0f;
  }
}

// ----------------------------------------------
// [SECTION] Detour functions.
// ----------------------------------------------

/**
 * Detour function for SkyCameraProp::update().
 * 
 * This is the main update function of the camera prop.
 */
static u64 SkyCameraProp_update_Listener(SkyCameraProp *this, u64 context) {
  u64 result;
  // NOTE: We should NOT save the SkyCameraProp *this variable due to it may vary
  // whenever. Every frame the update should be presented in the detour
  // function only.
  result = ((FnSkyCameraProp_update)tramp.fn_SkyCameraProp_update)(
    this, context);
  return result;
}

/**
 * Detour function for SkyCameraProp::_updateParams().
 * 
 * The original function calculates the camera position and facing direction.
 */
static u64 SkyCameraProp__updateParams_Listener(SkyCameraProp *this, u64 context) {
  u64 result;

  result = ((FnSkyCameraProp__updateParams)tramp.fn_SkyCameraProp__updateParams)(
    this, context);

  updatePropMain(this, (u64 *)context);

  return result;
}

/**
 * Detour function for SkyCameraProp::updateUI().
 * 
 * The original function renders the camera UI on the screen.
 */
static u64 SkyCameraProp_updateUI_Listener(
  SkyCameraProp *this,
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
  if (gState.noOriginalUi)
    // Disable original camera ui.
    return 0;
  result = ((FnSkyCameraProp_updateUI)tramp.fn_SkyCameraProp_updateUI)(
    this, a2, a3, a4, scale, focus, brightness, a8, a9);
  return result;
}

/**
 * Detour function for World::interactionCheck().
 * 
 * The original function is executing the interaction check for a line and
 * current level.
 * 
 * The pointer passed into this function must be a local variable address.
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
  if (savedContext != a1 && gInit) {
    // Save pointer to current context.
    savedContext = a1;
    LOGI("World::interactionTest(): context: %p\n", (void *)savedContext);
  }
  result = ((FnWorld_interactionTest)tramp.fn_Level_interactionTest)(
    a1, origin, direction, a4, a5, a6);
  return result;
}

/**
 * Detour function for WhiskerCamera::update().
 * 
 * The original function updates the default camera which mouse controls.
 */
static u64 WhiskerCamera_update_Listener(
  WhiskerCamera *this,
  u64 *context
) {
  u64 result;
  result = ((FnWhiskerCamera_update)tramp.fn_WhiskerCamera_update)(
    this, context);
  gMouseDelta = this->mouseDelta;
  return result;
}

/**
 * Detour function for SkyCamera::update().
 * 
 * The original function calculates the rotation matrix from the orientation
 * vector.
 */
static u64 SkyCamera_update_Listener(
  SkyCamera *this,
  u64 *context
) {
  u64 result;
  result = ((FnSkyCamera_update)tramp.fn_SkyCamera_update)(
    this, context);
  updateCameraMain(this);
  return result;
}

/**
 * Detour function for MainCamera::_getdelta().
 * 
 * The original function calculates the mouse delta on last frame.
 */
static u64 MainCamera__getDelta_Listener(
  u64 a1,
  i08 *a2,
  u64 a3,
  u64 a4,
  u64 a5,
  int a6,
  int a7
) {
  u64 result;
  result = ((FnMainCamera__getDelta)tramp.fn_MainCamera__getDelta)(
    a1, a2, a3, a4, a5, a6, a7);
  if (a2[1])
    gMouseDelta = *(v4f *)(a2 + 0x10);
  else
    gMouseDelta = v4fnew(0, 0, 0, 0);
  return result;
}

// ----------------------------------------------
// [SECTION] Initializations.
// ----------------------------------------------

static const void *detourFunc[9] = {
  SkyCameraProp__updateParams_Listener,
  SkyCameraProp_updateUI_Listener,
  NULL,
  SkyCameraProp_update_Listener,
  NULL,
  World_interactionTest_Listener,
  WhiskerCamera_update_Listener,
  SkyCamera_update_Listener,
  MainCamera__getDelta_Listener
};

/**
 * Scan functions with signature.
 */
i08 initAllHooks() {
  return setupFuncWithSig(&gFunc);
}

/**
 * Hook all the functions that we need.
 */
i08 createAllHooks() {
  MH_STATUS s;
  i32 length;
  i08 r = 1;
  void *fn;

  if (gInit)
    return 1;

  length = sizeof(detourFunc) / sizeof(void *);
  for (i32 i = 0; i < length; i++) {
    if (!gFunc.functions[i] || !detourFunc[i])
      continue;

    fn = gFunc.functions[i];
    s = MH_CreateHook(
      fn,
      (void *)detourFunc[i],
      &tramp.functions[i]);

    if (s) {
      LOGE("Create hook on 0x%p failed.\n", fn);
      r = 0;
    } else {
      LOGI("Created hook on 0x%p\n", fn);
      s = MH_EnableHook(fn);
      if (s) {
        LOGE("Enable hook on 0x%p failed.\n", fn);
        r = 0;
      } else
        LOGI("Enabled hook on 0x%p\n", fn);
    }
  }

  gInit = 1;

  return r;
}

/**
 * Remove hooks on the functions.
 */
i08 removeAllHooks() {
  MH_STATUS s;
  i32 length;
  i08 r = 1;
  void *fn;

  length = sizeof(detourFunc) / sizeof(void *);
  for (i32 i = 0; i < length; i++) {
    fn = gFunc.functions[i];
    if (!fn || !tramp.functions[i])
      continue;
    s = MH_RemoveHook(fn);
    if (s) {
      LOGE("Remove hook on 0x%p failed.\n", fn);
      r = 0;
    } else
      LOGI("Removed hook on 0x%p\n", fn);
  }

  gInit = 0;

  return r;
}
