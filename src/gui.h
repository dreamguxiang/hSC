#ifndef __GUI_H__
#define __GUI_H__

#include <windows.h>
#include <d3d9.h>

#include "vector.h"
#include "aliases.h"

#define clamp(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))

typedef struct {
  i32 mode;
  v4f pos;
  v2f rot;
  f32 scale;
  f32 focus;
  f32 brightness;
  i08 enable;
  i08 overridePos;
  i08 overrideDir;
  i08 overrideFocus;
  i08 overrideScale;
  i08 overrideBrightness;
} GUIState_t;

typedef struct {
  HWND hWnd;
  D3DPRESENT_PARAMETERS d3dpp;
  LPDIRECT3D9 pD3D;
  LPDIRECT3DDEVICE9 pD3dDevice;
  UINT resizeWidth;
  UINT resizeHeight;
  GUIState_t state;
  f32 dpiScale;
  i08 deviceLost;
  i08 isOpen;
} GUI_t;

#ifdef __cplusplus
extern "C" {
#endif

i08 gui_createWindowClass(WNDCLASSEXW *wc, HINSTANCE hInstance);
i08 gui_createWindow(GUI_t *gui, WNDCLASSEXW *wc);

i08 gui_init(GUI_t *gui);
i08 gui_deinit(GUI_t *gui);

i08 gui_update(GUI_t *gui);

#ifdef __cplusplus
}
#endif

#endif
