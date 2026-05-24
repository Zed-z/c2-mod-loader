#include "Coords.h"

#include "ModApi.h"
#include "Utils/Fonts.h"
#include "imgui.h"

#include <sstream>

extern ModApi *api;

namespace Overlay::Coords {

bool showCoords;

void Setup() {
	showCoords = api->SetupIniBool(L"GUI", L"ShowCoords", false);
}

void RenderCoords() {
	if (!showCoords)
		return;

	bool prevShow = showCoords;

	ImGui::Begin("Coords", &showCoords);
	ImGui::PushFont(Fonts::GetFontCode());

	StratEntity *croc = api->GetEntity(crocObjRef);
	if (croc == nullptr) {
		ImGui::Text("Unavailable!");
	} else {
		std::stringstream ss;
		ss << "X: " << croc->newPosition.x << std::endl;
		ss << "Y: " << croc->newPosition.y << std::endl;
		ss << "Z: " << croc->newPosition.z << std::endl;
		ss << "Rot: " << croc->newRotation.y << std::endl;
		ImGui::Text(ss.str().c_str());
	}

	ImGui::PopFont();
	ImGui::End();

	if (prevShow != showCoords) {
		api->WriteIniBool(L"GUI", L"ShowCoords", showCoords);
	}
}

} // namespace Overlay::Coords
