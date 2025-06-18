#include "dllmain.h"

LPVOID baseAddr;
GUI_t gui = {0};

static DWORD WINAPI onAttach(LPVOID lpParam) {
  MSG msg;
  i32 s;

  s = gui_init(&gui);
  LOGI("[HT-INFO] gui_init(): %d\n", s);

  while (GetMessageW(&msg, NULL, 0, 0)) {
    if (msg.message == WM_USER_EXIT)
      PostQuitMessage(0);
    if (msg.message == WM_QUIT)
      break;
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
  HANDLE hSubThread = NULL;
  DWORD threadId = 0;

  if (dwReason == DLL_PROCESS_ATTACH) {
    baseAddr = (LPVOID)GetModuleHandleA("Sky.exe");

    if (!baseAddr)
      return TRUE;

    recreateConsole();

    LOGI("[HT-INFO] Dll injected.\n");
    LOGI("[HT-INFO] (Sky.exe + 0x0): 0x%p\n", baseAddr);

    MH_Initialize();
    createAllHooks(baseAddr);
    s = MH_EnableHook(MH_ALL_HOOKS);
    LOGI("[HT-INFO] MH_EnableHook(): %d\n", s);

    hSubThread = CreateThread(
      NULL,
      0,
      onAttach,
      (LPVOID)hModule,
      0,
      &threadId
    );
    if (!hSubThread)
      return TRUE;
    LOGI("[HT-INFO] CreateThread(): 0x%lX\n", threadId);
  } else if (dwReason == DLL_PROCESS_DETACH) {
    PostThreadMessageW(threadId, WM_USER_EXIT, 0, 0);
    WaitForSingleObject(hSubThread, INFINITE);
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
  }

  return TRUE;
}
