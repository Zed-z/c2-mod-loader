#include "Style.h"

namespace Style {

const Style &GetStyle() {
	static Style style;
	return style;
}

void ApplyStyle() {
	const Style &style = GetStyle();
	ImGuiStyle &imguiStyle = ImGui::GetStyle();

	imguiStyle.Colors[ImGuiCol_WindowBg] = style.windowBg;
	imguiStyle.Colors[ImGuiCol_TitleBg] = style.titleBg;
	imguiStyle.Colors[ImGuiCol_TitleBgActive] = style.titleBgActive;
	imguiStyle.Colors[ImGuiCol_TitleBgCollapsed] = style.titleBgCollapsed;
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
	imguiStyle.Colors[ImGuiCol_ResizeGrip] = style.resizeGrip;
	imguiStyle.Colors[ImGuiCol_ResizeGripHovered] = style.resizeGripHovered;
	imguiStyle.Colors[ImGuiCol_ResizeGripActive] = style.resizeGripActive;
	imguiStyle.Colors[ImGuiCol_Separator] = style.separator;
	imguiStyle.Colors[ImGuiCol_SeparatorHovered] = style.separatorHovered;
	imguiStyle.Colors[ImGuiCol_SeparatorActive] = style.separatorActive;
	imguiStyle.Colors[ImGuiCol_SliderGrab] = style.sliderGrab;
	imguiStyle.Colors[ImGuiCol_SliderGrabActive] = style.sliderGrabActive;
	imguiStyle.Colors[ImGuiCol_CheckMark] = style.checkMark;
	imguiStyle.Colors[ImGuiCol_ModalWindowDimBg] = style.modalWindowDimBg;
	imguiStyle.Colors[ImGuiCol_TextDisabled] = style.textDisabled;
}

} // namespace Style
