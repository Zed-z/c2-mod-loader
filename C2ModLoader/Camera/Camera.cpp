#include "Camera.h"

#include "ModApi.h"

#include <string>
#include <vector>

#include "FreeCamera.h"
#include "OrbitCamera.h"
#include "Utils.h"

extern ModApi *api;

namespace Camera {

bool cameraEnabled = true;
bool orbitCamera = true;
CameraMode cameraMode = CameraMode::Normal;

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

	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *croc = api->GetEntity(crocObjRef);

	if (camera == nullptr) {
		cameraMode = CameraMode::Normal;
		api->LogError("Camera not found!");
		api->ShowErrorToast("No camera found!");
		return;
	}

	if (croc == nullptr) {
		cameraMode = CameraMode::Normal;
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

void __stdcall Step() {

	// Don't touch the camera on the main menu
	if (levelInfo->tribe == 0)
		return;

	// Inputs
	Inputs inputs = api->GetInputs();
	Inputs inputsPressed = api->GetInputsPressed();

	// Toggle
	if (inputs.stepLeft && inputs.stepRight && inputsPressed.flip) {
		toggleFreecam();
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

void toggleOrbitCamera() {
	orbitCamera = !orbitCamera;
	api->WriteIniBool(L"Camera", L"OrbitCamera", orbitCamera);

	if (cameraMode == CameraMode::Freecam)
		return;

	if (orbitCamera) {
		cameraSet(CameraMode::Orbit);
	} else {
		cameraSet(CameraMode::Normal);
	}
}

void toggleFreecam() {
	if (cameraMode == CameraMode::Freecam) {
		if (orbitCamera) {
			cameraSet(CameraMode::Orbit);
		} else {
			cameraSet(CameraMode::Normal);
		}
	} else {
		cameraSet(CameraMode::Freecam);
	}
}

void Setup() {
	cameraEnabled = api->SetupIniBool(L"Camera", L"Enabled", true);
	orbitCamera = api->SetupIniBool(L"Camera", L"OrbitCamera", true);

	if (!cameraEnabled)
		return;

	FreeCamera::Setup();
	OrbitCamera::Setup();

	cameraMode = orbitCamera ? CameraMode::Orbit : CameraMode::Normal;

	api->HookGame(GAME_HOOK_POST_STEP, Step);
}

} // namespace Camera
