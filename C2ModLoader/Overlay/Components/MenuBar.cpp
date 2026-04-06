#include "MenuBar.h"

#include "CheatsManager.h"
#include "Loader.h"
#include "ModApi.h"
#include "Overlay/Components/Coords.h"
#include "Overlay/Components/Inputs.h"
#include "Overlay/Components/LevelInfo.h"
#include "Overlay/Components/Log.h"
#include "Overlay/Components/ObjectList.h"
#include "Overlay/Components/SaveSlotList.h"
#include "Overlay/Components/Toast.h"
#include "Overlay/Components/VersionInfo.h"
#include "Utils.h"
#include "imgui.h"

#include <string>
#include <vector>

extern ModApi *api;

std::vector<MenuAction> menuActionRegistrations;

bool ImGuiRegisterMenuAction(HMODULE handle, MenuActionRegistrationFunction registration) {
	menuActionRegistrations.push_back({handle, registration});
	return true;
}

void RenderMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Mod Loader")) {
			if (ImGui::MenuItem("Show Log", nullptr, &showLog)) {
				api->WriteIniBool(L"GUI", L"ShowLog", showLog);
			}
			if (ImGui::BeginMenu("Logging")) {

				if (ImGui::MenuItem("Show Info", nullptr, &showLogInfo)) {
					api->WriteIniBool(L"Logging", L"Info", showLogInfo);
				}
				if (ImGui::MenuItem("Show Debug", nullptr, &showLogDebug)) {
					api->WriteIniBool(L"Logging", L"Debug", showLogDebug);
				}
				if (ImGui::MenuItem("Show Warnings", nullptr, &showLogWarning)) {
					api->WriteIniBool(L"Logging", L"Warning", showLogWarning);
				}
				if (ImGui::MenuItem("Show Errors", nullptr, &showLogError)) {
					api->WriteIniBool(L"Logging", L"Error", showLogError);
				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Toasts")) {

				if (ImGui::MenuItem("Show Info", nullptr, &showToastInfo)) {
					api->WriteIniBool(L"Toasts", L"Info", showToastInfo);
				}
				if (ImGui::MenuItem("Show Debug", nullptr, &showToastDebug)) {
					api->WriteIniBool(L"Toasts", L"Debug", showToastDebug);
				}
				if (ImGui::MenuItem("Show Warnings", nullptr, &showToastWarning)) {
					api->WriteIniBool(L"Toasts", L"Warning", showToastWarning);
				}
				if (ImGui::MenuItem("Show Errors", nullptr, &showToastError)) {
					api->WriteIniBool(L"Toasts", L"Error", showToastError);
				}

				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Show Inputs", nullptr, &showInputs)) {
				api->WriteIniBool(L"GUI", L"ShowInputs", showInputs);
			}
			if (ImGui::MenuItem("Show Object List", nullptr, &showObjectList)) {
				api->WriteIniBool(L"GUI", L"ShowObjectList", showObjectList);
			}
			if (ImGui::MenuItem("Show Coords", nullptr, &showCoords)) {
				api->WriteIniBool(L"GUI", L"ShowCoords", showCoords);
			}
			if (ImGui::MenuItem("Show Level Info", nullptr, &showLevelInfo)) {
				api->WriteIniBool(L"GUI", L"ShowLevelInfo", showLevelInfo);
			}
			if (ImGui::MenuItem("Show Save Slot List", nullptr, &showSaveSlotList)) {
				api->WriteIniBool(L"GUI", L"ShowSaveSlotList", showSaveSlotList);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Cheats")) {

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

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Mods")) {

			if (menuActionRegistrations.empty()) {
				ImGui::MenuItem("(Empty)", nullptr, nullptr, false);
			}

			for (const auto &registration : menuActionRegistrations) {
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
}
