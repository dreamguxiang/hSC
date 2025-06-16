#include <math.h>
#include <stdio.h>

#include "MinHook.h"

#include "offsets.h"
#include "types.h"
#include "gui.h"

#include "hooked.h"

#define MH_SUCCESSED(v, s) ((v) |= (!(s)))

LPVOID origin_SkyCamera_update
  , origin_SkyCamera_updateUI
  , origin_SkyCamera__updateParams;
extern GUI_t gui;

static u64 SkyCamera_update_Listener(SkyCamera *this, u64 context) {
  u64 result;
  result = ((u64 (__fastcall *)(SkyCamera *, u64))origin_SkyCamera_update)(this, context);
  return result;
}

static u64 SkyCamera__updateParams_Listener(SkyCamera *this, u64 context) {
  u64 result;
  v4f *pos, *dir;
  v2f *gsRot, rot;

  result = ((u64 (__fastcall *)(SkyCamera *, u64))origin_SkyCamera__updateParams)(this, context);

  if (gui.state.enable && this->cameraType == gui.state.mode + 1) {
    pos = (v4f *)((i08 *)this->unk_2_3[2] + 0x130);
    dir = (v4f *)((i08 *)this->unk_2_3[2] + 0x140);

    //((u64 (__fastcall *)(u64, v4f *))baseAddr + offset_Player_getPos)(this->player, &pp);

    gsRot = &gui.state.rot;

    // Override coodinates or sync original camera pos to overlay.
    OVERRIDE_2(gui.state.overridePos, *pos, gui.state.pos);

    if (gui.state.overrideDir) {
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
    OVERRIDE_3(gui.state.overrideScale, this->scaleAnim, this->scale, gui.state.scale);
    OVERRIDE_3(gui.state.overrideFocus, this->focusAnim, this->focus, gui.state.focus);
    OVERRIDE_3(gui.state.overrideBrightness, this->brightnessAnim, this->brightness, gui.state.brightness);
  }

  return result;
}

static u64 SkyCamera_updateUI_Listener(SkyCamera *this, u64 a2, u64 a3, u64 a4, f32 *scale, f32 *focus, f32 *brightness, u64 a8, i08 a9) {
  u64 result;
  result = ((u64 (__fastcall *)(SkyCamera *, u64, u64, u64, f32 *, f32 *, f32 *, u64, i08))origin_SkyCamera_updateUI)(this, a2, a3, a4, scale, focus, brightness, a8, a9);
  return result;
}

i08 createAllHooks(void *baseAddr) {
  MH_STATUS s;
  i08 r = 0;

  s = MH_CreateHook(baseAddr + offset_SkyCamera_update, SkyCamera_update_Listener, &origin_SkyCamera_update);
  MH_SUCCESSED(r, s);
  s = MH_CreateHook(baseAddr + offset_SkyCamera_updateUI, SkyCamera_updateUI_Listener, &origin_SkyCamera_updateUI);
  MH_SUCCESSED(r, s);
  s = MH_CreateHook(baseAddr + offset_SkyCamera__updateParams, SkyCamera__updateParams_Listener, &origin_SkyCamera__updateParams);
  MH_SUCCESSED(r, s);

  return !s;
}
