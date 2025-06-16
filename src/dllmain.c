#include "dllmain.h"

LPVOID baseAddr;

GUI_t gui = {0};

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
    createAllHooks(baseAddr);
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
