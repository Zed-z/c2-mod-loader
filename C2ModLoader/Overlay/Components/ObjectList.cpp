#include "ObjectList.h"

#include "ModApi.h"
#include "Utils/Fonts.h"
#include "Utils/Style.h"
#include "imgui.h"

#include <iomanip>
#include <sstream>
#include <string>

#include <cinttypes>

#include <algorithm>
#include <cctype>

#include <cmath>
using std::sqrt, std::pow;

extern ModApi *api;

namespace {

void renderTripleInts(const char *labelx, int *x, const char *labely, int *y, const char *labelz, int *z, const std::string &suffix, float inputW, int step = 1, int stepFast = 10) {
	float avail = ImGui::GetContentRegionAvail().x;
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	float needed = inputW * 3 + spacing * 2;
	if (avail >= needed) {
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labelx) + "##" + suffix).c_str(), x, step, stepFast);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labely) + "##" + suffix).c_str(), y, step, stepFast);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labelz) + "##" + suffix).c_str(), z, step, stepFast);
	} else if (avail >= inputW * 2 + spacing) {
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labelx) + "##" + suffix).c_str(), x, step, stepFast);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labely) + "##" + suffix).c_str(), y, step, stepFast);
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labelz) + "##" + suffix).c_str(), z, step, stepFast);
	} else {
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labelx) + "##" + suffix).c_str(), x, step, stepFast);
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labely) + "##" + suffix).c_str(), y, step, stepFast);
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labelz) + "##" + suffix).c_str(), z, step, stepFast);
	}
}

void renderDualInts(const char *labela, int *a, const char *labelb, int *b, const std::string &suffix, float inputW, int step = 1, int stepFast = 10) {
	float avail = ImGui::GetContentRegionAvail().x;
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	float needed = inputW * 2 + spacing;
	if (avail >= needed) {
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labela) + "##" + suffix).c_str(), a, step, stepFast);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labelb) + "##" + suffix).c_str(), b, step, stepFast);
	} else {
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labela) + "##" + suffix).c_str(), a, step, stepFast);
		ImGui::SetNextItemWidth(inputW);
		ImGui::InputInt((std::string(labelb) + "##" + suffix).c_str(), b, step, stepFast);
	}
}

std::string formatAddress(uintptr_t addr) {
	std::ostringstream ss;
	ss << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(sizeof(uintptr_t) * 2) << addr;
	return ss.str();
}

void RenderObjectDetailsContent(StratEntity *entity, StratEntity *playerObject) {
	if (entity == nullptr) {
		ImGui::Text("Object no longer valid");
		return;
	}

	uintptr_t key = reinterpret_cast<uintptr_t>(entity);
	ImGui::Text("%s (%s)", entity->name != nullptr ? entity->name : "Unknown Object", formatAddress(key).c_str());

	ImGui::Separator();

	bool isWide = ImGui::GetContentRegionAvail().x > 400.0f;
	std::string idSuffix = std::to_string(key);
	float inputWidth = 120.0f;

	ImGui::Text("Position");
	renderTripleInts("X", &(entity->newPosition.x), "Y", &(entity->newPosition.y), "Z", &(entity->newPosition.z), std::string("pos_") + idSuffix, inputWidth, 100, 1000);

	ImGui::Text("Start Position");
	renderTripleInts("X", &(entity->StartRotPos.position.x), "Y", &(entity->StartRotPos.position.y), "Z", &(entity->StartRotPos.position.z), std::string("startpos_") + idSuffix, inputWidth, 100, 1000);

	ImGui::Text("Rotation");
	renderTripleInts("X", &(entity->newRotation.x), "Y", &(entity->newRotation.y), "Z", &(entity->newRotation.z), std::string("rot_") + idSuffix, inputWidth, 100, 1000);

	ImGui::Spacing();

	ImGui::Text((std::string("Distance from player: ") + std::to_string(entity->distanceToPlayer)).c_str());

	ImGui::Separator();

	ImGui::Text("Model / Animation");
	renderDualInts("Model", reinterpret_cast<int *>(&entity->model), "Animation", reinterpret_cast<int *>(&entity->animation), std::string("model_anim_") + idSuffix, inputWidth, 0, 0);

	ImGui::Text("Scale");
	renderTripleInts("X", &(entity->scale.x), "Y", &(entity->scale.y), "Z", &(entity->scale.z), std::string("scale_") + idSuffix, 100, 1000);

	ImGui::Separator();

	ImGui::Text("Actions");
	if (ImGui::Button((std::string("Teleport Here##tphere_") + idSuffix).c_str())) {
		if (playerObject != nullptr) {
			entity->newPosition.x = playerObject->newPosition.x;
			entity->newPosition.y = playerObject->newPosition.y;
			entity->newPosition.z = playerObject->newPosition.z;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button((std::string("Teleport Start Here##tpstarthere_") + idSuffix).c_str())) {
		if (playerObject != nullptr) {
			entity->StartRotPos.position.x = playerObject->newPosition.x;
			entity->StartRotPos.position.y = playerObject->newPosition.y;
			entity->StartRotPos.position.z = playerObject->newPosition.z;
		}
	}

	if (ImGui::Button((std::string("Teleport To##tptot_") + idSuffix).c_str())) {
		if (playerObject != nullptr) {
			playerObject->newPosition.x = entity->newPosition.x;
			playerObject->newPosition.y = entity->newPosition.y;
			playerObject->newPosition.z = entity->newPosition.z;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button((std::string("Teleport To Start##tptostart_") + idSuffix).c_str())) {
		if (playerObject != nullptr) {
			playerObject->newPosition.x = entity->StartRotPos.position.x;
			playerObject->newPosition.y = entity->StartRotPos.position.y;
			playerObject->newPosition.z = entity->StartRotPos.position.z;
		}
	}

	{
		int flagMask = (1 << 4);
		bool isFlagSet = (entity->flags0 & flagMask) != 0;
		if (ImGui::Checkbox((std::string("Freeze##freeze_") + idSuffix).c_str(), &isFlagSet)) {
			if (isFlagSet)
				entity->flags0 |= flagMask;
			else
				entity->flags0 &= ~flagMask;
		}
	}
	ImGui::SameLine();
	{
		int flagMask = (1 << 24);
		bool isFlagSet = (entity->flags0 & flagMask) != 0;
		if (ImGui::Checkbox((std::string("Invisible##invis_") + idSuffix).c_str(), &isFlagSet)) {
			if (isFlagSet)
				entity->flags0 |= flagMask;
			else
				entity->flags0 &= ~flagMask;
		}
	}

	ImGui::Separator();

	ImGui::Text("Flags / Local Variables");

	if (ImGui::CollapsingHeader((std::string("Flags##flags_") + idSuffix).c_str())) {
		ImGui::InputInt((std::string("Flags 0##flags0_") + idSuffix).c_str(), &(entity->flags0), 0, 0);

		for (int i = 0; i < 32; i++) {
			std::string pad = (i < 10) ? "  " : ((i < 100) ? " " : "");
			std::string label = isWide ? (pad + std::to_string(i)) : "";
			std::string checkboxLabel = label + "##flags0flag_" + std::to_string(i) + idSuffix;
			int flagMask = (1 << i);
			bool isFlagSet = (entity->flags0 & flagMask) != 0;

			if (ImGui::Checkbox(checkboxLabel.c_str(), &isFlagSet)) {
				if (isFlagSet)
					entity->flags0 |= flagMask;
				else
					entity->flags0 &= ~flagMask;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip((std::string("Flag 0:") + std::to_string(i)).c_str());
			if ((i + 1) % 8 != 0 && i < 31)
				ImGui::SameLine();
		}

		ImGui::InputInt((std::string("Flags 1##flags1_") + idSuffix).c_str(), &(entity->flags1), 0, 0);
		for (int i = 0; i < 32; i++) {
			std::string pad = (i < 10) ? "  " : ((i < 100) ? " " : "");
			std::string label = isWide ? (pad + std::to_string(i)) : "";
			std::string checkboxLabel = label + "##flags1flag_" + std::to_string(i) + idSuffix;
			int flagMask = (1 << i);
			bool isFlagSet = (entity->flags1 & flagMask) != 0;
			if (ImGui::Checkbox(checkboxLabel.c_str(), &isFlagSet)) {
				if (isFlagSet)
					entity->flags1 |= flagMask;
				else
					entity->flags1 &= ~flagMask;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip((std::string("Flag 1:") + std::to_string(i)).c_str());
			if ((i + 1) % 8 != 0 && i < 31)
				ImGui::SameLine();
		}
	}

	int localVarCount = ((int32_t *)entity->triggers - (int32_t *)entity->localVars) - 20;
	if (ImGui::CollapsingHeader((std::string("Local Variables(") + std::to_string(localVarCount) + ")##localvars_" + idSuffix).c_str())) {
		float availWidth = ImGui::GetContentRegionAvail().x;
		float spacing = ImGui::GetStyle().ItemSpacing.x;

		int availPx = (int)std::floor(availWidth + 0.5f);
		int fixedItemPx = (int)std::ceil(inputWidth);
		int spacingPx = (int)std::ceil(spacing);

		const int labelSafetyMarginPx = 28;
		int itemTotalPx = fixedItemPx + labelSafetyMarginPx;

		int colsPerRow = (availPx + spacingPx) / (itemTotalPx + spacingPx);
		if (colsPerRow < 1)
			colsPerRow = 1;

		for (int i = 0; i < localVarCount; i++) {
			ImGui::SetNextItemWidth(inputWidth);
			std::string pad = (i < 10) ? "  " : ((i < 100) ? " " : "");
			std::string label = pad + std::to_string(i) + std::string("##localvar_") + std::to_string(i) + idSuffix;
			ImGui::InputInt(label.c_str(), &(entity->localVars[i]), 0, 0);
			if ((i + 1) % colsPerRow != 0 && i < localVarCount - 1)
				ImGui::SameLine(0.0f, spacing);
		}
	}
}

} // namespace

namespace Overlay::ObjectList {

bool showObjectList;

void Setup() {
	showObjectList = api->SetupIniBool(L"GUI", L"ShowObjectList", false);
}

void RenderObjectList() {
	if (!showObjectList)
		return;

	bool prevShow = showObjectList;

	ImGui::SetNextWindowSizeConstraints(ImVec2(640, 360), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::Begin("Object List", &showObjectList);

	ImGuiID splitterStorageID = ImGui::GetID("object_list_splitter_w");
	ImGuiStorage *storage = ImGui::GetStateStorage();
	static float listPanelWidth = 260.0f;
	listPanelWidth = storage->GetFloat(splitterStorageID, listPanelWidth);

	bool isWide = ImGui::GetContentRegionAvail().x > 400.0f;

	float itemWidth;

	StratEntity *rootObject = api->GetEntity(rootObjRef);
	StratEntity *crocObject = api->GetEntity(crocObjRef);
	StratEntity *bossObject = api->GetEntity(bossObjRef);
	StratEntity *cameraObject = api->GetEntity(cameraObjRef);
	StratEntity *dialogObject = api->GetEntity(dialogObjRef);
	int stratCountValue = *stratCount;
	int maxDistanceToPlayer = 0;

	static uintptr_t selectedObjectKey = 0;

	auto getPlayerDistance = [&](StratEntity *object) {
		if (object == nullptr || crocObject == nullptr)
			return 0;

		return static_cast<int>(std::sqrt(
			std::pow(object->newPosition.x - crocObject->newPosition.x, 2) + std::pow(object->newPosition.y - crocObject->newPosition.y, 2) + std::pow(object->newPosition.z - crocObject->newPosition.z, 2)));
	};

	auto buildObjectLabel = [&](StratEntity *object, const char *tag) {
		std::ostringstream ss;
		ss << "(" << formatAddress(reinterpret_cast<uintptr_t>(object)) << ") ";
		ss << (object->name ? object->name : "(unnamed)");
		if (tag != nullptr)
			ss << " [" << tag << "]";
		return ss.str();
	};

	auto toLower = [&](const std::string &s) {
		std::string out = s;
		std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
		return out;
	};

	auto renderObjectRow = [&](StratEntity *object, const char *tag, bool special = false) {
		if (object == nullptr) {
			ImGui::TextDisabled("(null)");
			return false;
		}

		int playerDistance = getPlayerDistance(object);
		float distanceModifier = 1.0f;
		if (maxDistanceToPlayer > 0) {
			distanceModifier = 1.0f - ((float)playerDistance / (float)maxDistanceToPlayer);
		} else if (maxDistanceToPlayer == -1) {
			distanceModifier = 1.0f;
		}

		const Style::Style &style = Style::GetStyle();
		ImVec4 bgColor = ImVec4(style.header.x * distanceModifier, style.header.y * distanceModifier, style.header.z * distanceModifier, style.header.w);

		uintptr_t objectKey = reinterpret_cast<uintptr_t>(object);
		std::string rowId = std::string("##") + (special ? "special_objrow_" : "objrow_") + (tag != nullptr ? tag : "obj") + "_" + std::to_string(objectKey);

		bool isSelected = (selectedObjectKey == objectKey);

		ImVec2 itemMin = ImGui::GetCursorScreenPos();
		float itemHeight = ImGui::GetFrameHeight();
		ImVec2 itemSize(ImGui::GetContentRegionAvail().x, itemHeight);
		ImVec2 itemMax(itemMin.x + itemSize.x, itemMin.y + itemSize.y);
		ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, ImGui::GetColorU32(bgColor));

		if (ImGui::Selectable((buildObjectLabel(object, tag) + rowId).c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(itemSize.x, itemSize.y))) {
			selectedObjectKey = objectKey;
		}

		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("%s", object->name ? object->name : "(unnamed)");
			ImGui::Separator();
			ImGui::Text("Address: %s", formatAddress(objectKey).c_str());
			ImGui::Separator();
			ImGui::Text("Position: X=%d  Y=%d  Z=%d", object->newPosition.x, object->newPosition.y, object->newPosition.z);
			ImGui::Text("Rotation: X=%d  Y=%d  Z=%d", object->newRotation.x, object->newRotation.y, object->newRotation.z);
			ImGui::Text("Scale:    X=%d  Y=%d  Z=%d", object->scale.x, object->scale.y, object->scale.z);
			ImGui::Separator();
			int localVarCount = ((int32_t *)object->triggers - (int32_t *)object->localVars) - 20;
			ImGui::Text("Local vars: %d", localVarCount);
			ImGui::EndTooltip();
		}

		return isSelected;
	};

	ImGui::PushFont(Fonts::GetFontCode());

	ImVec2 availSize = ImGui::GetContentRegionAvail();

	const float splitterHitW = 8.0f;
	const float minDetailsWidth = 320.0f;
	const float minListWidth = 260.0f;
	const float absoluteMinListWidth = 100.0f;

	int availPx = (int)std::floor(availSize.x + 0.5f);
	int splitterPx = (int)std::ceil(splitterHitW);
	int minDetailsPx = (int)std::ceil(minDetailsWidth);
	int minListPx = (int)std::ceil(minListWidth);
	int absoluteMinListPx = (int)std::ceil(absoluteMinListWidth);

	int totalMinPx = minListPx + minDetailsPx + splitterPx;
	int maxListPx = availPx - minDetailsPx - splitterPx;
	if (availPx >= totalMinPx) {
		if ((int)std::round(listPanelWidth) > maxListPx)
			listPanelWidth = (float)maxListPx;
		if ((int)std::round(listPanelWidth) < minListPx)
			listPanelWidth = (float)minListPx;
	} else {
		int desiredListPx = availPx - minDetailsPx - splitterPx;
		if (desiredListPx < absoluteMinListPx)
			desiredListPx = absoluteMinListPx;
		listPanelWidth = (float)desiredListPx;
		maxListPx = availPx - minDetailsPx - splitterPx;
	}

	// List panel
	ImGui::BeginChild("ObjectListPanel", ImVec2(listPanelWidth, availSize.y), false);

	ImGui::Text("Special Objects");
	ImGui::Separator();

	auto considerDistance = [&](StratEntity *object) {
		if (object == nullptr)
			return;

		int playerDistance = getPlayerDistance(object);
		if (playerDistance > maxDistanceToPlayer)
			maxDistanceToPlayer = playerDistance;
	};

	if (crocObject == nullptr) {
		maxDistanceToPlayer = -1;
	} else {
		considerDistance(rootObject);
		considerDistance(crocObject);
		considerDistance(cameraObject);
		considerDistance(bossObject);
		considerDistance(dialogObject);

		if (rootObject != nullptr && rootObject->next != nullptr) {
			StratEntity *distanceNode = rootObject->next;
			while (distanceNode->prev != nullptr) {
				distanceNode = distanceNode->prev;
			}

			while (distanceNode != nullptr) {
				considerDistance(distanceNode);
				distanceNode = distanceNode->next;
			}
		}
	}

	ImGui::Text("Root");
	renderObjectRow(rootObject, "Root", true);
	ImGui::Text("Croc");
	renderObjectRow(crocObject, "Player", true);
	ImGui::Text("Camera");
	renderObjectRow(cameraObject, "Camera", true);
	ImGui::Text("Boss");
	renderObjectRow(bossObject, "Boss", true);
	ImGui::Text("Dialog");
	renderObjectRow(dialogObject, "Dialog", true);

	ImGui::Spacing();

	ImGui::Text("All Objects (Count: %d)", stratCountValue);
	ImGui::Separator();

	if (rootObject != nullptr && rootObject->next != nullptr) {
		StratEntity *node = rootObject->next;
		while (node->prev != nullptr) {
			node = node->prev;
		}

		static char searchBuffer[256] = {};
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputTextWithHint("##search", "Search...", searchBuffer, IM_ARRAYSIZE(searchBuffer));

		while (node != nullptr) {
			std::string tag;
			if (node == rootObject) {
				tag = "Root";
			} else if (node == crocObject) {
				tag = "Player";
			} else if (node == bossObject) {
				tag = "Boss";
			} else if (node == cameraObject) {
				tag = "Camera";
			} else if (node == dialogObject) {
				tag = "Dialog";
			}

			// Apply search filter
			std::string objectLabel = buildObjectLabel(node, tag.empty() ? nullptr : tag.c_str());
			bool matchesSearch = true;
			if (searchBuffer[0] != '\0') {
				std::string searchLower = toLower(std::string(searchBuffer));
				std::string labelLower = toLower(objectLabel);
				matchesSearch = labelLower.find(searchLower) != std::string::npos;
			}

			if (matchesSearch) {
				renderObjectRow(node, tag.empty() ? nullptr : tag.c_str(), false);
			}
			node = node->next;
		}
	} else {
		ImGui::Text("No objects found!");
	}

	ImGui::EndChild();

	// Splitter
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::InvisibleButton("##hsplitter", ImVec2(splitterHitW, availSize.y));
	ImVec2 splitterMin = ImGui::GetItemRectMin();
	ImVec2 splitterMax = ImGui::GetItemRectMax();
	float splitCenterX = (splitterMin.x + splitterMax.x) * 0.5f;
	ImU32 col = ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_SeparatorActive : (ImGui::IsItemHovered() ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator));
	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(splitCenterX - 1.0f, splitterMin.y), ImVec2(splitCenterX + 1.0f, splitterMax.y), col);
	if (ImGui::IsItemActive()) {
		float delta = ImGui::GetIO().MouseDelta.x;
		float newWidth = listPanelWidth + delta;

		int newWidthPx = (int)std::round(newWidth);
		if (newWidthPx < (int)std::ceil(minListWidth))
			newWidthPx = (int)std::ceil(minListWidth);
		if (newWidthPx > maxListPx)
			newWidthPx = maxListPx;
		listPanelWidth = (float)newWidthPx;

		storage->SetFloat(splitterStorageID, listPanelWidth);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}

	// Details panel
	ImGui::SameLine(0.0f, 0.0f);

	int detailsPx = availPx - (int)std::round(listPanelWidth) - splitterPx;
	if (detailsPx < minDetailsPx) {
		int needed = minDetailsPx - detailsPx;
		int newListPx = (int)std::round(listPanelWidth) - needed;
		if (newListPx < absoluteMinListPx)
			newListPx = absoluteMinListPx;
		listPanelWidth = (float)newListPx;
		detailsPx = availPx - newListPx - splitterPx;
		if (detailsPx < minDetailsPx)
			detailsPx = minDetailsPx;
	}
	float detailsPanelWidth = (float)detailsPx;
	ImGui::BeginChild("ObjectDetailsPanel", ImVec2(detailsPanelWidth, availSize.y), false);

	if (selectedObjectKey != 0) {
		StratEntity *selectedObject = reinterpret_cast<StratEntity *>(selectedObjectKey);
		if (selectedObject != nullptr && api != nullptr) {
			bool isValid = false;
			if (rootObject != nullptr && rootObject->next != nullptr) {
				StratEntity *node = rootObject->next;
				while (node->prev != nullptr) {
					node = node->prev;
				}
				while (node != nullptr) {
					if (node == selectedObject) {
						isValid = true;
						break;
					}
					node = node->next;
				}
			}

			if (isValid) {
				RenderObjectDetailsContent(selectedObject, crocObject);
			} else {
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Object no longer valid");
				selectedObjectKey = 0;
			}
		}
	} else {
		ImGui::TextDisabled("Select an object for details");
	}

	ImGui::EndChild();
	ImGui::PopFont();
	ImGui::End();

	if (prevShow != showObjectList) {
		api->WriteIniBool(L"GUI", L"ShowObjectList", showObjectList);
	}
}

} // namespace Overlay::ObjectList
