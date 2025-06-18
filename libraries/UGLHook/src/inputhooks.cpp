#include "inputhooks.h"

namespace InputHandler {
  static WNDPROC oWndProc;
  static WndProcEx sHandler = nullptr;
  static void *pUser = nullptr;

  static LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    UINT block = 0;

    if (!sHandler)
      return CallWindowProcW(oWndProc, hwnd, uMsg, wParam, lParam);

    sHandler(hwnd, uMsg, wParam, lParam, &block, pUser);
    if (block)
      return TRUE;

    return CallWindowProcW(oWndProc, hwnd, uMsg, wParam, lParam);
  }

  void init(HWND hWnd, WndProcEx handler, void *lpUser) {
    oWndProc = (WNDPROC)SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (__int3264)(LONG_PTR)WndProc);
    sHandler = handler;
    pUser = lpUser;
  }

  void deinit(HWND hWnd) {
    SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
  }
}
