#include "dllmain.h"

LPVOID baseAddr;

static DWORD WINAPI onAttach(LPVOID lpParam) {
  MSG msg;
  i32 s;

  // Wait for the dx12 to be loaded.
  s = gui_waitForDll();
  if (!s) {
    LOGEF("dxgi.dll or d3d12.dll load timed out.");
    return 0;
  }

  // Initialize gui.
  s = gui_init();
  LOGI("gui_init(): %d\n", s);
  // We'll wait for the gui.
  if (!gui_waitForInit()) {
    LOGEF("Gui init timed out.");
    return 0;
  }

  // Initialize functions. Sleep for a while in order to wait for the game to
  // completely decrypt the uinstructions.
  Sleep(2500);
  initAllHooks();
  createAllHooks();

  while (GetMessageW(&msg, NULL, 0, 0)) {
    // Wait for the exit message.
    if (msg.message == WM_USER_EXIT)
      PostQuitMessage(0);
    if (msg.message == WM_QUIT)
      break;
  }

  // This won't execute actually. This thread may be terminated before the
  // DllMain is called.
  removeAllHooks();
  gui_deinit();

  return 0;
}

BOOL APIENTRY DllMain(
  HMODULE hModule,
  DWORD dwReason,
  LPVOID lpReserved
) {
  HANDLE hSubThread = NULL;
  DWORD threadId = 0;

  if (dwReason == DLL_PROCESS_ATTACH) {
    baseAddr = (LPVOID)GetModuleHandleA("Sky.exe");

    if (!baseAddr)
      // Not the correct game process.
      return TRUE;

    recreateConsole();

    LOGI("Dll injected.\n");
    LOGI("(Sky.exe + 0x0): 0x%p\n", baseAddr);

    MH_Initialize();

    hSubThread = CreateThread(
      NULL,
      0,
      onAttach,
      (LPVOID)hModule,
      0,
      &threadId);
    if (!hSubThread) {
      LOGEF("Create subthread failed.\n");
      return TRUE;
    }
    LOGI("CreateThread(): 0x%lX\n", threadId);
  } else if (dwReason == DLL_PROCESS_DETACH) {
    LOGI("Dll detached.\n");
    PostThreadMessageW(threadId, WM_USER_EXIT, 0, 0);
    WaitForSingleObject(hSubThread, INFINITE);
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
  }

  return TRUE;
}
