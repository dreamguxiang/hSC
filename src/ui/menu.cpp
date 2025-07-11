#include <math.h>
#include "imgui.h"

#include "ui/gui.h"
#include "ui/menu.h"

#define clamp(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))

static const char *MODES[] = { "FirstPerson", "Front", "Placed" }
  , *FREECAMMODES[] = { "Orientation", "Axial", "Full-direction" };


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

static void gui_subMenuSet() {
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

static void gui_subMenuFreecam() {
  ImGui::SeparatorText("Free camera");

  ImGui::Combo(
    "Mode",
    &gState.freecamMode,
    FREECAMMODES,
    IM_ARRAYSIZE(FREECAMMODES));

  ImGui::Checkbox("Check collision", (bool *)&gState.freecamCollision);

  ImGui::DragFloat("Rotate Speed", &gState.freecamRotateSpeed, .01f, 0, 10.0f);

  ImGui::DragFloat("Speed", &gState.freecamSpeed, .01f, 0, 100.0f);
  if (ImGui::Button("Reset pos"))
    gState.resetPosFlag = 1;
}

static void gui_subMenuFPV() {
  ImGui::SeparatorText("FPV");
  if (ImGui::Button("Reset pos"))
    gState.resetPosFlag = 1;
}

static void gui_navMain() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Edit")) {
      ImGui::MenuItem("Preferences", nullptr, (bool *)&gGui.showSettings);
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
}

void gui_windowMain() {
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // Title.
  ImGui::Begin(
    "hSC Main",
    (bool *)&gGui.isOpen,
    ImGuiWindowFlags_MenuBar);

  gui_navMain();

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
