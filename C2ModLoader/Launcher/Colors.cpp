#include "Colors.h"

namespace Launcher {

const Style &GetStyle() {
	static Style style;
	return style;
}

void ApplyStyle() {
	const Style &style = GetStyle();
	ImGuiStyle &imguiStyle = ImGui::GetStyle();

	imguiStyle.Colors[ImGuiCol_WindowBg] = style.windowBg;
	imguiStyle.Colors[ImGuiCol_Button] = style.primary;
	imguiStyle.Colors[ImGuiCol_ButtonHovered] = style.primaryHovered;
	imguiStyle.Colors[ImGuiCol_ButtonActive] = style.primaryActive;
	imguiStyle.Colors[ImGuiCol_Header] = style.header;
	imguiStyle.Colors[ImGuiCol_HeaderHovered] = style.headerHovered;
	imguiStyle.Colors[ImGuiCol_HeaderActive] = style.headerActive;
	imguiStyle.Colors[ImGuiCol_Tab] = style.tab;
	imguiStyle.Colors[ImGuiCol_TabHovered] = style.tabHovered;
	imguiStyle.Colors[ImGuiCol_TabActive] = style.tabActive;
	imguiStyle.Colors[ImGuiCol_FrameBg] = style.primary;
	imguiStyle.Colors[ImGuiCol_FrameBgHovered] = style.primaryHovered;
	imguiStyle.Colors[ImGuiCol_FrameBgActive] = style.primaryActive;
	imguiStyle.Colors[ImGuiCol_CheckMark] = style.checkMark;
}

} // namespace Launcher
