#include "VersionInfo.h"

#include "Loader.h"
#include "ModApi.h"
#include "Overlay/Backend.h"
#include "Resource.h"
#include "Utils/Fonts.h"
#include "imgui.h"

#include <string>

extern ModApi *api;

void RenderVersionInfo() {
	ImGuiIO &io = ImGui::GetIO();

	LevelInfo levelInfo = api->GetLevelInfo();
	if (levelInfo.tribe == 0 && levelInfo.level == 0) {
		ImFont *fontTitle = Fonts::GetFontTitle();

		std::string labelText = std::string(LOADER_NAME " v" LOADER_VERSION) + "\n" + "Mods loaded: " + std::to_string(modsLoaded);
		char *labelTextChar = const_cast<char *>(labelText.c_str());
		ImVec2 labelSize = fontTitle->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0f, labelTextChar);

		float displayScale = io.DisplaySize.y / 720.0f;
		float fontSize = 32.0f * displayScale;

		const float margin = 48.0f * displayScale;
		float labelX = margin;
		float labelY = io.DisplaySize.y - margin - labelSize.y * 2 * displayScale / Overlay::Backend::dpiScale;
		float labelStrokeWidth = 2.0f * displayScale;

		int labelOpacity = levelInfo.map == 0 ? 255 : 63;
		ImU32 labelColor = IM_COL32(254, 254, 200, labelOpacity);
		ImU32 labelStrokeColor = IM_COL32(101, 81, 24, labelOpacity);

		ImDrawList *drawList = ImGui::GetBackgroundDrawList();
		drawList->AddText(fontTitle, fontSize, ImVec2(labelX - labelStrokeWidth, labelY), labelStrokeColor, labelTextChar);
		drawList->AddText(fontTitle, fontSize, ImVec2(labelX + labelStrokeWidth, labelY), labelStrokeColor, labelTextChar);
		drawList->AddText(fontTitle, fontSize, ImVec2(labelX, labelY - labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(fontTitle, fontSize, ImVec2(labelX, labelY + labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(fontTitle, fontSize, ImVec2(labelX - labelStrokeWidth, labelY - labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(fontTitle, fontSize, ImVec2(labelX - labelStrokeWidth, labelY + labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(fontTitle, fontSize, ImVec2(labelX + labelStrokeWidth, labelY - labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(fontTitle, fontSize, ImVec2(labelX + labelStrokeWidth, labelY + labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(fontTitle, fontSize, ImVec2(labelX, labelY), labelColor, labelTextChar);
	}
}
