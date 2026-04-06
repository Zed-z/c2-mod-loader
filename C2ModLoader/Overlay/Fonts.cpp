#include "Fonts.h"

#include "Utils.h"

#include "imgui.h"

constexpr float fontSizeTitle = 22.0f;
constexpr float fontSizeText = 16.0f;
constexpr float fontSizeCode = 16.0f;

namespace Fonts {

ImFont *GetFontTitle() {
	static ImFont *fontTitle = LoadTextFont(IDR_FONT_TITLE, fontSizeTitle);
	return fontTitle;
}

ImFont *GetFontText() {
	static ImFont *fontText = LoadTextFont(IDR_FONT_TEXT, fontSizeText);
	return fontText;
}

ImFont *GetFontCode() {
	static ImFont *fontCode = LoadTextFont(IDR_FONT_CODE, fontSizeCode);
	return fontCode;
}

} // namespace Fonts
