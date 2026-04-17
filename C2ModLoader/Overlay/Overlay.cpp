#include "Overlay.h"

#include "Components/Coords.h"
#include "Components/Inputs.h"
#include "Components/LevelInfo.h"
#include "Components/LevelSelect.h"
#include "Components/Log.h"
#include "Components/MenuBar.h"
#include "Components/ObjectList.h"
#include "Components/SaveSlotList.h"
#include "Components/Toast.h"
#include "Components/VersionInfo.h"
#include "Utils/Style.h"

bool showGui;

void ImGuiDraw() {
	Style::ApplyStyle();

	RenderVersionInfo();

	if (showGui) {
		RenderCoords();
		RenderInputs();
		RenderLevelInfo();
		RenderLevelSelect();
		RenderLog();
		RenderMenuBar();
		RenderObjectList();
		RenderSaveSlotList();
	}

	RenderToasts();
}
