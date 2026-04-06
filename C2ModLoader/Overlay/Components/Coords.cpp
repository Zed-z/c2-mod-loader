#include "Coords.h"

#include "ModApi.h"
#include "imgui.h"

#include <sstream>

extern ModApi *api;

bool showCoords;

void RenderCoords() {
	if (!showCoords)
		return;

	ImGui::Begin("Coords");

	StratEntity *croc = api->GetEntity(ADDR_CROC_OBJ);
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

	ImGui::End();
}
