#include "dllmain.h"

LPVOID origin_SkyCamera_update
  , origin_SkyCamera_updateUI
  , origin_SkyCamera__updateParams
  , baseAddr;

v4f targetPos = { 6.855124474, 213.959, -35.39220428, 0 };
f32 t = 0.1;
GUI_t gui = {0};

DWORD64 SkyCamera_update_Listener(SkyCamera *this, DWORD64 context) {
  DWORD64 result;
  result = ((DWORD64 (__fastcall *)(SkyCamera *, DWORD64))origin_SkyCamera_update)(this, context);
  return result;
}

DWORD64 SkyCamera__updateParams_Listener(SkyCamera *this, DWORD64 context) {
  DWORD64 result;
  v4f *pos, *dir;
  v2f *gsRot, rot;

  result = ((DWORD64 (__fastcall *)(SkyCamera *, DWORD64))origin_SkyCamera__updateParams)(this, context);

  if (gui.state.enable && this->cameraType == gui.state.mode + 1) {
    pos = (v4f *)((i08 *)this->unk_2_3[2] + 0x130);
    dir = (v4f *)((i08 *)this->unk_2_3[2] + 0x140);

    //((DWORD64 (__fastcall *)(DWORD64, v4f *))baseAddr + offset_Player_getPos)(this->player, &pp);

    gsRot = &gui.state.rot;

    if (gui.state.overridePos)
      // Override coodinates.
      *pos = gui.state.pos;
    else
      // Sync original camera pos to overlay.
      gui.state.pos = *pos;
    
    if (gui.state.overrideDir) {
      // Override facing directions.
      rot.x = 3.14f * gsRot->x / 180.0f;
      rot.y = 3.14f * gsRot->y / 180.0f;

      this->rotateSpeedX = this->rotateSpeedY = 0;
      this->rotateXAnim = this->rotateX = rot.x;
      this->rotateYAnim = this->rotateY = rot.y;

      dir->x = cosf(rot.x) * cosf(rot.y);
      dir->y = sinf(rot.y);
      dir->z = sinf(rot.x) * cosf(rot.y);
    } else {
      // Sync original facing directions to overlay.
      gsRot->y = asinf(dir->y) / 3.14f * 180.0f;
      if (dir->x == 0 && dir->z == 0)
        gsRot->x = 0;
      else {
        gsRot->x = atan2f(dir->z, dir->x) / 3.14f * 180.0f;
        gsRot->x += gsRot->x < 0 ? 360.0f : 0;
      }
    }

    // Override or sync camera params.
    if (gui.state.overrideScale)
      this->scaleAnim = this->scale = gui.state.scale;
    else
      gui.state.scale = this->scaleAnim;

    if (gui.state.overrideFocus)
      this->focusAnim = this->focus = gui.state.focus;
    else
      gui.state.focus = this->focusAnim;

    if (gui.state.overrideBrightness)
      this->brightnessAnim = this->brightness = gui.state.brightness;
    else
      gui.state.brightness = this->brightnessAnim;
  }

  return result;
}

DWORD64 SkyCamera_updateUI_Listener(SkyCamera *this, u64 a2, u64 a3, u64 a4, f32 *scale, f32 *focus, f32 *brightness, u64 a8, i08 a9) {
  DWORD64 result;
  result = ((DWORD64 (__fastcall *)(SkyCamera *, u64, u64, u64, f32 *, f32 *, f32 *, u64, i08))origin_SkyCamera_updateUI)(this, a2, a3, a4, scale, focus, brightness, a8, a9);
  return result;
}

static DWORD WINAPI wndThread(LPVOID lpParam) {
  int s;

  s = gui_init(&gui);
  printf("[HT-INFO] gui_init(): %d\n", s);

  while (gui_update(&gui)) {
    if (GetAsyncKeyState(VK_INSERT) & 0x1) {
      gui.isOpen = !gui.isOpen;
      ShowWindow(gui.hWnd, gui.isOpen ? SW_NORMAL : SW_HIDE);
    }
  }

  gui_deinit(&gui);

  return 0;
}

BOOL APIENTRY DllMain(
  HMODULE hModule,
  DWORD dwReason,
  LPVOID lpReserved
) {
  MH_STATUS s;
  HANDLE hSubThread;
  DWORD threadId;

  if (dwReason == DLL_PROCESS_ATTACH) {
    baseAddr = (LPVOID)GetModuleHandleA("Sky.exe");

    if (!baseAddr)
      return TRUE;

    FreeConsole();
    AllocConsole();
    freopen("CONOUT$", "w+t", stdout);
    freopen("CONIN$", "r+t", stdin);
    printf("[HT-INFO] Dll injected.\n");
    printf("[HT-INFO] (Sky.exe + 0x0): 0x%p\n", baseAddr);

    MH_Initialize();
    MH_CreateHook(baseAddr + offset_SkyCamera_update, SkyCamera_update_Listener, &origin_SkyCamera_update);
    MH_CreateHook(baseAddr + offset_SkyCamera_updateUI, SkyCamera_updateUI_Listener, &origin_SkyCamera_updateUI);
    MH_CreateHook(baseAddr + offset_SkyCamera__updateParams, SkyCamera__updateParams_Listener, &origin_SkyCamera__updateParams);

    s = MH_EnableHook(MH_ALL_HOOKS);
    printf("[HT-INFO] MH_EnableHook(): %d\n", s);

    hSubThread = CreateThread(
      NULL,
      0,
      wndThread,
      (LPVOID)hModule,
      0,
      &threadId
    );
    if (!hSubThread)
      return TRUE;
    printf("[HT-INFO] CreateThread(): 0x%lX\n", threadId);
  } else if (dwReason == DLL_PROCESS_DETACH) {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
  }

  return TRUE;
}
