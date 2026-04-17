#include "LevelInfo.h"

#include "ModApi.h"
#include "Utils/Fonts.h"
#include "imgui.h"

#include <sstream>

extern ModApi *api;

bool showLevelInfo;

void RenderLevelInfo() {
	if (!showLevelInfo)
		return;

	LevelInfo levelInfo = api->GetLevelInfo();

	ImGui::Begin("Level Info");
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
}
