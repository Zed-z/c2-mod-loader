#include "Fonts.h"

#include "Resource.h"
#include "Utils/ResourceLoader.h"
#include "imgui.h"

#include <unordered_map>

constexpr float fontSizeTitle = 22.0f;
constexpr float fontSizeText = 16.0f;
constexpr float fontSizeCode = 16.0f;

namespace Fonts {

namespace {

struct FontSet {
	ImFont *title = nullptr;
	ImFont *text = nullptr;
	ImFont *code = nullptr;
};

static std::unordered_map<ImGuiContext *, FontSet> g_fontsByContext;

FontSet &GetFontSetForCurrentContext() {
	ImGuiContext *context = ImGui::GetCurrentContext();
	return g_fontsByContext[context];
}

} // namespace

ImFont *GetFontTitle() {
	FontSet &fonts = GetFontSetForCurrentContext();
	if (!fonts.title) {
		fonts.title = ResourceLoader::LoadTextFont(nullptr, IDR_FONT_TITLE, fontSizeTitle);
	}
	return fonts.title;
}

ImFont *GetFontText() {
	FontSet &fonts = GetFontSetForCurrentContext();
	if (!fonts.text) {
		fonts.text = ResourceLoader::LoadTextFont(nullptr, IDR_FONT_TEXT, fontSizeText);
	}
	return fonts.text;
}

ImFont *GetFontCode() {
	FontSet &fonts = GetFontSetForCurrentContext();
	if (!fonts.code) {
		fonts.code = ResourceLoader::LoadTextFont(nullptr, IDR_FONT_CODE, fontSizeCode);
	}
	return fonts.code;
}

} // namespace Fonts
