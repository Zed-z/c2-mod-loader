#include "LevelInfo.h"

#include "ModApi.h"
#include "imgui.h"

#include <sstream>

extern ModApi *api;

bool showLevelInfo;

void RenderLevelInfo() {
	if (!showLevelInfo)
		return;

	LevelInfo levelInfo = api->GetLevelInfo();

	ImGui::Begin("Level Info");
	std::stringstream ss;
	ss << "Tribe: " << levelInfo.tribe << std::endl;
	ss << "Level: " << levelInfo.level << std::endl;
	ss << "Map: " << levelInfo.map << std::endl;
	ImGui::Text(ss.str().c_str());
	ImGui::End();
}
