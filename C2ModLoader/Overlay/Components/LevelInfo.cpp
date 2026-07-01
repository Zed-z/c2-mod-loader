#include "LevelInfo.h"

#include "ModApi.h"
#include "Utils/Fonts.h"
#include "WadNames.h"
#include "imgui.h"

#include <iomanip>
#include <sstream>
#include <string>

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

	{
		std::stringstream lookupKey;
		lookupKey << "wads/t" << levelInfo.tribe;
		switch (levelInfo.type) {
		case WAD_TYPE_LEVEL:
			lookupKey << "l";
			break;
		case WAD_TYPE_CUTSCENE:
			lookupKey << "i";
			break;
		case WAD_TYPE_BOSS:
			lookupKey << "b";
			break;
		}
		lookupKey << std::to_string(levelInfo.level);
		lookupKey << "m";
		lookupKey << std::setfill('0') << std::setw(3) << levelInfo.map;
		lookupKey << ".wad";

		if (wadNames.find(lookupKey.str()) != wadNames.end()) {
			std::string wadName = wadNames[lookupKey.str()];
			ImGui::Text(wadName.c_str());
			ImGui::Separator();
		}
	}

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
