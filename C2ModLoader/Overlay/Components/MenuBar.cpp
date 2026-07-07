#include "MenuBar.h"

#include "Camera/Camera.h"
#include "CheatsManager.h"
#include "Loader.h"
#include "MenuActions.h"
#include "ModApi.h"
#include "Overlay/Components/CameraStack.h"
#include "Overlay/Components/Coords.h"
#include "Overlay/Components/Inputs.h"
#include "Overlay/Components/LevelInfo.h"
#include "Overlay/Components/LevelSelect.h"
#include "Overlay/Components/Log.h"
#include "Overlay/Components/Minimap.h"
#include "Overlay/Components/ObjectList.h"
#include "Overlay/Components/SaveSlotList.h"
#include "Overlay/Components/Toast.h"
#include "Overlay/Components/VersionInfo.h"
#include "Overlay/Overlay.h"
#include "Utils.h"
#include "imgui.h"

#include <algorithm>
#include <string>
#include <vector>

extern ModApi *api;

namespace Overlay::MenuBar {

void RenderMenuBar() {
	static bool menuOpen = false;
	static float alpha = 1.0f;
	static const float revealZone = 36.0f;
	static const float fadeSpeed = 12.0f;

	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();
	float now = ImGui::GetTime();
	float dt = ImGui::GetIO().DeltaTime;

	float mouseY = io.MousePos.y;
	float topY = vp->Pos.y;

	float scaledReveal = revealZone * io.FontGlobalScale * io.DisplayFramebufferScale.y;
	float styleBarHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y * 2.0f;
	bool inRevealZone = (mouseY >= 0.0f && mouseY <= topY + scaledReveal) || (mouseY >= topY && mouseY <= topY + styleBarHeight);

	static float lastRevealTime = 0.0f;
	static const float hideDelay = 0.6f;
	if (inRevealZone || menuOpen) {
		lastRevealTime = now;
	}
	bool recentlyRevealed = (now - lastRevealTime) <= hideDelay;

	bool shouldReveal = inRevealZone || menuOpen || recentlyRevealed;

	float targetAlpha = shouldReveal ? 1.0f : 0.0f;
	alpha += (targetAlpha - alpha) * std::clamp(fadeSpeed * dt, 0.0f, 1.0f);

	if (alpha <= 0.001f)
		return;

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

	bool anyMenuOpenThisFrame = false;
	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("Loader")) {
			anyMenuOpenThisFrame = true;

			if (ImGui::BeginMenu("Logging")) {
				if (ImGui::MenuItem("Show Info", nullptr, &Overlay::Log::showLogInfo)) {
					api->WriteIniBool(L"Logging", L"Info", Overlay::Log::showLogInfo);
				}
				if (ImGui::MenuItem("Show Debug", nullptr, &Overlay::Log::showLogDebug)) {
					api->WriteIniBool(L"Logging", L"Debug", Overlay::Log::showLogDebug);
				}
				if (ImGui::MenuItem("Show Warnings", nullptr, &Overlay::Log::showLogWarning)) {
					api->WriteIniBool(L"Logging", L"Warning", Overlay::Log::showLogWarning);
				}
				if (ImGui::MenuItem("Show Errors", nullptr, &Overlay::Log::showLogError)) {
					api->WriteIniBool(L"Logging", L"Error", Overlay::Log::showLogError);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Toasts")) {
				anyMenuOpenThisFrame = true;
				if (ImGui::MenuItem("Show Info", nullptr, &Overlay::Toast::showToastInfo)) {
					api->WriteIniBool(L"Toasts", L"Info", Overlay::Toast::showToastInfo);
				}
				if (ImGui::MenuItem("Show Debug", nullptr, &Overlay::Toast::showToastDebug)) {
					api->WriteIniBool(L"Toasts", L"Debug", Overlay::Toast::showToastDebug);
				}
				if (ImGui::MenuItem("Show Warnings", nullptr, &Overlay::Toast::showToastWarning)) {
					api->WriteIniBool(L"Toasts", L"Warning", Overlay::Toast::showToastWarning);
				}
				if (ImGui::MenuItem("Show Errors", nullptr, &Overlay::Toast::showToastError)) {
					api->WriteIniBool(L"Toasts", L"Error", Overlay::Toast::showToastError);
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			anyMenuOpenThisFrame = true;

			if (ImGui::MenuItem("Show Windows", nullptr, &Overlay::showWindows)) {
				api->WriteIniBool(L"GUI", L"ShowWindows", Overlay::showWindows);
			}

			ImGui::Separator();

			ImGui::BeginDisabled(!Overlay::showWindows);

			if (ImGui::MenuItem("Show Log", nullptr, &Overlay::Log::showLog)) {
				api->WriteIniBool(L"GUI", L"ShowLog", Overlay::Log::showLog);
			}
			if (ImGui::MenuItem("Show Inputs", nullptr, &Overlay::InputsComponent::showInputs)) {
				api->WriteIniBool(L"GUI", L"ShowInputs", Overlay::InputsComponent::showInputs);
			}
			if (ImGui::MenuItem("Show Object List", nullptr, &Overlay::ObjectList::showObjectList)) {
				api->WriteIniBool(L"GUI", L"ShowObjectList", Overlay::ObjectList::showObjectList);
			}
			if (ImGui::MenuItem("Show Coords", nullptr, &Overlay::Coords::showCoords)) {
				api->WriteIniBool(L"GUI", L"ShowCoords", Overlay::Coords::showCoords);
			}
			if (ImGui::MenuItem("Show Minimap", nullptr, &Overlay::Minimap::showMinimap)) {
				api->WriteIniBool(L"GUI", L"ShowMinimap", Overlay::Minimap::showMinimap);
			}
			if (ImGui::MenuItem("Show Level Info", nullptr, &Overlay::LevelInfoComponent::showLevelInfo)) {
				api->WriteIniBool(L"GUI", L"ShowLevelInfo", Overlay::LevelInfoComponent::showLevelInfo);
			}
			if (ImGui::MenuItem("Show Save Slot List", nullptr, &Overlay::SaveSlotList::showSaveSlotList)) {
				api->WriteIniBool(L"GUI", L"ShowSaveSlotList", Overlay::SaveSlotList::showSaveSlotList);
			}
			if (ImGui::MenuItem("Show Level Select", nullptr, &Overlay::LevelSelect::showLevelSelect)) {
				api->WriteIniBool(L"GUI", L"ShowLevelSelect", Overlay::LevelSelect::showLevelSelect);
			}
			if (ImGui::MenuItem("Show Camera Stack", nullptr, &Overlay::CameraStack::showCameraStack)) {
				api->WriteIniBool(L"GUI", L"ShowCameraStack", Overlay::CameraStack::showCameraStack);
			}

			ImGui::EndDisabled();

			ImGui::EndMenu();
		}

		if (Camera::cameraEnabled && ImGui::BeginMenu("Camera")) {
			anyMenuOpenThisFrame = true;

			if (ImGui::MenuItem("Orbit Camera", nullptr, Camera::orbitCamera)) {
				Camera::toggleOrbitCamera();
			}

			if (ImGui::MenuItem("Free Camera", nullptr, Camera::cameraMode == Camera::CameraMode::Freecam)) {
				Camera::toggleFreecam();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Cheats")) {
			anyMenuOpenThisFrame = true;

			if (ImGui::MenuItem("Debug Menu", nullptr, &cheatsDebugMenu)) {
				setDebugMenu(cheatsDebugMenu);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Press [Inv Prev + Inv Next] for a debug menu.");
			}

			if (ImGui::MenuItem("Position Bar", nullptr, &cheatsPositionBar)) {
				setPositionBar(cheatsPositionBar);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Press [F7] for position bar.");
			}

			if (ImGui::MenuItem("Invulnerability", nullptr, &cheatsInvulnerability)) {
				setInvulnerability(cheatsInvulnerability);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Never take any damage.");
			}

			if (ImGui::MenuItem("Bonus Crystals", nullptr, &cheatsBonusCrystals)) {
				setBonusCrystals(cheatsBonusCrystals);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Press [Inv Next + Attack] for 100 crystals.");
			}

			if (ImGui::MenuItem("Music Select", nullptr, &cheatsMusicSelect)) {
				setMusicSelect(cheatsMusicSelect);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Unlocks music select in sound options.");
			}

			if (ImGui::MenuItem("Open Level Select", nullptr, false)) {
				api->GotoLevelSelect();
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Open the level select screen.");
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Mods")) {
			anyMenuOpenThisFrame = true;

			if (MenuActions::menuActionRegistrations.empty()) {
				ImGui::MenuItem("(Empty)", nullptr, nullptr, false);
			}

			for (const auto &registration : MenuActions::menuActionRegistrations) {
				Mod *mod = GetModByHandle(registration.handle);
				std::string category = mod ? WStringToString(mod->getName()) : "Unknown";

				if (ImGui::BeginMenu(category.c_str())) {
					MenuActionRegistration action = registration.function();

					ImGui::BeginDisabled(!action.enabled);
					if (ImGui::MenuItem(action.label != nullptr ? action.label : "")) {
						if (action.callback)
							action.callback();
					}
					if (ImGui::IsItemHovered() && action.tooltip != nullptr && action.tooltip[0] != '\0') {
						ImGui::SetTooltip(action.tooltip);
					}
					ImGui::EndDisabled();

					ImGui::EndMenu();
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	menuOpen = anyMenuOpenThisFrame;

	ImGui::PopStyleVar();
}

} // namespace Overlay::MenuBar
