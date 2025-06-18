#ifndef __UGLH_INPUTS_H__
#define __UGLH_INPUTS_H__

#include <windows.h>

namespace InputHandler {
  typedef void (CALLBACK *WndProcEx)(HWND, UINT, WPARAM, LPARAM, UINT *, void *);

  /**
   * Detour window process.
   * 
   * This function should be called in InitCB of the GL hook.
   * 
   * The function will call `handler` when a message is dispatched to the
   * target window, and block messages from reaching the original window
   * process when the variable pointed to by the UINT pointer is set to 1.
   * 
   * @param hWnd Target window. Get from GL hook.
   * @param handler Window process detour function.
   */
  extern void init(HWND hWnd, WndProcEx handler, void *lpUser);

  /**
   * Restore original window process.
   * 
   * @param hWnd Target window.
   */
  extern void deinit(HWND hWnd);
}

#endif
