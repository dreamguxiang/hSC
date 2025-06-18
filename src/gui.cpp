#include <math.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h" 
#include "uglhook.h"

#include "gui.h"

static const char *MODES[] = { "FirstPerson", "Front", "Placed" };

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
  GUI_t *gui = (GUI_t *)lpUser;
  if (gui->isOpen) {
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

    if (
      uMsg == WM_LBUTTONDOWN
      || uMsg == WM_LBUTTONDBLCLK
      || uMsg == WM_LBUTTONUP
    )
      *uBlock = io.WantCaptureMouse;
    if (
      uMsg == WM_KEYDOWN
      || uMsg == WM_CHAR
    ) {
      if (gui->state.overrideMode == 1) {
        *uBlock = gui->state.enable;
      } else
        *uBlock = io.WantCaptureKeyboard;
    }
    return;
  }

  *uBlock = 0;
}

i08 gui_init(GUI_t *gui) {
  QueryPerformanceFrequency((LARGE_INTEGER *)&gui->performFreq);
  D3D12Hooks::init(
    [](const DXGI_SWAP_CHAIN_DESC *sDesc, void *lpUser) -> void {
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
        lpUser
      );

      ((GUI_t *)lpUser)->hWnd = UniHookGlobals::mainWindow;
    },
    [](void *lpUser) -> void {
      (void)gui_update((GUI_t *)lpUser);
    },
    [](void *lpUser) -> void {
      InputHandler::deinit(UniHookGlobals::mainWindow);
      ImGui_ImplDX12_Shutdown();
      ImGui_ImplWin32_Shutdown();
      ImGui::DestroyContext();
    },
    (void *)gui
  );

  return 1;
}

i08 gui_deinit(GUI_t *gui) {
  D3D12Hooks::deinit();
  return 1;
}

static inline void gui_subMenuSet(GUI_t *gui) {
  ImGui::SeparatorText("Set Camera Params");

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
}

static inline void gui_subMenuFreecam(GUI_t *gui) {
  ImGui::SeparatorText("Free camera");
  ImGui::DragFloat("Speed", &gui->state.freecamSpeed, .01f, 0, 100.0f);
  if (ImGui::Button("Reset pos"))
    gui->state.freecamReset = 1;

  if (ImGui::IsKeyDown(ImGuiKey_W))
    // Go foward.
    gui->state.freecamDir = 1;
  else if (ImGui::IsKeyDown(ImGuiKey_A))
    gui->state.freecamDir = 2;
  else
    gui->state.freecamDir = 0;
}

i08 gui_update(GUI_t *gui) {
  i64 qpc, inteval;
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  if (!gui->lastFrameCounter)
    QueryPerformanceCounter((LARGE_INTEGER *)&gui->lastFrameCounter);
  else {
    QueryPerformanceCounter((LARGE_INTEGER *)&qpc);
    inteval = qpc - gui->lastFrameCounter;
    gui->lastFrameCounter = qpc;
    gui->timeElapsedSecond = (f32)inteval / (f32)gui->performFreq;
  }

  if (GetAsyncKeyState(VK_INSERT) & 0x1)
    gui->isOpen = !gui->isOpen;

  ImGuiStyle &style = ImGui::GetStyle();
  style.ScrollbarRounding = 0.0f;
  style.WindowTitleAlign.x = 0.5f;
  style.WindowTitleAlign.y = 0.5f;

  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  if (gui->isOpen) {
    // Title.
    ImGui::Begin(
      "SkyCamera configure",
      nullptr,
      ImGuiWindowFlags_None
    );

    // General switch.
    ImGui::Checkbox("Take over", (bool *)&gui->state.enable);
    ImGui::Combo("Use mode", &gui->state.cameraMode, MODES, IM_ARRAYSIZE(MODES));

    ImGui::RadioButton("Set", &gui->state.overrideMode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Freecam", &gui->state.overrideMode, 1);
    ImGui::SameLine();
    ImGui::RadioButton("FPV", &gui->state.overrideMode, 2);

    if (gui->state.overrideMode == 0)
      gui_subMenuSet(gui);
    if (gui->state.overrideMode == 1)
      gui_subMenuFreecam(gui);

    // Overlay window FPS display.
    ImGui::Text("Overlay %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

    ImGui::End();
  }

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
