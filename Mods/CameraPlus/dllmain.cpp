#include "ModApi.h"

#include <string>
#include <vector>

#include "FreeCamera.h"
#include "OrbitCamera.h"
#include "Utils.h"

ModApi *api = nullptr;

enum class CameraMode {
	None = -1,
	Normal = 0,
	Orbit = 1,
	Freecam = 2
};
const char *CameraModeNames[] = {"None", "Normal", "Orbit", "Freecam"};
CameraMode cameraMode = CameraMode::Normal;

void saveCameraMode() {
	api->WriteIniInt(L"Config", L"CameraMode", static_cast<int>(cameraMode));
}

CameraMode loadCameraMode() {
	return static_cast<CameraMode>(api->SetupIniInt(L"Config", L"CameraMode", static_cast<int>(CameraMode::Normal)));
}

void __stdcall cameraSet(CameraMode mode = CameraMode::None) {
	CameraMode nextCameraMode = CameraMode::None;
	if (mode == CameraMode::None) {
		switch (cameraMode) {
		case CameraMode::Normal:
			nextCameraMode = CameraMode::Orbit;
			break;
		case CameraMode::Orbit:
			nextCameraMode = CameraMode::Freecam;
			break;
		case CameraMode::Freecam:
			nextCameraMode = CameraMode::Normal;
			break;
		}
	} else {
		nextCameraMode = mode;
	}

	if (cameraMode == CameraMode::Orbit)
		OrbitCamera::SwitchOut();
	if (cameraMode == CameraMode::Freecam)
		FreeCamera::SwitchOut();

	cameraMode = nextCameraMode;

	saveCameraMode();

	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *croc = api->GetEntity(crocObjRef);

	if (camera == nullptr) {
		cameraMode = CameraMode::Normal;
		saveCameraMode();
		api->LogError("Camera not found!");
		api->ShowErrorToast("No camera found!");
		return;
	}

	if (croc == nullptr) {
		cameraMode = CameraMode::Normal;
		saveCameraMode();
		api->LogError("Croc not found!");
		api->ShowErrorToast("Croc not found!");
		return;
	}

	if (cameraMode == CameraMode::Orbit)
		OrbitCamera::SwitchIn();
	if (cameraMode == CameraMode::Freecam)
		FreeCamera::SwitchIn();

	const std::string modeString = "Camera mode: " + std::string(CameraModeNames[static_cast<int>(cameraMode) + 1]);
	api->LogInfo(modeString.c_str());
	api->ShowInfoToast(modeString.c_str());
}

void __stdcall cameraSetNormal() {
	cameraSet(CameraMode::Normal);
}

void __stdcall cameraSetOrbit() {
	cameraSet(CameraMode::Orbit);
}

void __stdcall cameraSetFreecam() {
	cameraSet(CameraMode::Freecam);
}

void __stdcall Step() {

	// Don't touch the camera on the main menu
	if (levelInfo->tribe == 0)
		return;

	// Inputs
	Inputs inputs = api->GetInputs();
	Inputs inputsPressed = api->GetInputsPressed();

	// Toggle
	if (inputs.stepLeft && inputs.stepRight && inputsPressed.flip) {
		cameraSet();
	}

	// Camera
	switch (cameraMode) {
	case CameraMode::Normal: {
		break;
	}
	case CameraMode::Orbit: {
		OrbitCamera::Step();
		break;
	}
	case CameraMode::Freecam: {
		FreeCamera::Step();
		break;
	}
	}
}

MenuActionRegistration __stdcall cameraNormalRegistration() {
	return {"Normal Camera", "The standard camera mode.", cameraSetNormal, cameraMode != CameraMode::Normal};
}

MenuActionRegistration __stdcall cameraOrbitRegistration() {
	return {"Orbit Camera", "A modern manual camera mode.", cameraSetOrbit, cameraMode != CameraMode::Orbit};
}

MenuActionRegistration __stdcall cameraFreeRegistration() {
	return {"Free Camera", "Move the camera anywhere you want.", cameraSetFreecam, cameraMode != CameraMode::Freecam};
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		api = LoadModApi();
		if (!api)
			return FALSE;

		FreeCamera::Setup();
		OrbitCamera::Setup();

		CameraMode _camMode = loadCameraMode();
		cameraMode = _camMode;
		// cameraSet(_camMode);

		api->HookGame(GAME_HOOK_POST_STEP, Step);
		api->RegisterMenuAction(hModule, cameraNormalRegistration);
		api->RegisterMenuAction(hModule, cameraOrbitRegistration);
		api->RegisterMenuAction(hModule, cameraFreeRegistration);

		DisableThreadLibraryCalls(hModule);
	}
	return TRUE;
}
