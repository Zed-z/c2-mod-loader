#include "Overlay.h"

#include "Components/CameraStack.h"
#include "Components/Coords.h"
#include "Components/Inputs.h"
#include "Components/LevelInfo.h"
#include "Components/LevelSelect.h"
#include "Components/Log.h"
#include "Components/MenuBar.h"
#include "Components/Minimap.h"
#include "Components/ObjectList.h"
#include "Components/SaveSlotList.h"
#include "Components/Toast.h"
#include "Components/VersionInfo.h"
#include "Loader.h"
#include "Overlay/Backend.h"
#include "Utils/Style.h"
#include "imgui.h"

#include <windows.h>

namespace Overlay {

bool guiEnabled;
bool showWindows;

void Setup(HMODULE hModule) {
	guiEnabled = api->SetupIniBool(L"GUI", L"GuiEnabled", true);
	showWindows = api->SetupIniBool(L"GUI", L"ShowWindows", true);

	Overlay::Coords::Setup();
	Overlay::InputsComponent::Setup();
	Overlay::LevelInfoComponent::Setup();
	Overlay::LevelSelect::Setup();
	Overlay::Log::Setup();
	Overlay::ObjectList::Setup();
	Overlay::SaveSlotList::Setup();
	Overlay::Toast::Setup();
	Overlay::CameraStack::Setup();
	Overlay::Minimap::Setup();
	
	if (guiEnabled) {
		CreateThread(nullptr, 0, Overlay::Backend::OverlayInitThread, hModule, 0, nullptr);
	}
}

void Draw() {
	Style::ApplyStyle();

	ImGuiIO &io = ImGui::GetIO();
	if (ImGui::IsKeyPressed(ImGuiKey_Tab) && !io.WantCaptureKeyboard) {
		showWindows = !showWindows;
		api->WriteIniBool(L"GUI", L"ShowWindows", showWindows);
	}

	Overlay::VersionInfo::RenderVersionInfo();
	Overlay::MenuBar::RenderMenuBar();

	if (showWindows) {
		Overlay::Coords::RenderCoords();
		Overlay::InputsComponent::RenderInputs();
		Overlay::LevelInfoComponent::RenderLevelInfo();
		Overlay::LevelSelect::RenderLevelSelect();
		Overlay::Log::RenderLog();
		Overlay::ObjectList::RenderObjectList();
		Overlay::SaveSlotList::RenderSaveSlotList();
		Overlay::CameraStack::RenderCameraStack();
		Overlay::Minimap::RenderMinimap();
	}

	Overlay::Toast::RenderToasts();
}

} // namespace Overlay
