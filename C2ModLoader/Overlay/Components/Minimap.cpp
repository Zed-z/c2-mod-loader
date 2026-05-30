#include "Minimap.h"

#include "ModApi.h"
#include "ObjectList.h"
#include "Utils/Fonts.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstring>

extern ModApi *api;

namespace {

constexpr double PI = 3.14159265358979323846;
constexpr float MINIMAP_ZOOM_MIN = 0.05f;
constexpr float MINIMAP_ZOOM_MAX = 1.0f;
constexpr float MINIMAP_ZOOM_SCROLL_STEP = 0.05f;
constexpr float MINIMAP_DRAG_HANDLE_HEIGHT = 12.0f;

float ClampMinimapZoom(float zoom) {
	return (std::max)((std::min)(zoom, MINIMAP_ZOOM_MAX), MINIMAP_ZOOM_MIN);
}

double GameRotationToRadians(int gameRotation) {
	return (gameRotation * PI) / 2048.0;
}

enum class MapDotShape {
	Circle,
	Square,
	Diamond,
};

struct MapDot {
	ImVec4 color;
	MapDotShape shape;
	float radius;
	bool alwaysShow;
};

MapDot getMapDot(const StratEntity *object) {
	StratEntity *croc = api->GetEntity(crocObjRef);
	StratEntity *boss = api->GetEntity(bossObjRef);
	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *dialog = api->GetEntity(dialogObjRef);

	if (object == croc)
		return MapDot{ImVec4(0.95f, 0.95f, 0.95f, 1.0f), MapDotShape::Circle, 5.0f, true};
	if (object == boss)
		return MapDot{ImVec4(1.0f, 0.4f, 0.4f, 1.0f), MapDotShape::Circle, 8.0f, true};
	if (object == camera)
		return MapDot{ImVec4(1.0f, 0.75f, 0.2f, 1.0f), MapDotShape::Circle, 4.0f, true};
	if (object == dialog)
		return MapDot{ImVec4(0.45f, 0.75f, 1.0f, 1.0f), MapDotShape::Circle, 5.0f, false};

	// Crystals
	if (strncmp(object->name, "crystal", 7) == 0)
		return MapDot{ImVec4(0.25f, 0.75f, 1.0f, 1.0f), MapDotShape::Diamond, 5.0f, false};

	if (strncmp(object->name, "bonus crystal", 13) == 0)
		return MapDot{ImVec4(0.45f, 0.95f, 1.0f, 1.0f), MapDotShape::Diamond, 8.0f, true};

	if (strncmp(object->name, "Smash Box", 9) == 0)
		return MapDot{ImVec4(0.95f, 0.5f, 0.25f, 1.0f), MapDotShape::Square, 8.0f, false};

	if (strncmp(object->name, "Push Block", 10) == 0)
		return MapDot{ImVec4(0.65f, 0.3f, 0.05f, 1.0f), MapDotShape::Square, 8.0f, false};

	if (strncmp(object->name, "GONG", 4) == 0)
		return MapDot{ImVec4(0.75f, 0.65f, 0.35f, 1.0f), MapDotShape::Circle, 8.0f, false};

	if (strncmp(object->name, "Heart", 5) == 0)
		return MapDot{ImVec4(0.95f, 0.0f, 0.5f, 1.0f), MapDotShape::Circle, 5.0f, false};

	if (strncmp(object->name, "Golden Gobbo", 12) == 0)
		return MapDot{ImVec4(0.95f, 0.75f, 0.0f, 1.0f), MapDotShape::Diamond, 8.0f, false};

	// Collectibles
	if (
		strncmp(object->name, "GobboMountaineer", 16) == 0 ||
		strncmp(object->name, "inca baby", 9) == 0 ||
		strncmp(object->name, "Inca Dantini with Baby", 22) == 0 ||
		strncmp(object->name, "IncaSwingBaby", 13) == 0 ||
		strncmp(object->name, "IncaCarouselBaby", 16) == 0 ||
		strncmp(object->name, "inca throwing baby", 18) == 0)
		return MapDot{ImVec4(0.15f, 0.85f, 0.15f, 1.0f), MapDotShape::Circle, 6.0f, false};

	// Hazards
	if (object->flags0 & (1 << 16))
		return MapDot{ImVec4(1.0f, 0.25f, 0.25f, 1.0f), MapDotShape::Circle, 5.0f, false};

	return MapDot{ImVec4(0.5f, 0.5f, 0.5f, 0.75f), MapDotShape::Circle, 3.0f, false};
}

void drawRotatedArrow(ImDrawList *drawList, const ImVec2 &center, float angleRadians, float size, ImU32 fillColor) {
	const float cosAngle = std::cos(angleRadians);
	const float sinAngle = std::sin(angleRadians);
	auto rotatePoint = [&](float x, float y) {
		return ImVec2(center.x + (x * cosAngle - y * sinAngle), center.y + (x * sinAngle + y * cosAngle));
	};

	const float tipLength = size * 1.15f;
	const float bodyLength = size * 0.70f;
	const float halfWidth = size * 0.66f;

	ImVec2 tip = rotatePoint(0.0f, -tipLength);
	ImVec2 left = rotatePoint(-halfWidth, bodyLength);
	ImVec2 right = rotatePoint(halfWidth, bodyLength);

	drawList->AddTriangleFilled(tip, left, right, fillColor);
}

} // namespace

namespace Overlay::Minimap {

bool showMinimap;
float minimapZoom;
bool minimapAutoRotate;

enum class MinimapStyle {
	Circle,
	Square,
};
MinimapStyle minimapStyle;

void Setup() {
	showMinimap = api->SetupIniBool(L"GUI", L"ShowMinimap", false);
	minimapZoom = ClampMinimapZoom(api->SetupIniFloat(L"GUI", L"MinimapZoom", MINIMAP_ZOOM_MAX));
	minimapAutoRotate = api->SetupIniBool(L"GUI", L"MinimapAutoRotate", true);
	minimapStyle = static_cast<MinimapStyle>(api->SetupIniInt(L"GUI", L"MinimapStyle", static_cast<int>(MinimapStyle::Circle)));
}

void RenderMinimap() {
	if (!showMinimap)
		return;

	bool prevShow = showMinimap;

	ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 320.0f), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::Begin("Minimap", &showMinimap, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);
	ImGui::PushFont(Fonts::GetFontCode());
	ImGui::Dummy(ImVec2(0.0f, MINIMAP_DRAG_HANDLE_HEIGHT));

	StratEntity *firstObject = nullptr;
	StratEntity *playerObject = nullptr;
	StratEntity *cameraObject = nullptr;
	StratEntity *bossObject = nullptr;
	StratEntity *dialogObject = nullptr;

	if (currentWadFile != nullptr && *currentWadFile != nullptr) {
		WadFile *wad = *currentWadFile;
		if (wad->chunkData != nullptr) {
			firstObject = wad->chunkData->data.FirstStrat;
			playerObject = wad->chunkData->data.Player;
			cameraObject = wad->chunkData->data.Camera;
			bossObject = wad->chunkData->data.Boss;
			dialogObject = wad->chunkData->data.Dialog;
		}
	}

	StratEntity *crocObject = playerObject != nullptr ? playerObject : api->GetEntity(crocObjRef);
	const bool hasCroc = crocObject != nullptr;

	ImVec2 available = ImGui::GetContentRegionAvail();
	const float canvasSide = (std::min)(available.x, available.y);
	const float pad = 8.0f;
	const float innerSide = canvasSide - pad * 2.0f;
	const float mapRadius = innerSide * 0.5f;
	const float worldRadius = 4096.0f;

	float cameraDistanceMultiplier = 1.0f;
	if (hasCroc && cameraObject != nullptr) {
		float dx = cameraObject->newPosition.x - crocObject->newPosition.x;
		float dy = cameraObject->newPosition.y - crocObject->newPosition.y;
		float dz = cameraObject->newPosition.z - crocObject->newPosition.z;
		float cameraDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
		const float baselineCameraDistance = 2048.0f;
		cameraDistanceMultiplier = cameraDistance / baselineCameraDistance;
	}

	const float zoom = (std::max)(minimapZoom / cameraDistanceMultiplier, MINIMAP_ZOOM_MIN);
	const float scale = mapRadius * zoom / worldRadius;
	const float crocRotationRadians = hasCroc ? (float)GameRotationToRadians(crocObject->newRotPos.rotation.y >> 12) : 0.0f;
	const float cameraYawRadians = (cameraRot != nullptr) ? (float)GameRotationToRadians(cameraRot->y) : -crocRotationRadians;
	const float mapRotationRadians = minimapAutoRotate ? cameraYawRadians : 0.0f;
	const float mapCos = std::cos(mapRotationRadians);
	const float mapSin = std::sin(mapRotationRadians);

	ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
	ImVec2 canvasPos(canvasOrigin.x + (available.x - canvasSide) * 0.5f, canvasOrigin.y + (available.y - canvasSide) * 0.5f);
	ImVec2 canvasEnd(canvasPos.x + canvasSide, canvasPos.y + canvasSide);

	ImGui::SetCursorScreenPos(canvasPos);
	ImGui::InvisibleButton("##minimap_canvas", ImVec2(canvasSide, canvasSide));
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
		float mouseWheel = ImGui::GetIO().MouseWheel;
		if (mouseWheel != 0.0f) {
			minimapZoom = ClampMinimapZoom(minimapZoom + (mouseWheel * MINIMAP_ZOOM_SCROLL_STEP));
			api->WriteIniFloat(L"GUI", L"MinimapZoom", minimapZoom);
		}
	}
	if (ImGui::BeginPopupContextItem("##minimap_context", ImGuiPopupFlags_MouseButtonRight)) {
		ImGui::SetNextItemWidth(150.0f);
		if (ImGui::SliderFloat("Zoom", &minimapZoom, MINIMAP_ZOOM_MIN, MINIMAP_ZOOM_MAX, "%.3f")) {
			minimapZoom = ClampMinimapZoom(minimapZoom);
			api->WriteIniFloat(L"GUI", L"MinimapZoom", minimapZoom);
		}
		if (ImGui::Checkbox("Auto Rotate", &minimapAutoRotate)) {
			api->WriteIniBool(L"GUI", L"MinimapAutoRotate", minimapAutoRotate);
		}
		ImGui::SetNextItemWidth(150.0f);
		if (ImGui::Combo("Style", reinterpret_cast<int *>(&minimapStyle), "Circle\0Square\0")) {
			api->WriteIniInt(L"GUI", L"MinimapStyle", static_cast<int>(minimapStyle));
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Close minimap")) {
			showMinimap = false;
		}
		ImGui::EndPopup();
	}

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(canvasPos, canvasEnd, true);

	ImVec2 center((canvasPos.x + canvasEnd.x) * 0.5f, (canvasPos.y + canvasEnd.y) * 0.5f);
	const float outerRadius = canvasSide * 0.5f;
	const ImU32 mapBackgroundColor = ImGui::GetColorU32(ImVec4(0.10f, 0.11f, 0.13f, 0.92f));
	switch (minimapStyle) {
	case MinimapStyle::Square:
		drawList->AddRectFilled(canvasPos, canvasEnd, mapBackgroundColor);
		drawList->AddRect(ImVec2(center.x - mapRadius, center.y - mapRadius), ImVec2(center.x + mapRadius, center.y + mapRadius), ImGui::GetColorU32(ImGuiCol_Separator), 0.0f, 0, 1.25f);
		break;
	case MinimapStyle::Circle:
		drawList->AddCircleFilled(center, outerRadius, mapBackgroundColor, 128);
		drawList->AddCircle(center, mapRadius, ImGui::GetColorU32(ImGuiCol_Separator), 128, 1.25f);
		break;
	}

	drawList->AddLine(ImVec2(center.x - mapRadius, center.y), ImVec2(center.x + mapRadius, center.y), ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
	drawList->AddLine(ImVec2(center.x, center.y - mapRadius), ImVec2(center.x, center.y + mapRadius), ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);

	if (!hasCroc) {
		drawList->PopClipRect();
		ImGui::PopFont();
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		if (prevShow != showMinimap)
			api->WriteIniBool(L"GUI", L"ShowMinimap", showMinimap);
		return;
	}

	StratEntity *hoveredObject = nullptr;
	auto drawDot = [&](StratEntity *object) {
		if (object == nullptr)
			return;

		MapDot dot = getMapDot(object);

		float dx = static_cast<float>(object->newPosition.x - crocObject->newPosition.x);
		float dz = static_cast<float>(object->newPosition.z - crocObject->newPosition.z);

		ImVec2 offset(-dx * scale, -dz * scale);
		if (minimapAutoRotate) {
			float rotatedX = offset.x * mapCos - offset.y * mapSin;
			float rotatedY = offset.x * mapSin + offset.y * mapCos;
			offset.x = rotatedX;
			offset.y = rotatedY;
		}

		const float visibleRadius = mapRadius - dot.radius;
		bool offMap = false;
		switch (minimapStyle) {
		case MinimapStyle::Square:
			offMap = std::abs(offset.x) > visibleRadius || std::abs(offset.y) > visibleRadius;
			break;
		case MinimapStyle::Circle:
			offMap = (offset.x * offset.x + offset.y * offset.y) > (visibleRadius * visibleRadius);
			break;
		}

		if (offMap && !dot.alwaysShow)
			return;

		ImVec2 renderCenter(center.x + offset.x, center.y + offset.y);
		if (offMap) {
			switch (minimapStyle) {
			case MinimapStyle::Square: {
				const float clampX = mapRadius - dot.radius;
				const float clampY = mapRadius - dot.radius;
				renderCenter.x = center.x + (std::max)((std::min)(offset.x, clampX), -clampX);
				renderCenter.y = center.y + (std::max)((std::min)(offset.y, clampY), -clampY);
				break;
			}
			case MinimapStyle::Circle: {
				float offsetLength = std::sqrt(offset.x * offset.x + offset.y * offset.y);
				if (offsetLength > 0.001f) {
					float clamped = visibleRadius;
					renderCenter.x = center.x + offset.x / offsetLength * clamped;
					renderCenter.y = center.y + offset.y / offsetLength * clamped;
				}
				break;
			}
			}
		}
		const float hoverRadius = dot.radius + 3.0f;
		bool isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
			ImGui::IsMouseHoveringRect(ImVec2(renderCenter.x - hoverRadius, renderCenter.y - hoverRadius), ImVec2(renderCenter.x + hoverRadius, renderCenter.y + hoverRadius), false);
		if (isHovered) {
			hoveredObject = object;
			ImGui::SetTooltip("%s", object->name ? object->name : "(unnamed)");
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				Overlay::ObjectList::SelectObject(object);
			}
		}

		switch (dot.shape) {
		case MapDotShape::Circle:
			drawList->AddCircleFilled(renderCenter, dot.radius, ImGui::GetColorU32(dot.color), 12);
			break;
		case MapDotShape::Square:
			drawList->AddRectFilled(ImVec2(renderCenter.x - dot.radius, renderCenter.y - dot.radius), ImVec2(renderCenter.x + dot.radius, renderCenter.y + dot.radius), ImGui::GetColorU32(dot.color), 0.0f);
			break;
		case MapDotShape::Diamond:
			ImVec2 diamondPoints[4] = {
				ImVec2(renderCenter.x, renderCenter.y - dot.radius),
				ImVec2(renderCenter.x + dot.radius, renderCenter.y),
				ImVec2(renderCenter.x, renderCenter.y + dot.radius),
				ImVec2(renderCenter.x - dot.radius, renderCenter.y),
			};
			drawList->AddConvexPolyFilled(
				diamondPoints, 4, ImGui::GetColorU32(dot.color));
			break;
		}
	};

	drawDot(firstObject);
	drawDot(cameraObject);
	drawDot(bossObject);
	drawDot(dialogObject);

	if (firstObject != nullptr && firstObject->next != nullptr) {
		StratEntity *node = firstObject->next;
		while (node->prev != nullptr)
			node = node->prev;

		while (node != nullptr) {
			if (node != firstObject && node != playerObject && node != cameraObject && node != bossObject && node != dialogObject) {
				drawDot(node);
			}
			node = node->next;
		}
	}

	if (hoveredObject == nullptr && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		ImVec2 windowPos = ImGui::GetWindowPos();
		ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
		ImGui::SetWindowPos(ImVec2(windowPos.x + dragDelta.x, windowPos.y + dragDelta.y));
		ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
	}

	const float crocArrowAngle = mapRotationRadians - crocRotationRadians;
	ImU32 crocFill = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	drawRotatedArrow(drawList, center, crocArrowAngle, 9.0f, crocFill);

	drawList->PopClipRect();

	ImGui::PopFont();
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();

	if (prevShow != showMinimap) {
		api->WriteIniBool(L"GUI", L"ShowMinimap", showMinimap);
	}
}

} // namespace Overlay::Minimap
