#include "dllmain.h"

LPVOID baseAddr;

static DWORD WINAPI onAttach(LPVOID lpParam) {
  MSG msg;
  MH_STATUS ms;
  i32 s;

  tryInitWithSig();

  createAllHooks(baseAddr);
  ms = MH_EnableHook(MH_ALL_HOOKS);
  LOGI("MH_EnableHook(): %d\n", ms);

  s = gui_init();
  LOGI("gui_init(): %d\n", s);

  while (GetMessageW(&msg, NULL, 0, 0)) {
    if (msg.message == WM_USER_EXIT)
      PostQuitMessage(0);
    if (msg.message == WM_QUIT)
      break;
  }

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
    if (!hSubThread)
      return TRUE;
    LOGI("CreateThread(): 0x%lX\n", threadId);
  } else if (dwReason == DLL_PROCESS_DETACH) {
    PostThreadMessageW(threadId, WM_USER_EXIT, 0, 0);
    WaitForSingleObject(hSubThread, INFINITE);
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
  }

  return TRUE;
}
