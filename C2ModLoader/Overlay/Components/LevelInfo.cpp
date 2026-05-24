#include "LevelInfo.h"

#include "ModApi.h"
#include "Utils/Fonts.h"
#include "imgui.h"

#include <sstream>

extern ModApi *api;

namespace Overlay::LevelInfoComponent {

bool showLevelInfo;

void Setup() {
	showLevelInfo = api->SetupIniBool(L"GUI", L"ShowLevelInfo", false);
}

void RenderLevelInfo() {
	if (!showLevelInfo)
		return;

	bool prevShow = showLevelInfo;

	LevelInfo levelInfo = api->GetLevelInfo();

	ImGui::Begin("Level Info", &showLevelInfo);
	ImGui::PushFont(Fonts::GetFontCode());
	std::stringstream ss;
	ss << "Game State: " << gameStateNames[*gameState] << std::endl;
	ss << "Tribe: " << levelInfo.tribe << std::endl;
	ss << "Level: " << levelInfo.level << std::endl;
	ss << "Map: " << levelInfo.map << std::endl;
	ss << "Type: " << wadFileTypeNames[levelInfo.type + 1] << std::endl;
	ImGui::Text(ss.str().c_str());
	ImGui::PopFont();
	ImGui::End();

	if (prevShow != showLevelInfo) {
		api->WriteIniBool(L"GUI", L"ShowLevelInfo", showLevelInfo);
	}
}

} // namespace Overlay::LevelInfoComponent
