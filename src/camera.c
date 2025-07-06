#include <math.h>

#include "mth/matrix.h"
#include "setup.h"
#include "fpv.h"
#include "gui.h"
#include "camera.h"
#include "aabb.h"

// ----------------------------------------------------------------------------
// [SECTION] Declarations and definitions.
// ----------------------------------------------------------------------------

// Macros.
#define OVERRIDE_2(cond, v1, v2) ((cond) ? ((v1) = (v2)) : ((v2) = (v1)))
#define OVERRIDE_3(cond, v11, v12, v2) ((cond) ? ((v11) = ((v12) = (v2))) : ((v2) = (v11)))

// Typedefs.
typedef u64 (__fastcall *FnWorld_interactionTest)(
  u64, v4f *, v4f *, float, v4f *, i08 *);

// External variables.
// gTramp trampoline functions defined in hooked.c
extern SetupFunctions_t gTramp;
// gFpv defined in fpv.c
extern FPV_t gFpv;
// GUI globals defined in gui.cpp
extern GUI_t gGui;
extern GUIState_t gState;
extern GUIOptions_t gOptions;

// Static variables and consts.
static const v4f gravity = {-9.8f, 0.0f, 0.0f, 0.0f};
static i08 gDoUpdate = 0;
static v4f gMouseDelta = {0};

// Globale variables.
u64 gSavedLevelContext = 0;

// ----------------------------------------------------------------------------
// [SECTION] Static helper functions.
// ----------------------------------------------------------------------------

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
 i08 fpvCheckCollision(
  AABB_t *aabb,
  v4f *delta
) {
  u08 result = 0
    , done = 0;
  v4f vec = *delta
    , vertices[8]
    , origin, dir;
  f32 len;
  i32 iter;
  InteractionResult ir;

  if (!gSavedLevelContext || !gTramp.fn_Level_interactionTest)
    return 0;

  aabb_getAllVertices(aabb, vertices);
  dir = v4fnormalize(vec);
  len = v4flen(vec);

  for (i32 i = 0; i < 8; i++) {
    iter = 0;
    origin = vertices[i];

    while (
      ((FnWorld_interactionTest)gTramp.fn_Level_interactionTest)(
        gSavedLevelContext, &origin, &dir, len, NULL, (i08 *)&ir)
    ) {
      vec = v4fsub(vec, v4fprojection(vec, ir.normalize));
      dir = v4fnormalize(vec);
      len = v4flen(vec);

      result = 1;
      if (len < 0.0001) {
        done = 1;
        break;
      }

      iter++;
      if (iter >= 3)
        break;
    }

    if (done)
      break;
  }

  *delta = vec;

  return result;
}

// ----------------------------------------------------------------------------
// [SECTION] Camera update functions.
// ----------------------------------------------------------------------------

i08 updatePropSet(SkyCameraProp *this) {
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

i08 updatePropFreecam(SkyCameraProp *this) {
  v4f deltaRot = {0}
    , size = {0.1f, 0.1f, 0.1f, 0.1f}
    , *pos, *dir
    , lastPos, delta;
  v2f tmp;
  f32 t;
  m44 mat = {0};
  AABB_t aabb;

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
    deltaRot.z = gState.facingInput.z * gState.freecamRotateSpeed;
    deltaRot.x = -gMouseDelta.x * gOptions.general.mouseSensitivity;
    deltaRot.y = gMouseDelta.y * gOptions.general.mouseSensitivity * gOptions.general.verticalSenseScale;

    if (gOptions.freecam.swapRollYaw) {
      t = deltaRot.z;
      deltaRot.z = deltaRot.x;
      deltaRot.x = t;
    }

    deltaRot = v4fscale(deltaRot,
      gGui.timeElapsedSecond);

    // Calculate rotation matrix based on last frame.
    eulerToRotationXYZ(deltaRot, (v4f *)&mat);
    mat.row4 = v4fnew(0, 0, 0, 1);
    m44mul((m44 *)gState.mat, &mat, (m44 *)gState.mat);

    // Combine forward vector and left vector. The full-direction mode will use
    // the foward vector we calculated, instead of the game.
    delta = v4fscale(gState.mat[2], -gState.movementInput.z);
    delta = v4fnormalize(delta);

    // Override the rotation matrix to enable roll angle.
    gState.useMatrix = 1;
  }
  delta = v4fscale(delta, gState.freecamSpeed * gGui.timeElapsedSecond);

  if (gState.freecamCollision) {
    aabb.lower = v4fsub(gState.pos, size);
    aabb.upper = v4fadd(gState.pos, size);
    fpvCheckCollision(&aabb, &delta);
  }

  // Multiply by speed.
  gState.pos = v4fadd(
    lastPos,
    delta);
  *pos = gState.pos;

  return 1;
}

i08 updatePropFPV(SkyCameraProp *this) {
  v4f *pos;//, *dir;

  if (this->cameraType != gState.cameraMode + 1)
    return 0;

  pos = &((SkyCamera *)this->unk_2_3[2])->pos;
  //dir = (v4f *)((i08 *)this->unk_2_3[2] + 0x140);

  if (gState.resetPosFlag) {
    //fpv_init(fpvCheckCollision, *pos, gravity);
    gState.resetPosFlag = 0;
  }

  fpv_update(gGui.timeElapsedSecond);

  *pos = gFpv.pos;

  return 1;
}

/**
 * Calculate the rotation matrix and camera pos with gui data, and save
 * caculated data to gState.
 */
void updatePropMain(SkyCameraProp *this) {
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
    doUpdate = updatePropFreecam(this);
  else if (guiState->overrideMode == 2)
    doUpdate = updatePropFPV(this);

  gDoUpdate = doUpdate;
}

/**
 * All the data is calculated in updateProp functions, this function only
 * copies them into the SkyCamera struct.
 */
void updateCameraMain(SkyCamera *this) {
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

// ----------------------------------------------------------------------------
// [SECTION] Status update functions.
// ----------------------------------------------------------------------------

/**
 * Update the mouse delta, only called by the hook on MainCamera::_getdelta().
 */
void updateMouseDelta(v4f delta) {
  //gMouseDelta.x = gState.facingInput.x;
  //gMouseDelta.y = gState.facingInput.y;
  gMouseDelta = delta;
}
