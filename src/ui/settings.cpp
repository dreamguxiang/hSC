#include "imgui.h"

#include "ui/gui.h"
#include "ui/settings.h"

void gui_windowSettings() {
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::Begin("hSC Settings", (bool *)&gGui.showSettings);

  ImGui::SeparatorText("General settings");

  ImGui::Text("Mouse sensitivity");
  ImGui::DragFloat(
    "##settingsDrag1",
    &gOptions.general.mouseSensitivity,
    0.01f,
    0.0f,
    10.0f);
  ImGui::Text("Vertical sensitivity scale");
  ImGui::DragFloat(
    "##settingsDrag2",
    &gOptions.general.verticalSenseScale,
    0.01f,
    0.0f,
    2.0f);

  ImGui::SeparatorText("Freecam settings");

  ImGui::Text("Swap yaw and roll");
  ImGui::Checkbox(
    "##settingsCheckbox3",
    (bool *)&gOptions.freecam.swapRollYaw);

  ImGui::End();
}
