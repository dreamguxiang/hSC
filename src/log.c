#include <windows.h>

#include "log.h"

void recreateConsole() {
#ifdef DEBUG_CONSOLE
  FreeConsole();
  AllocConsole();
  freopen("CONOUT$", "w+t", stdout);
  freopen("CONIN$", "r+t", stdin);
#endif
}
