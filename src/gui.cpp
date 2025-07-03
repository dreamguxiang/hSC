#include <math.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h" 
#include "uglhook.h"
#include "gui.h"

#define clamp(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))
#define SBV(v, b) ((v) |= (b))
#define CBV(v, b) ((v) &= ~(b))
#define BTV(v, b) ((v) & (b))

// Pixel per degree.
#define MOUSE_SENSITIVITY 1.0f

static const char *MODES[] = { "FirstPerson", "Front", "Placed" }
  , *FREECAMMODES[] = { "Orientation", "Axial", "Full-direction" };

GUI_t gGui = {0};
GUIState_t gState = {0};
GUIOptions_t gOptions = {
  .freecam = {
    .mouseSensitivity = 2.0f
  }
};

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
  HWND hWnd,
  UINT msg,
  WPARAM wParam,
  LPARAM lParam);

static void gui_wndProcEx(
  HWND hWnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam,
  UINT *uBlock,
  void *lpUser
) {
  ImGuiIO &io = ImGui::GetIO();

  ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

  switch (uMsg) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP:
      *uBlock = io.WantCaptureMouse;
      return;
    case WM_KEYDOWN:
      if (gState.overrideMode == 1 && gState.enable) {
        if (
          wParam == 'W'
          || wParam == 'A'
          || wParam == 'S'
          || wParam == 'D'
          || wParam == 'Q'
          || wParam == 'E'
          || wParam == VK_SPACE
          || wParam == VK_SHIFT
        )
          *uBlock = 1;
        else 
          *uBlock = io.WantCaptureKeyboard;
      } else
        *uBlock = io.WantCaptureKeyboard;
      return;
    case WM_CHAR:
      *uBlock = 1;
      return;
  }

  *uBlock = 0;
}

/**
 * Wait for the dll to be loaded.
 */
i08 gui_waitForDll() {
  i32 ctr = 0;
  while (!GetModuleHandleA("dxgi.dll") || !GetModuleHandleA("d3d12.dll")) {
    Sleep(100);
    ctr++;
    // Wait for 30 seconds.
    if (ctr > 300000)
      return 0;
  }
  return 1;
}

/**
 * Initialize the gui.
 */
i08 gui_init() {
  // We assume that the gGui is safely deinitialized.
  memset(&gGui, 0, sizeof(GUI_t));
  QueryPerformanceFrequency((LARGE_INTEGER *)&gGui.performFreq);
  gGui.hInit = CreateEventW(NULL, 0, 0, NULL);
  return D3D12Hooks::init(
    [](const DXGI_SWAP_CHAIN_DESC *sDesc, void *lpUser) -> void {
      SetEvent(gGui.hInit);

      f32 dpiScale;
      ImGui::CreateContext();
      ImGui::StyleColorsDark();

      ImGuiIO &io = ImGui::GetIO(); (void)io;
      io.Fonts->AddFontDefault();
      io.IniFilename = NULL;

      ImGuiStyle& style = ImGui::GetStyle();
      dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(sDesc->OutputWindow);
      style.ScaleAllSizes(dpiScale);
      io.FontGlobalScale = dpiScale;

      ImGui_ImplWin32_Init(UniHookGlobals::mainWindow);
      ImGui_ImplDX12_Init(
        D3D12Hooks::gDevice,
        D3D12Hooks::gBufferCount,
        sDesc->BufferDesc.Format,
        D3D12Hooks::gHeapSRV,
        D3D12Hooks::gHeapSRV->GetCPUDescriptorHandleForHeapStart(),
        D3D12Hooks::gHeapSRV->GetGPUDescriptorHandleForHeapStart()
      );
      ImGui_ImplDX12_CreateDeviceObjects();

      InputHandler::init(
        UniHookGlobals::mainWindow,
        gui_wndProcEx,
        nullptr
      );

      gGui.hWnd = UniHookGlobals::mainWindow;
    },
    [](void *lpUser) -> void {
      (void)gui_update();
    },
    [](void *lpUser) -> void {
      InputHandler::deinit(UniHookGlobals::mainWindow);
      ImGui_ImplDX12_Shutdown();
      ImGui_ImplWin32_Shutdown();
      ImGui::DestroyContext();
    },
    nullptr
  );
}

/**
 * Uninitialze the dx12 hooks.
 */
i08 gui_deinit() {
  D3D12Hooks::deinit();
  if (gGui.hInit)
    CloseHandle(gGui.hInit);
  return 1;
}

/**
 * Wait for the first IDXGISwapChain::Present() call.
 */
i08 gui_waitForInit() {
  i08 r = 1;
  if (WaitForSingleObject(gGui.hInit, 30000) != WAIT_OBJECT_0)
    r = 0;
  CloseHandle(gGui.hInit);
  gGui.hInit = NULL;
  return r;
}

static inline void gui_displayTips(i08 sameLine, const char *desc) {
  if (sameLine)
    ImGui::SameLine();
  ImGui::TextDisabled("(?)");
  if (ImGui::BeginItemTooltip()) {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

static inline void gui_subMenuSet() {
  ImGui::SeparatorText("Set Camera Params");

  // Camera position input.
  ImGui::Checkbox("##cb1", (bool *)&gState.overridePos);
  ImGui::SameLine();
  ImGui::InputFloat3("Pos", (float *)&gState.pos);

  // Camera rotation input.
  ImGui::Checkbox("##cb2", (bool *)&gState.overrideDir);
  ImGui::SameLine();
  ImGui::InputFloat3("Rotation", (float *)&gState.rot);

  // Clamp camera facing.
  gState.rot.x = fmodf(gState.rot.x, 360.0f);
  gState.rot.y = clamp(gState.rot.y, -89.5f, 89.5f);

  // Camera scale input.
  ImGui::Checkbox("##cb3", (bool *)&gState.overrideScale);
  ImGui::SameLine();
  ImGui::DragFloat("Scale", &gState.scale, .01f, 0.0f, 1.0f);

  // Camera focus(blur) input.
  ImGui::Checkbox("##cb4", (bool *)&gState.overrideFocus);
  ImGui::SameLine();
  ImGui::DragFloat("Focus", &gState.focus, .01f, 0.0f, 1.0f);

  // Camera brightness input.
  ImGui::Checkbox("##cb5", (bool *)&gState.overrideBrightness);
  ImGui::SameLine();
  ImGui::DragFloat("Brightness", &gState.brightness, .01f, 0.0f, 1.0f);
}

static inline void gui_subMenuFreecam() {
  ImGui::SeparatorText("Free camera");

  ImGui::Combo(
    "Mode",
    &gState.freecamMode,
    FREECAMMODES,
    IM_ARRAYSIZE(FREECAMMODES));

  ImGui::Checkbox("Check collision", (bool *)&gState.freecamCollision);

  ImGui::BeginDisabled(gState.freecamMode == FC_FULLDIR);
  if (gState.freecamMode == FC_FULLDIR)
    gState.freecamRoll = 1;
  ImGui::Checkbox("QE Roll", (bool *)&gState.freecamRoll);
  ImGui::EndDisabled();
  gui_displayTips(
    true,
    "The camera won't move based on orientation when enabled this.");

  ImGui::BeginDisabled(!gState.freecamRoll);
  ImGui::DragFloat("Roll Speed", &gState.freecamRollSpeed, .01f, 0, 10.0f);
  ImGui::EndDisabled();

  ImGui::BeginDisabled(gState.freecamMode != FC_FULLDIR);
  ImGui::DragFloat("Mouse sensitivity", &gState.freecamSensitivity, .01f, 0, 10.0f);
  ImGui::EndDisabled();

  ImGui::DragFloat("Speed", &gState.freecamSpeed, .01f, 0, 100.0f);
  if (ImGui::Button("Reset pos"))
    gState.resetPosFlag = 1;
}

static inline void gui_subMenuFPV() {
  ImGui::SeparatorText("FPV");
  if (ImGui::Button("Reset pos"))
    gState.resetPosFlag = 1;
}

static inline void gui_windowMain() {
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // Title.
  ImGui::Begin(
    "hSC Menu",
    nullptr,
    ImGuiWindowFlags_None
  );

  // General options.
  if (ImGui::Checkbox("Take over", (bool *)&gState.enable))
    gState.resetPosFlag = 1;
  ImGui::Combo("Use mode", &gState.cameraMode, MODES, IM_ARRAYSIZE(MODES));
  ImGui::Checkbox("No UI", (bool *)&gState.noOriginalUi);
  gui_displayTips(
    true,
    "Hide the original camera UI. Please adjust the parameters before select this item.");

  ImGui::RadioButton("Set", &gState.overrideMode, 0);
  ImGui::SameLine();
  ImGui::RadioButton("Freecam", &gState.overrideMode, 1);
  ImGui::SameLine();
  ImGui::RadioButton("FPV", &gState.overrideMode, 2);

  if (gState.overrideMode == 0)
    gui_subMenuSet();
  if (gState.overrideMode == 1)
    gui_subMenuFreecam();
  if (gState.overrideMode == 2)
    gui_subMenuFPV();

  // Overlay window FPS display.
  ImGui::Text("Overlay %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

  ImGui::End();
}

static inline void gui_windowSettings() {

}

/**
 * Handle keyboard and mouse inputs for freecam mode.
 */
static void gui_inputFreecam() {
  ImGuiIO &io = ImGui::GetIO();
  ImVec2 mouseDelta = io.MouseDelta;
  v4f r = v4fnew(0.0f, 0.0f, 0.0f, 0.0f)
    , s = v4fnew(0.0f, 0.0f, 0.0f, 0.0f);

  // Movement.
  if (ImGui::IsKeyDown(ImGuiKey_W))
    r.z += 1.0f;
  if (ImGui::IsKeyDown(ImGuiKey_A))
    r.x += 1.0f;
  if (ImGui::IsKeyDown(ImGuiKey_S))
    r.z -= 1.0f;
  if (ImGui::IsKeyDown(ImGuiKey_D))
    r.x -= 1.0f;

  // Up and down.
  if (ImGui::IsKeyDown(ImGuiKey_Space))
    r.y += 1.0f;
  if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
    r.y -= 1.0f;

  // Roll.
  if (ImGui::IsKeyDown(ImGuiKey_Q))
    s.z += 1.0f;
  if (ImGui::IsKeyDown(ImGuiKey_E))
    s.z -= 1.0f;

  // Mouse.
  s.x = mouseDelta.x / MOUSE_SENSITIVITY / 180.0f * PI_F;
  s.y = mouseDelta.y / MOUSE_SENSITIVITY / 180.0f * PI_F;

  gState.movementInput = r;
  gState.facingInput = s;
}

/**
 * Render ui and handle keyboard inputs.
 */
i08 gui_update() {
  if (GetAsyncKeyState(VK_INSERT) & 0x1)
    gGui.isOpen = !gGui.isOpen;

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScrollbarRounding = 0.0f;
  style.WindowTitleAlign.x = 0.5f;
  style.WindowTitleAlign.y = 0.5f;

  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  // Draw menus.
  if (gGui.isOpen)
    gui_windowMain();

  // Handle keyboard input.
  if (gState.overrideMode == 1)
    gui_inputFreecam();

  // Rendering.
  ImGui::EndFrame();

  D3D12Hooks::FrameContext& currentFrameContext
    = D3D12Hooks::gFrameContext[D3D12Hooks::gSavedSwapChain->GetCurrentBackBufferIndex()];

  currentFrameContext.commandAllocator->Reset();

  D3D12_RESOURCE_BARRIER barrier;
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = currentFrameContext.mainRenderTargetResource;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

  D3D12Hooks::gCommandList->Reset(
    currentFrameContext.commandAllocator,
    nullptr);
  D3D12Hooks::gCommandList->ResourceBarrier(1, &barrier);
  D3D12Hooks::gCommandList->OMSetRenderTargets(
    1,
    &currentFrameContext.mainRenderTargetDescriptor,
    FALSE,
    nullptr);
  D3D12Hooks::gCommandList->SetDescriptorHeaps(1, &D3D12Hooks::gHeapSRV);

  ImGui::Render();
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), D3D12Hooks::gCommandList);

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

  D3D12Hooks::gCommandList->ResourceBarrier(1, &barrier);
  D3D12Hooks::gCommandList->Close();

  D3D12Hooks::gCommandQueue->ExecuteCommandLists(
    1,
    reinterpret_cast<ID3D12CommandList* const*>(&D3D12Hooks::gCommandList));

  return 1;
}
