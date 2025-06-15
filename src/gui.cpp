#include <math.h>
#include <dwmapi.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h" 

#include "gui.h"

static const char *MODES[] = { "FirstPerson", "Front", "Placed" };

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT WINAPI gui_wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  GUI_t *gui;
  ImVec2 windowPos, mousePos, windowSize;
  i08 flags = 0;

  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;
  
  gui = (GUI_t *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
  if (!gui)
    goto RunDefWndProc;

  switch (msg) {
    case WM_NCHITTEST: 
      mousePos = ImGui::GetMousePos();
      windowPos = ImGui::GetMainViewport()->Pos;
      windowSize = ImGui::GetMainViewport()->Size;
      if (mousePos.y - windowPos.y < 15 && mousePos.x - windowPos.x > 15)
        // On the top and not the collapse button.
        return HTCAPTION;
      flags = (mousePos.x - windowPos.x <= 3)
        | ((windowPos.x + windowSize.x - mousePos.x <= 3) << 1)
        | ((windowPos.y + windowSize.y - mousePos.y <= 3) << 2);
      if (flags == 1)
        return HTLEFT;
      if (flags == 2)
        return HTRIGHT;
      if (flags == 4)
        return HTBOTTOM;
      if (flags == 5)
        return HTBOTTOMLEFT;
      if (flags == 6)
        return HTBOTTOMRIGHT;
      break;
    case WM_SIZE:
      if (wParam == SIZE_MINIMIZED)
        return 0;
      // Queue resize.
      gui->resizeWidth = (UINT)LOWORD(lParam);
      gui->resizeHeight = (UINT)HIWORD(lParam);
      return 0;
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU)
        // Disable ALT application menu.
        return 0;
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }

RunDefWndProc:
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static bool gui_createDeviceD3D(GUI_t *gui) {
  if ((gui->pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
    return false;

  // Create the D3DDevice.
  ZeroMemory(&gui->d3dpp, sizeof(gui->d3dpp));
  gui->d3dpp.Windowed = TRUE;
  gui->d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  // Need to use an explicit format with alpha if needing per-pixel alpha composition.
  gui->d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
  gui->d3dpp.EnableAutoDepthStencil = TRUE;
  gui->d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
  // Present with vsync
  gui->d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
  // Present without vsync, maximum unthrottled framerate
  //gui->d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  if (gui->pD3D->CreateDevice(
    D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, gui->hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &gui->d3dpp, &gui->pD3dDevice
  ) < 0)
    return false;

  return true;
}

static void gui_cleanupDeviceD3D(GUI_t *gui) {
  if (gui->pD3dDevice) {
    gui->pD3dDevice->Release();
    gui->pD3dDevice = nullptr;
  }

  if (gui->pD3D) {
    gui->pD3D->Release();
    gui->pD3D = nullptr;
  }
}

static void gui_resetDevice(GUI_t *gui) {
  ImGui_ImplDX9_InvalidateDeviceObjects();
  HRESULT hr = gui->pD3dDevice->Reset(&gui->d3dpp);
  if (hr == D3DERR_INVALIDCALL)
    IM_ASSERT(0);
  ImGui_ImplDX9_CreateDeviceObjects();
}

static f32 gui_getDpiScale(HWND hWnd) {
  const UINT dpi = GetDpiForWindow(hWnd);
  return dpi / 96.0f;
}

void gui_scaleForDpi(float dpiScale) {
  ImGuiIO& io = ImGui::GetIO();

  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(dpiScale);
  io.FontGlobalScale = dpiScale;
}

i08 gui_createWindowClass(WNDCLASSEXW *wc, HINSTANCE hInstance) {
  wc->cbSize = sizeof(wc);
  wc->style = CS_CLASSDC;
  wc->lpfnWndProc = gui_wndProc;
  wc->cbClsExtra = 0L;
  wc->cbWndExtra = 0L;
  wc->hInstance = GetModuleHandleW(nullptr);
  wc->hIcon = nullptr;
  wc->hCursor = nullptr;
  wc->hbrBackground = nullptr;
  wc->lpszMenuName = nullptr;
  wc->lpszClassName = L"__HT_WC_IMGUI__";
  wc->hIconSm = nullptr;

  return !!RegisterClassExW(wc);
}

i08 gui_createWindow(GUI_t *gui, WNDCLASSEXW *wc) {
  gui->hWnd = CreateWindowW(
    wc->lpszClassName,
    L"Hook Sky Camera",
    WS_OVERLAPPEDWINDOW,
    100, 100, 1280, 800,
    nullptr,
    nullptr,
    wc->hInstance,
    nullptr
  );

  SetWindowLongPtrW(gui->hWnd, GWLP_USERDATA, (LONG_PTR)gui);

  ShowWindow(gui->hWnd, SW_SHOWDEFAULT);
  UpdateWindow(gui->hWnd);

  return !!gui->hWnd;
}

i08 gui_init(GUI_t *gui) {
  WNDCLASSEXW wc = {
    sizeof(WNDCLASSEX),
    CS_CLASSDC,
    gui_wndProc,
    0L,
    0L,
    GetModuleHandle(nullptr),
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    L"__HT_IMGUI_C__",
    nullptr
  };
  HWND hWnd;
  f32 dpiScale;

  RegisterClassExW(&wc);

  // Create window.
  hWnd = CreateWindowExW(
    0,
    wc.lpszClassName,
    L"Hook SkyCamera",
    WS_POPUP | WS_VISIBLE,
    CW_USEDEFAULT, CW_USEDEFAULT,
    100, 100,
    nullptr,
    nullptr,
    wc.hInstance,
    nullptr
  );
  ShowWindow(hWnd, SW_SHOWDEFAULT);
  ImGui_ImplWin32_EnableAlphaCompositing(hWnd);
  gui->hWnd = hWnd;
  UpdateWindow(hWnd);
  SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)gui);

  // Init D3D9.
  gui_createDeviceD3D(gui);

  // Init ImGui.
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.IniFilename = NULL;

  // Handle dpi scale.
  dpiScale = gui_getDpiScale(hWnd);
  gui_scaleForDpi(dpiScale);
  SetWindowPos(
    hWnd,
    nullptr,
    0, 0,
    300 * dpiScale, 200 * dpiScale,
    SWP_NOMOVE | SWP_NOZORDER
  );
  gui->dpiScale = dpiScale;

  ImGui_ImplWin32_Init(hWnd);
  ImGui_ImplDX9_Init(gui->pD3dDevice);

  return 1;
}

i08 gui_deinit(GUI_t *gui) {
  ImGui_ImplDX9_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  gui_cleanupDeviceD3D(gui);
  DestroyWindow(gui->hWnd);

  return 1;
}

i08 gui_update(GUI_t *gui) {
  MSG msg;
  bool next = true;
  ImVec4 bgColor = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
  HRESULT hr;

  while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
    if (msg.message == WM_QUIT)
      return false;
  }

  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScrollbarRounding = 0.0f;
  style.WindowTitleAlign.x = 0.5f;
  style.WindowTitleAlign.y = 0.5f;

  // Handle lost D3D9 device.
  if (gui->deviceLost) {
    hr = gui->pD3dDevice->TestCooperativeLevel();
    if (hr == D3DERR_DEVICELOST) {
      Sleep(10);
      return true;
    }
    if (hr == D3DERR_DEVICENOTRESET)
      gui_resetDevice(gui);
    gui->deviceLost = false;
  }

  // Handle window resize (we don't resize directly in the WM_SIZE handler)
  if (gui->resizeWidth != 0 && gui->resizeHeight != 0) {
    gui->d3dpp.BackBufferWidth = gui->resizeWidth;
    gui->d3dpp.BackBufferHeight = gui->resizeHeight;
    gui->resizeWidth = gui->resizeHeight = 0;
    gui_resetDevice(gui);
  }

  ImGui_ImplDX9_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  ImGuiViewport* viewport = ImGui::GetMainViewport();

  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);

  if (gui->isOpen) {
    // Title.
    ImGui::Begin(
      "SkyCamera configure",
      nullptr,
      ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoCollapse
    );

    // General switch.
    ImGui::Checkbox("Take over", (bool *)&gui->state.enable);
    ImGui::Combo("Use mode", &gui->state.mode, MODES, IM_ARRAYSIZE(MODES));

    // Camera position input.
    ImGui::Checkbox("##cb1", (bool *)&gui->state.overridePos);
    ImGui::SameLine();
    ImGui::InputFloat3("Pos", (float *)&gui->state.pos);

    // Camera facing input.
    ImGui::Checkbox("##cb2", (bool *)&gui->state.overrideDir);
    ImGui::SameLine();
    ImGui::InputFloat2("Facing", (float *)&gui->state.rot);

    // Clamp camera facing.
    gui->state.rot.x = fmodf(gui->state.rot.x, 360.0f);
    gui->state.rot.y = clamp(gui->state.rot.y, -89.5f, 89.5f);

    /*
    if (
      ImGui::SmallButton("Foward")
      && gui->state.overridePos
    ) {

    }*/

    // Camera scale input.
    ImGui::Checkbox("##cb3", (bool *)&gui->state.overrideScale);
    ImGui::SameLine();
    ImGui::DragFloat("Scale", &gui->state.scale, .01f, 0.0f, 1.0f);

    // Camera focus(blur) input.
    ImGui::Checkbox("##cb4", (bool *)&gui->state.overrideFocus);
    ImGui::SameLine();
    ImGui::DragFloat("Focus", &gui->state.focus, .01f, 0.0f, 1.0f);

    // Camera brightness input.
    ImGui::Checkbox("##cb5", (bool *)&gui->state.overrideBrightness);
    ImGui::SameLine();
    ImGui::DragFloat("Brightness", &gui->state.brightness, .01f, 0.0f, 1.0f);

    // Overlay window FPS display.
    ImGui::Text("Overlay %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

    ImGui::End();
  }

  // Rendering
  ImGui::EndFrame();
  gui->pD3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
  gui->pD3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  gui->pD3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
  D3DCOLOR bgColorD3d = D3DCOLOR_RGBA(
    (int)(bgColor.x * bgColor.w * 255.0f),
    (int)(bgColor.y * bgColor.w * 255.0f),
    (int)(bgColor.z * bgColor.w * 255.0f),
    (int)(bgColor.w * 255.0f)
  );
  gui->pD3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, bgColorD3d, 1.0f, 0);
  if (gui->pD3dDevice->BeginScene() >= 0){
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    gui->pD3dDevice->EndScene();
  }
  HRESULT result = gui->pD3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
  if (result == D3DERR_DEVICELOST)
    gui->deviceLost = true;

  return next;
}
