#ifndef __GUI_H__
#define __GUI_H__

#include <windows.h>

#include "vector.h"
#include "aliases.h"

#define WM_USER_EXIT (0x8000 + 1)

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
  v2f rot;
  f32 scale;
  f32 focus;
  f32 brightness;

  // Set param mode.
  i08 overridePos;
  i08 overrideDir;
  i08 overrideFocus;
  i08 overrideScale;
  i08 overrideBrightness;

  f32 freecamSpeed;
  i08 freecamAxial;
  i08 freecamCollision;
  i08 resetPosFlag;
} GUIState_t;

typedef struct {
  HWND hWnd;
  HANDLE hInit;
  GUIState_t state;
  i64 performFreq;
  i64 lastFrameCounter;
  f32 timeElapsedSecond;
  f32 dpiScale;
  i08 isOpen;
} GUI_t;

i08 gui_init();
i08 gui_deinit();
i08 gui_waitForInit();
i08 gui_update();

#ifdef __cplusplus
}
#endif

#endif
