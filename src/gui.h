#ifndef __GUI_H__
#define __GUI_H__

#include <windows.h>

#include "vector.h"
#include "aliases.h"

#define WM_USER_EXIT (0x8000 + 1)

#define FC_ORIENT (0x00)
#define FC_AXIAL (0x01)
#define FC_FULLDIR (0x02)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // General controller.
  i08 enable;
  i08 noOriginalUi;
  i32 cameraMode;
  i32 overrideMode;

  // Inputs.
  v4f movementInput;
  v4f facingInput;

  // Original data sync.
  v4f pos;
  v4f rot;
  f32 scale;
  f32 focus;
  f32 brightness;

  // Set param mode.
  i08 overridePos;
  i08 overrideDir;
  i08 overrideFocus;
  i08 overrideScale;
  i08 overrideBrightness;

  // Freecam mode.
  f32 freecamSpeed;
  f32 freecamRollSpeed;
  f32 freecamSensitivity;
  i32 freecamMode;
  i08 freecamCollision;
  i08 resetPosFlag;
  i08 freecamRoll;

  // Pre-calculated rotation matrix and pos.
  i08 useMatrix;
  i08 usePos;
  v4f mat[4];
} GUIState_t;

typedef struct {
  HWND hWnd;
  HANDLE hInit;
  i64 performFreq;
  i64 lastFrameCounter;
  f32 timeElapsedSecond;
  f32 dpiScale;
  i08 isOpen;
} GUI_t;

i08 gui_init();
i08 gui_deinit();
i08 gui_waitForInit();
i08 gui_waitForDll();
i08 gui_update();

#ifdef __cplusplus
}
#endif

#endif
