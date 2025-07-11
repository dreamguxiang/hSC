#include "imgui.h"

#include "ui/gui.h"
#include "ui/input.h"

void gui_inputFreecam() {
  v4f r = v4fnew(0.0f, 0.0f, 0.0f, 0.0f)
    , s = v4fnew(0.0f, 0.0f, 0.0f, 0.0f);

  // Movement.
  if (ImGui::IsKeyDown(ImGuiKey_W))
    r.z += 1.0f;
  if (ImGui::IsKeyDown(ImGuiKey_A))
    gState.freecamMode != FC_FULLDIR
      ? r.x += 1.0f
      : s.z += 1.0f;
  if (ImGui::IsKeyDown(ImGuiKey_S))
    r.z -= 1.0f;
  if (ImGui::IsKeyDown(ImGuiKey_D))
    gState.freecamMode != FC_FULLDIR
      ? r.x -= 1.0f
      : s.z -= 1.0f;

  // Up and down.
  if (ImGui::IsKeyDown(ImGuiKey_Space))
    r.y += 1.0f;
  if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
    r.y -= 1.0f;

  // Mouse. Not used.
  /*
  s.x = mouseDelta.x / MOUSE_SENSITIVITY / 180.0f * PI_F;
  s.y = mouseDelta.y / MOUSE_SENSITIVITY / 180.0f * PI_F;
  */
  s.x = gMouseDelta.x * gOptions.general.mouseSensitivity;
  s.y = gMouseDelta.y * gOptions.general.mouseSensitivity * gOptions.general.verticalSenseScale;

  gState.movementInput = r;
  gState.facingInput = s;
}
