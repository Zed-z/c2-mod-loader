#include "CameraStack.h"

#include "ModApi.h"
#include "Utils/Fonts.h"
#include "imgui.h"

#include <sstream>
#include <string>

extern ModApi *api;

namespace Overlay::CameraStack {

bool showCameraStack;

void Setup() {
	showCameraStack = api->SetupIniBool(L"GUI", L"ShowCameraStack", false);
}

void RenderCameraStack() {
	if (!showCameraStack)
		return;

	bool prevShow = showCameraStack;

	ImGui::Begin("Camera Stack", &showCameraStack);
	ImGui::PushFont(Fonts::GetFontCode());

	if (
		currentWadFile == nullptr || *currentWadFile == nullptr ||
		(*currentWadFile)->chunkData == nullptr) {
		ImGui::Text("Camera stack unavailable.");
		ImGui::PopFont();
		ImGui::End();
		return;
	}
	WadChunkData chunkData = (*currentWadFile)->chunkData->data;
	auto stack = chunkData.cameraStack;
	int stackCount = chunkData.cameraStackCount;

	ImGui::Text("Entries: %d", stackCount);
	ImGui::Separator();

	for (int i = 0; i < stackCount; i++) {
		CameraStackEntry &entry = stack[i];
		std::string headerId = std::string("camera_stack_") + std::to_string(i);
		std::string headerLabel = std::string("Camera ") + std::to_string(i) + "###" + headerId;

		if (ImGui::CollapsingHeader(headerLabel.c_str()), nullptr, ImGuiTreeNodeFlags_DefaultOpen) {
			ImGui::Indent();
			ImGui::Text("Target: %s", entry.target != nullptr ? entry.target->name : "null");
			ImGui::Text("Camera: %s", entry.camera != nullptr ? entry.camera->name : "null");
			ImGui::Separator();
			ImGui::Text("RotPos0");
			ImGui::Indent();
			ImGui::Text("Position: X=%d, Y=%d, Z=%d", entry.rotPos0.position.x, entry.rotPos0.position.y, entry.rotPos0.position.z);
			ImGui::Text("Rotation: X=%d, Y=%d, Z=%d", entry.rotPos0.rotation.x, entry.rotPos0.rotation.y, entry.rotPos0.rotation.z);
			ImGui::Unindent();
			ImGui::Text("RotPos1");
			ImGui::Indent();
			ImGui::Text("Position: X=%d, Y=%d, Z=%d", entry.rotPos1.position.x, entry.rotPos1.position.y, entry.rotPos1.position.z);
			ImGui::Text("Rotation: X=%d, Y=%d, Z=%d", entry.rotPos1.rotation.x, entry.rotPos1.rotation.y, entry.rotPos1.rotation.z);
			ImGui::Unindent();
			ImGui::Text("Goto");
			ImGui::Indent();
			ImGui::Text("X=%d  Y=%d  Z=%d", entry.Goto.x, entry.Goto.y, entry.Goto.z);
			ImGui::Unindent();
			ImGui::Unindent();
		}
	}

	ImGui::PopFont();
	ImGui::End();

	if (prevShow != showCameraStack) {
		api->WriteIniBool(L"GUI", L"ShowCameraStack", showCameraStack);
	}
}

} // namespace Overlay::CameraStack
