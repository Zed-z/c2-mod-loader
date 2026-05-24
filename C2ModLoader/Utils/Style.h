#pragma once

#include <imgui.h>

namespace Style {

struct Style {
	ImVec4 windowBg = ImVec4(0.105f, 0.105f, 0.1f, 0.9f);
	ImVec4 titleBg = ImVec4(0.105f, 0.105f, 0.10f, 1.0f);
	ImVec4 titleBgActive = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	ImVec4 titleBgCollapsed = ImVec4(0.105f, 0.105f, 0.10f, 1.0f);
	ImVec4 primary = ImVec4(0.30f, 0.55f, 0.15f, 1.0f);
	ImVec4 primaryHovered = ImVec4(0.40f, 0.65f, 0.25f, 1.0f);
	ImVec4 primaryActive = ImVec4(0.50f, 0.75f, 0.35f, 1.0f);
	ImVec4 header = ImVec4(0.10f, 0.35f, 0.0f, 1.0f);
	ImVec4 headerHovered = ImVec4(0.20f, 0.45f, 0.10f, 1.0f);
	ImVec4 headerActive = ImVec4(0.15f, 0.40f, 0.05f, 1.0f);
	ImVec4 tab = ImVec4(0.10f, 0.35f, 0.0f, 1.0f);
	ImVec4 tabHovered = ImVec4(0.40f, 0.65f, 0.25f, 1.0f);
	ImVec4 tabActive = ImVec4(0.30f, 0.55f, 0.15f, 1.0f);
	ImVec4 resizeGrip = ImVec4(0.30f, 0.55f, 0.15f, 1.0f);
	ImVec4 resizeGripHovered = ImVec4(0.40f, 0.65f, 0.25f, 1.0f);
	ImVec4 resizeGripActive = ImVec4(0.50f, 0.75f, 0.35f, 1.0f);
	ImVec4 separator = ImVec4(0.30f, 0.55f, 0.15f, 1.0f);
	ImVec4 separatorHovered = ImVec4(0.40f, 0.65f, 0.25f, 1.0f);
	ImVec4 separatorActive = ImVec4(0.50f, 0.75f, 0.35f, 1.0f);
	ImVec4 link = ImVec4(0.40f, 0.65f, 0.25f, 1.0f);
	ImVec4 checkMark = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	ImVec4 modalWindowDimBg = ImVec4(0.10f, 0.10f, 0.10f, 0.75f);
	ImVec4 textDisabled = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
};

const Style &GetStyle();
void ApplyStyle();

} // namespace Style
