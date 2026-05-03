#include "ObjectList.h"

#include "ModApi.h"
#include "Utils/Fonts.h"
#include "Utils/Style.h"
#include "imgui.h"

#include <sstream>
#include <string>

#include <cmath>
using std::sqrt, std::pow;

extern ModApi *api;

bool showObjectList;

void RenderObjectList() {
	if (!showObjectList)
		return;

	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 400), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::Begin("Object List");

	bool isWide = ImGui::GetContentRegionAvail().x > 400.0f;

	float itemWidth;

	StratEntity *rootObject = api->GetEntity(rootObjRef);
	StratEntity *crocObject = api->GetEntity(crocObjRef);
	StratEntity *cameraObject = api->GetEntity(cameraObjRef);
	StratEntity *dialogObject = api->GetEntity(dialogObjRef);
	int stratCountValue = *stratCount;

	if (rootObject != nullptr) {
		if (rootObject->next != nullptr) {

			// Reverse to the beginning of the list
			StratEntity *node = rootObject->next;
			while (node->prev != nullptr) {
				node = node->prev;
			}

			// Max distance to player
			StratEntity *distanceNode = node;
			int maxDistanceToPlayer = 0;
			if (crocObject == nullptr) {
				maxDistanceToPlayer = -1;
			} else {
				while (distanceNode != nullptr) {

					int playerDistance = static_cast<int>(std::sqrt(
						std::pow(distanceNode->newPosition.x - crocObject->newPosition.x, 2) + std::pow(distanceNode->newPosition.y - crocObject->newPosition.y, 2) + std::pow(distanceNode->newPosition.z - crocObject->newPosition.z, 2)));

					if (playerDistance > maxDistanceToPlayer) {
						maxDistanceToPlayer = playerDistance;
					}

					distanceNode = distanceNode->next;
				}
			}

			ImGui::Text(("Object count: " + std::to_string(stratCountValue)).c_str());

			ImGui::PushFont(Fonts::GetFontCode());
			while (node != nullptr) {

				int playerDistance = (crocObject != nullptr)
					? static_cast<int>(std::sqrt(
						  std::pow(node->newPosition.x - crocObject->newPosition.x, 2) + std::pow(node->newPosition.y - crocObject->newPosition.y, 2) + std::pow(node->newPosition.z - crocObject->newPosition.z, 2)))
					: 0;

				std::ostringstream ss;
				ss << "(" << std::hex << std::uppercase << (uintptr_t)node << ") ";
				ss << std::nouppercase << node->name;
				if (node == crocObject) {
					ss << " [Player]";
				}
				if (node == cameraObject) {
					ss << " [Camera]";
				}
				if (node == dialogObject) {
					ss << " [Dialog]";
				}

				// Header colors
				float distanceModifier = maxDistanceToPlayer != -1 ? (1 - ((float)playerDistance / (float)maxDistanceToPlayer)) : 1;

				const Style::Style &style = Style::GetStyle();
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(style.header.x * distanceModifier, style.header.y * distanceModifier, style.header.z * distanceModifier, style.header.w));
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(style.headerHovered.x * distanceModifier, style.headerHovered.y * distanceModifier, style.headerHovered.z * distanceModifier, style.headerHovered.w));
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(style.headerActive.x * distanceModifier, style.headerActive.y * distanceModifier, style.headerActive.z * distanceModifier, style.headerActive.w));

				if (ImGui::CollapsingHeader(ss.str().c_str())) {
					ImGui::Indent();

					// Position
					ImGui::Text("Position / Rotation");

					itemWidth = (ImGui::GetContentRegionAvail().x / 3 - ImGui::GetStyle().ItemSpacing.x * 16 / 3);

					if (isWide) {
						ImGui::SetNextItemWidth(itemWidth);
					}
					ImGui::InputInt((std::string("PosX##posx") + ss.str()).c_str(), &(node->newPosition.x), 100, 1000);
					if (isWide) {
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
					}
					ImGui::InputInt((std::string("PosY##posy") + ss.str()).c_str(), &(node->newPosition.y), 100, 1000);
					if (isWide) {
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
					}
					ImGui::InputInt((std::string("PosZ##posz") + ss.str()).c_str(), &(node->newPosition.z), 100, 1000);

					if (isWide) {
						ImGui::SetNextItemWidth(itemWidth);
					}
					ImGui::InputInt((std::string("RotX##rotx") + ss.str()).c_str(), &(node->newRotation.x), 100, 1000);
					if (isWide) {
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
					}
					ImGui::InputInt((std::string("RotY##roty") + ss.str()).c_str(), &(node->newRotation.y), 100, 1000);
					if (isWide) {
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
					}
					ImGui::InputInt((std::string("RotZ##rotz") + ss.str()).c_str(), &(node->newRotation.z), 100, 1000);

					ImGui::Text(("Distance from player: " + std::to_string(node->distanceToPlayer) + " (" + std::to_string(playerDistance) + ")").c_str());

					// More position info
					if (ImGui::CollapsingHeader(("More Position Info##moreposinfo" + ss.str()).c_str())) {

						// Old position
						ImGui::Text("Old Position / Rotation");

						itemWidth = (ImGui::GetContentRegionAvail().x / 3 - ImGui::GetStyle().ItemSpacing.x * 16 / 3);

						if (isWide) {
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("PosX##oldposx") + ss.str()).c_str(), &(node->OldRotPos.position.x), 100, 1000);
						if (isWide) {
							ImGui::SameLine();
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("PosY##oldposy") + ss.str()).c_str(), &(node->OldRotPos.position.y), 100, 1000);
						if (isWide) {
							ImGui::SameLine();
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("PosZ##oldposz") + ss.str()).c_str(), &(node->OldRotPos.position.z), 100, 1000);

						if (isWide) {
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("RotX##oldrotx") + ss.str()).c_str(), &(node->OldRotPos.rotation.x), 100, 1000);
						if (isWide) {
							ImGui::SameLine();
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("RotY##oldroty") + ss.str()).c_str(), &(node->OldRotPos.rotation.y), 100, 1000);
						if (isWide) {
							ImGui::SameLine();
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("RotZ##oldrotz") + ss.str()).c_str(), &(node->OldRotPos.rotation.z), 100, 1000);

						// Start Position
						ImGui::Text("Start Position");
						if (isWide) {
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("PosX##startposx") + ss.str()).c_str(), &(node->StartRotPos.position.x), 100, 1000);
						if (isWide) {
							ImGui::SameLine();
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("PosY##startposy") + ss.str()).c_str(), &(node->StartRotPos.position.y), 100, 1000);
						if (isWide) {
							ImGui::SameLine();
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("PosZ##startposz") + ss.str()).c_str(), &(node->StartRotPos.position.z), 100, 1000);

						if (isWide) {
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("RotX##startrotx") + ss.str()).c_str(), &(node->StartRotPos.rotation.x), 100, 1000);
						if (isWide) {
							ImGui::SameLine();
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("RotY##startroty") + ss.str()).c_str(), &(node->StartRotPos.rotation.y), 100, 1000);
						if (isWide) {
							ImGui::SameLine();
							ImGui::SetNextItemWidth(itemWidth);
						}
						ImGui::InputInt((std::string("RotZ##startrotz") + ss.str()).c_str(), &(node->StartRotPos.rotation.z), 100, 1000);
					}

					// Model
					ImGui::Text("Model Data");
					uintptr_t modelAddress = reinterpret_cast<uintptr_t>(node->model);
					if (ImGui::InputScalar((std::string("Model Address##model_addr") + ss.str()).c_str(), ImGuiDataType_U32, &modelAddress, NULL, NULL, "%X", ImGuiSliderFlags_None)) {
						node->model = reinterpret_cast<void *>(modelAddress);
					}

					uintptr_t animationAddress = reinterpret_cast<uintptr_t>(node->animation);
					if (ImGui::InputScalar((std::string("Animation Address##animation_addr") + ss.str()).c_str(), ImGuiDataType_U32, &animationAddress, NULL, NULL, "%X", ImGuiSliderFlags_None)) {
						node->animation = reinterpret_cast<void *>(animationAddress);
					}

					// Scale
					ImGui::Text("Scale");

					itemWidth = (ImGui::GetContentRegionAvail().x / 3 - ImGui::GetStyle().ItemSpacing.x * 8 / 3);

					if (isWide) {
						ImGui::SetNextItemWidth(itemWidth);
					}
					ImGui::InputInt((std::string("X##scalex") + ss.str()).c_str(), &(node->scale.x), 128, 4096);
					if (isWide) {
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
					}
					ImGui::InputInt((std::string("Y##scaley") + ss.str()).c_str(), &(node->scale.y), 128, 4096);
					if (isWide) {
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
					}
					ImGui::InputInt((std::string("Z##scalez") + ss.str()).c_str(), &(node->scale.z), 128, 4096);

					// Actions
					ImGui::Text("Actions");

					if (ImGui::Button((std::string("Teleport Here##tphere") + ss.str()).c_str())) {
						if (crocObject != nullptr) {
							node->newPosition.x = crocObject->newPosition.x;
							node->newPosition.y = crocObject->newPosition.y;
							node->newPosition.z = crocObject->newPosition.z;
						}
					}

					ImGui::SameLine();

					if (ImGui::Button((std::string("Teleport Start Here##tpstarthere") + ss.str()).c_str())) {
						if (crocObject != nullptr) {
							node->StartRotPos.position.x = crocObject->newPosition.x;
							node->StartRotPos.position.y = crocObject->newPosition.y;
							node->StartRotPos.position.z = crocObject->newPosition.z;
						}
					}

					if (ImGui::Button((std::string("Teleport To##tphere") + ss.str()).c_str())) {
						if (crocObject != nullptr) {
							crocObject->newPosition.x = node->newPosition.x;
							crocObject->newPosition.y = node->newPosition.y;
							crocObject->newPosition.z = node->newPosition.z;
						}
					}

					ImGui::SameLine();

					if (ImGui::Button((std::string("Teleport To Start##tpstarthere") + ss.str()).c_str())) {
						if (crocObject != nullptr) {
							crocObject->newPosition.x = node->StartRotPos.position.x;
							crocObject->newPosition.y = node->StartRotPos.position.y;
							crocObject->newPosition.z = node->StartRotPos.position.z;
						}
					}

					// Freeze
					{
						int flagMask = (1 << 4);
						bool isFlagSet = (node->flags0 & flagMask) != 0;
						if (ImGui::Checkbox(("Freeze##freezecheckbox" + ss.str()).c_str(), &isFlagSet)) {
							if (isFlagSet) {
								node->flags0 |= flagMask;
							} else {
								node->flags0 &= ~flagMask;
							}
						}
					}

					ImGui::SameLine();

					// Invisible
					{
						int flagMask = (1 << 24);
						bool isFlagSet = (node->flags0 & flagMask) != 0;
						if (ImGui::Checkbox(("Invisible##invisiblecheckbox" + ss.str()).c_str(), &isFlagSet)) {
							if (isFlagSet) {
								node->flags0 |= flagMask;
							} else {
								node->flags0 &= ~flagMask;
							}
						}
					}

					// Local Variables
					if (ImGui::CollapsingHeader(("Local Variables##localvars" + ss.str()).c_str())) {

						ImGui::Text("Flags");
						ImGui::InputInt(("Flags 0" + std::string("##flags0") + ss.str()).c_str(), &(node->flags0), 0, 0);

						for (int i = 0; i < 32; i++) {
							std::string label = isWide ? ((i < 10 ? " " : "") + std::to_string(i)) : "";
							std::string checkboxLabel = label + "##flags0flag" + std::to_string(i) + ss.str();
							int flagMask = (1 << i);
							bool isFlagSet = (node->flags0 & flagMask) != 0;

							if (ImGui::Checkbox(checkboxLabel.c_str(), &isFlagSet)) {
								if (isFlagSet) {
									node->flags0 |= flagMask;
								} else {
									node->flags0 &= ~flagMask;
								}
							}
							if (ImGui::IsItemHovered()) {
								std::string tooltipText = "Flag 0:" + std::to_string(i);
								ImGui::SetTooltip(tooltipText.c_str());
							}

							if ((i + 1) % 8 != 0 && i < 31) {
								ImGui::SameLine();
							}
						}

						ImGui::InputInt(("Flags 1" + std::string("##flags1") + ss.str()).c_str(), &(node->flags1), 0, 0);

						for (int i = 0; i < 32; i++) {
							std::string label = isWide ? ((i < 10 ? " " : "") + std::to_string(i)) : "";
							std::string checkboxLabel = label + "##flags1flag" + std::to_string(i) + ss.str();
							int flagMask = (1 << i);
							bool isFlagSet = (node->flags1 & flagMask) != 0;

							if (ImGui::Checkbox(checkboxLabel.c_str(), &isFlagSet)) {
								if (isFlagSet) {
									node->flags1 |= flagMask;
								} else {
									node->flags1 &= ~flagMask;
								}
							}
							if (ImGui::IsItemHovered()) {
								std::string tooltipText = "Flag 1:" + std::to_string(i);
								ImGui::SetTooltip(tooltipText.c_str());
							}

							if ((i + 1) % 8 != 0 && i < 31) {
								ImGui::SameLine();
							}
						}

						ImGui::Text("Local Variables");
						itemWidth = (ImGui::GetContentRegionAvail().x / 2 - ImGui::GetStyle().ItemSpacing.x * 8 / 3);
						for (int i = 0; i < LOCAL_VAR_COUNT; i++) {
							ImGui::SetNextItemWidth(itemWidth);

							std::string label =
								(i < 10 ? " " : "") + std::to_string(i) + std::string("##localvar") + std::to_string(i) + ss.str();

							ImGui::InputInt(label.c_str(), &(node->localVars->vars[i]), 0, 0);

							if (i % 2 == 0) {
								ImGui::SameLine();
							}
						}
					}

					ImGui::Unindent();
				}

				// Header colors
				ImGui::PopStyleColor(3);

				node = node->next;
			}
			ImGui::PopFont();

			if (ImGui::Button("Dump to Log##logdump")) {
				StratEntity *node = rootObject->next;

				// Reverse to the beginning of the list
				while (node->prev != nullptr) {
					node = node->prev;
				}

				std::ostringstream ss;

				int i = 0;
				while (node != nullptr) {

					ss << "- (" << std::hex << (uintptr_t)node << ") " << node->name;
					ss << "\t\tPos: " << node->newPosition.x << "," << node->newPosition.y << "," << node->newPosition.z;
					ss << "\t\tRot: " << node->newRotation.x << "," << node->newRotation.y << "," << node->newRotation.z;
					ss << "\t\tScale: " << node->scale.x << "," << node->scale.y << "," << node->scale.z;
					ss << "\t\tLocal Vars: ";
					for (int j = 0; j < LOCAL_VAR_COUNT; j++) {
						ss << node->localVars->vars[j] << (j < 19 ? ", " : "");
					}
					ss << std::endl;

					node = node->next;
				}

				std::string objectListLog = "Object List: (" + std::to_string(stratCountValue) + ")\n" + ss.str();
				api->LogInfo(objectListLog.c_str());
			}
		} else {
			ImGui::Text("No objects found!");
		}
	} else {
		ImGui::Text("No objects found!");
	}

	ImGui::End();
}
