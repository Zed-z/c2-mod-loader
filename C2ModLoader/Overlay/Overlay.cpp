#include "Overlay.h"

#include "Components/Coords.h"
#include "Components/Inputs.h"
#include "Components/LevelInfo.h"
#include "Components/Log.h"
#include "Components/MenuBar.h"
#include "Components/ObjectList.h"
#include "Components/SaveSlotList.h"
#include "Components/Toast.h"
#include "Components/VersionInfo.h"
#include "Launcher/Colors.h"

bool showGui;

void ImGuiDraw() {
	Launcher::ApplyStyle();

	RenderVersionInfo();

	if (showGui) {
		RenderLog();
		RenderInputs();
		RenderObjectList();
		RenderCoords();
		RenderMenuBar();
		RenderLevelInfo();
		RenderSaveSlotList();
	}

	// Toast notifications
	RenderToasts();
}
