#include "ModApi.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using std::sin, std::cos, std::min, std::max;

ModApi *api = nullptr;

enum class CameraMode {
	None = -1,
	Normal = 0,
	Orbit = 1,
	Freecam = 2
};
CameraMode cameraMode = CameraMode::Normal;
bool noclipCameraFlag = false;

RotPos3i cameraRotPos;
double cameraYaw = 0;
const double MIN_CAMERA_PITCH = -0.2;
const double MAX_CAMERA_PITCH = 0.9;
const double DEFAULT_CAMERA_PITCH = 0.3;
double cameraPitch = DEFAULT_CAMERA_PITCH;
Vec3i cameraLookAt;

constexpr double PI = 3.14159265358979323846;

const double CAMERA_ORBIT_Y_OFFSET = 800.0;
const double ORBIT_MIN_DISTANCE = 1200.0;
const double ORBIT_MAX_DISTANCE = 3600.0;
const double DEFAULT_ORBIT_DISTANCE = 2400.0;
double orbitCameraDistance = DEFAULT_ORBIT_DISTANCE;

const double CAMERA_YAW_SPEED = 0.15;
const double CAMERA_PITCH_SPEED = 0.05;
const double CAMERA_ZOOM_SPEED = 80.0;

bool orbitInvertX;
bool orbitInvertY;
bool orbitAutoTurn;
int orbitAutoTurnStrength;
int orbitAutoTurnMinSpeed;
enum ZoomControls {
	ZOOM_NONE = 0,
	ZOOM_TRIGGERS = 1,
	ZOOM_RIGHT_STICK_CLICK = 2
};
ZoomControls orbitZoomControls = ZOOM_TRIGGERS;
std::vector<LevelInfo> orbitLevelBlocklist;

void populateOrbitBlocklist(wchar_t *orbitLevelBlockList, std::vector<LevelInfo> &blocklist) {
	std::wstring orbitLevelBlocklistStr(orbitLevelBlockList);
	std::wstring token;
	std::wstringstream wss(orbitLevelBlocklistStr);
	while (std::getline(wss, token, L',')) {
		LevelInfo levelInfo;
		size_t firstSep = token.find(':');
		size_t secondSep = token.find(':', firstSep + 1);
		if (firstSep == std::wstring::npos || secondSep == std::wstring::npos) {
			continue;
		}

		try {
			levelInfo.tribe = std::stoi(token.substr(0, firstSep));
			levelInfo.level = std::stoi(token.substr(firstSep + 1, secondSep - firstSep - 1));
			levelInfo.map = std::stoi(token.substr(secondSep + 1));
		} catch (...) {
			continue;
		}

		orbitLevelBlocklist.push_back(levelInfo);
	}
}

bool orbitHasLastCrocPos = false;
Vec3i orbitLastCrocPos;

bool freecamInvertX;
bool freecamInvertY;

void saveCameraMode() {
	api->WriteIniInt(L"Config", L"CameraMode", static_cast<int>(cameraMode));
}

CameraMode loadCameraMode() {
	return static_cast<CameraMode>(api->SetupIniInt(L"Config", L"CameraMode", static_cast<int>(CameraMode::Normal)));
}

static double LerpAngle(double a, double b, double t) {
	double diff = fmod(b - a + PI, 2 * PI);
	if (diff < 0) {
		diff += 2 * PI;
	}
	diff -= PI;
	return a + diff * t;
}

int RadiansToGameRotation(double radians_input) {
	radians_input = fmod(radians_input, 2 * PI);
	while (radians_input <= -PI) {
		radians_input += 2 * PI;
	}
	while (radians_input > PI) {
		radians_input -= 2 * PI;
	}

	double scaled_value = radians_input * (2048.0 / PI);

	int game_rotation_value = static_cast<int>(round(scaled_value));

	if (game_rotation_value == 2048) {
		game_rotation_value = -2048;
	}

	return game_rotation_value;
}

double GameRotationToRadians(int game_rotation) {
	return (game_rotation * PI) / 2048.0;
}

void __stdcall cameraSet(CameraMode mode = CameraMode::None) {
	if (mode == CameraMode::None) {
		switch (cameraMode) {
		case CameraMode::Normal:
			cameraMode = CameraMode::Orbit;
			break;
		case CameraMode::Orbit:
			cameraMode = CameraMode::Freecam;
			break;
		case CameraMode::Freecam:
			cameraMode = CameraMode::Normal;
			break;
		}
	} else {
		cameraMode = mode;
	}
	saveCameraMode();

	StratEntity *camera = api->GetEntity(ADDR_CAMERA_OBJ);
	StratEntity *croc = api->GetEntity(ADDR_CROC_OBJ);

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

	switch (cameraMode) {
	case CameraMode::Normal: {

		api->LogInfo("Camera mode: Normal");
		api->ShowInfoToast("Camera mode: Normal");

		break;
	}
	case CameraMode::Orbit: {

		// Get camera position and lookat
		cameraRotPos = camera->newRotPos;
		orbitHasLastCrocPos = false;

		// Get rotation from lookat
		cameraYaw = -(double)(api->AddressGetInt(ADDR_CAMERA_ROT_Y)) / 2048.0 * PI;
		cameraPitch = (double)(api->AddressGetInt(ADDR_CAMERA_ROT_X)) / 2048.0 * PI;

		api->LogInfo("Camera mode: Orbit");
		api->ShowInfoToast("Camera mode: Orbit");

		break;
	}
	case CameraMode::Freecam: {

		// Get camera position and lookat
		cameraRotPos = camera->newRotPos;
		cameraLookAt.x = api->AddressGetInt(ADDR_CAMERA_LOOKAT_X);
		cameraLookAt.y = api->AddressGetInt(ADDR_CAMERA_LOOKAT_Y);
		cameraLookAt.z = api->AddressGetInt(ADDR_CAMERA_LOOKAT_Z);

		// Get rotation from lookat
		cameraYaw = -(double)(api->AddressGetInt(ADDR_CAMERA_ROT_Y)) / 2048.0 * PI;
		cameraPitch = (double)(api->AddressGetInt(ADDR_CAMERA_ROT_X)) / 2048.0 * PI;

		api->LogInfo("Camera mode: Freecam");
		api->ShowInfoToast("Camera mode: Freecam");

		break;
	}
	}

	if (cameraMode == CameraMode::Freecam) {
		// Change camera flag
		noclipCameraFlag = camera->flags1 & (1 << 23);
		camera->flags1 &= ~(1 << 23);

		// Pause player movement
		croc->flags0 |= (1 << 4);
	} else {
		camera->flags1 |= (noclipCameraFlag << 23);
		croc->flags0 &= ~(1 << 4);
	}
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

void __stdcall orbitCameraReset() {
	StratEntity *croc = api->GetEntity(ADDR_CROC_OBJ);
	if (croc == nullptr) {
		api->LogDebug("[orbitCameraReset] Croc not found!");
		return;
	}
	cameraYaw = GameRotationToRadians(croc->newRotPos.rotation.y >> 0xc) - PI;
	cameraPitch = DEFAULT_CAMERA_PITCH;
	orbitCameraDistance = DEFAULT_ORBIT_DISTANCE;
}

static DWORD WINAPI delayedOrbitCameraResetThread(LPVOID param) {
	int delay = (int)param;
	Sleep(delay);
	orbitCameraReset();
	return 0;
}

void __stdcall mapDoorChangeOrbitCameraReset() {
	CreateThread(nullptr, 0, delayedOrbitCameraResetThread, (LPVOID)100, 0, nullptr);
}

void __stdcall deathChangeOrbitCameraReset() {
	CreateThread(nullptr, 0, delayedOrbitCameraResetThread, (LPVOID)700, 0, nullptr);
}

void __stdcall PhysicsLoop() {

	// Xinput
	XinputInput input = api->GetXinputState();
	const bool xinputEnabled = input.config.enabled;
	const float stickScale = input.config.stickScale;
	const float triggerScale = input.config.triggerScale;

	// Inputs
	Inputs inputs = api->GetInputs();
	Inputs inputsPressed = api->GetInputsPressed();
	Inputs inputsReleased = api->GetInputsReleased();

	// Entities
	StratEntity *camera = api->GetEntity(ADDR_CAMERA_OBJ);
	StratEntity *croc = api->GetEntity(ADDR_CROC_OBJ);

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

		if (camera == nullptr)
			return;

		if (croc == nullptr)
			return;

		LevelInfo currentLevel = api->GetLevelInfo();
		const bool orbitBlockedOnLevel = std::any_of(
			orbitLevelBlocklist.begin(),
			orbitLevelBlocklist.end(),
			[&](const LevelInfo &blocked) {
				return blocked.tribe == currentLevel.tribe && blocked.level == currentLevel.level && blocked.map == currentLevel.map;
			});
		if (orbitBlockedOnLevel)
			break;

		if (inputs.flip) {
			orbitCameraReset();
		}

		if (xinputEnabled) {
			switch (orbitZoomControls) {
			case ZOOM_TRIGGERS: {
				orbitCameraDistance += ((input.leftTrigger - input.rightTrigger) / triggerScale) * CAMERA_ZOOM_SPEED;
				orbitCameraDistance = min(max(orbitCameraDistance, ORBIT_MIN_DISTANCE), ORBIT_MAX_DISTANCE);
				break;
			}
			case ZOOM_RIGHT_STICK_CLICK: {
				static bool wasRightStickClicked = false;
				if (input.rightStick.click && !wasRightStickClicked) {
					wasRightStickClicked = true;
					if (orbitCameraDistance == ORBIT_MAX_DISTANCE) {
						orbitCameraDistance = ORBIT_MIN_DISTANCE;
					} else if (orbitCameraDistance == ORBIT_MIN_DISTANCE) {
						orbitCameraDistance = DEFAULT_ORBIT_DISTANCE;
					} else if (orbitCameraDistance == DEFAULT_ORBIT_DISTANCE) {
						orbitCameraDistance = ORBIT_MAX_DISTANCE;
					} else {
						orbitCameraDistance = DEFAULT_ORBIT_DISTANCE;
					}
				} else if (!input.rightStick.click) {
					wasRightStickClicked = false;
				}
				break;
			}
			}
		}

		float input_rot_yaw, input_rot_pitch;
		if (xinputEnabled) {
			input_rot_yaw = ((float)input.rightStick.x / stickScale) * (orbitInvertX ? -1 : 1);
			input_rot_pitch = ((float)input.rightStick.y / stickScale) * (orbitInvertY ? -1 : 1);
		} else {
			input_rot_yaw = (inputs.stepRight - inputs.stepLeft) * (orbitInvertX ? -1 : 1);
			input_rot_pitch = (inputs.invRight - inputs.invLeft) * (orbitInvertY ? -1 : 1);
		}

		cameraYaw += -input_rot_yaw * CAMERA_YAW_SPEED;
		cameraPitch += input_rot_pitch * CAMERA_PITCH_SPEED;
		cameraPitch = min(max(cameraPitch, MIN_CAMERA_PITCH), MAX_CAMERA_PITCH);

		if (!orbitHasLastCrocPos) {
			orbitLastCrocPos = croc->newRotPos.position;
			orbitHasLastCrocPos = true;
		}

		const int moveX = croc->newRotPos.position.x - orbitLastCrocPos.x;
		const int moveZ = croc->newRotPos.position.z - orbitLastCrocPos.z;
		const double moveDistanceSq = (double)moveX * (double)moveX + (double)moveZ * (double)moveZ;
		const double minMoveDistanceSq = (double)orbitAutoTurnMinSpeed * (double)orbitAutoTurnMinSpeed;

		if (orbitAutoTurn && input_rot_yaw == 0 && moveDistanceSq >= minMoveDistanceSq) {
			const double moveYaw = atan2((double)moveX, (double)moveZ);
			const double targetYaw = moveYaw + PI;
			const double yawLerpSpeed = ((double)orbitAutoTurnStrength / 100.0) * 0.08;
			cameraYaw = LerpAngle(cameraYaw, targetYaw, yawLerpSpeed);
		}

		orbitLastCrocPos = croc->newRotPos.position;

		double offsetX = cos(cameraPitch) * sin(cameraYaw) * orbitCameraDistance;
		double offsetY = sin(cameraPitch) * orbitCameraDistance;
		double offsetZ = cos(cameraPitch) * cos(cameraYaw) * orbitCameraDistance;

		cameraRotPos.position.x = croc->newRotPos.position.x + static_cast<int>(offsetX);
		cameraRotPos.position.y = static_cast<int>(CAMERA_ORBIT_Y_OFFSET) + croc->newRotPos.position.y + static_cast<int>(offsetY);
		cameraRotPos.position.z = croc->newRotPos.position.z + static_cast<int>(offsetZ);

		camera->OldRotPos = cameraRotPos;
		camera->newRotPos = cameraRotPos;
		api->AddressSetInt(ADDR_CAMERA_POS_X, cameraRotPos.position.x);
		api->AddressSetInt(ADDR_CAMERA_POS_Y, cameraRotPos.position.y);
		api->AddressSetInt(ADDR_CAMERA_POS_Z, cameraRotPos.position.z);

		break;
	}
	case CameraMode::Freecam: {

		float input_x, input_y, input_z;
		float input_rot_yaw, input_rot_pitch;
		if (xinputEnabled) {
			input_x = -((float)input.leftStick.x / stickScale);
			input_z = ((float)input.leftStick.y / stickScale);
			input_y = inputs.jump - inputs.attack;
			input_rot_yaw = ((float)input.rightStick.x / stickScale) * (freecamInvertX ? -1 : 1);
			input_rot_pitch = ((float)input.rightStick.y / stickScale) * (freecamInvertY ? -1 : 1);
		} else {
			input_x = inputs.right - inputs.left;
			input_z = -(inputs.down - inputs.up);
			input_y = inputs.jump - inputs.attack;
			input_rot_yaw = (inputs.stepRight - inputs.stepLeft) * (freecamInvertX ? -1 : 1);
			input_rot_pitch = (inputs.invRight - inputs.invLeft) * (freecamInvertY ? -1 : 1);
		}

		cameraYaw += input_rot_yaw * 0.1;
		cameraPitch += input_rot_pitch * 0.05;
		cameraPitch = min(max(cameraPitch, -0.9), 0.9);

		double forwards_x = -cos(cameraYaw);
		double forwards_z = -sin(cameraYaw);

		cameraRotPos.position.x += static_cast<int>(forwards_x * input_x * 100);
		cameraRotPos.position.z += static_cast<int>(forwards_z * input_x * 100);

		double sidewards_x = cos(cameraYaw + PI / 2);
		double sidewards_z = sin(cameraYaw + PI / 2);

		cameraRotPos.position.x += static_cast<int>(sidewards_x * input_z * 100);
		cameraRotPos.position.y += static_cast<int>(input_y * 100);
		cameraRotPos.position.z += static_cast<int>(sidewards_z * input_z * 100);

		cameraLookAt.x = cameraRotPos.position.x + static_cast<int>(sin(-cameraYaw) * cos(-cameraPitch) * 100.0);
		cameraLookAt.y = cameraRotPos.position.y + static_cast<int>(sin(-cameraPitch) * 100.0);
		cameraLookAt.z = cameraRotPos.position.z + static_cast<int>(cos(-cameraYaw) * cos(-cameraPitch) * 100.0);

		camera->OldRotPos = cameraRotPos;
		camera->newRotPos = cameraRotPos;
		api->AddressSetInt(ADDR_CAMERA_POS_X, cameraRotPos.position.x);
		api->AddressSetInt(ADDR_CAMERA_POS_Y, cameraRotPos.position.y);
		api->AddressSetInt(ADDR_CAMERA_POS_Z, cameraRotPos.position.z);

		api->AddressSetInt(ADDR_CAMERA_LOOKAT_X, cameraLookAt.x);
		api->AddressSetInt(ADDR_CAMERA_LOOKAT_Y, cameraLookAt.y);
		api->AddressSetInt(ADDR_CAMERA_LOOKAT_Z, cameraLookAt.z);

		api->AddressSetInt(ADDR_CAMERA_ROT_X, RadiansToGameRotation(-cameraPitch));
		api->AddressSetInt(ADDR_CAMERA_ROT_Y, RadiansToGameRotation(-cameraYaw));

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

		CameraMode _camMode = loadCameraMode();
		cameraMode = _camMode;
		// cameraSet(_camMode);

		orbitInvertX = api->SetupIniBool(L"OrbitCamera", L"InvertX", false);
		orbitInvertY = api->SetupIniBool(L"OrbitCamera", L"InvertY", false);
		orbitAutoTurn = api->SetupIniBool(L"OrbitCamera", L"AutoTurn", true);
		orbitAutoTurnStrength = min(max(api->SetupIniInt(L"OrbitCamera", L"AutoTurnStrength", 40), 0), 100);
		orbitAutoTurnMinSpeed = max(api->SetupIniInt(L"OrbitCamera", L"AutoTurnMinSpeed", 20), 0);

		const int zoomControlsValue = min(max(api->SetupIniInt(L"OrbitCamera", L"ZoomControls", 1), ZOOM_NONE), ZOOM_RIGHT_STICK_CLICK);
		orbitZoomControls = static_cast<ZoomControls>(zoomControlsValue);

		constexpr int orbitLevelBlocklistStringLength = 65536;
		std::unique_ptr<wchar_t[]> orbitLevelBlocklistWchar = std::make_unique<wchar_t[]>(orbitLevelBlocklistStringLength);
		api->ReadIniString(L"OrbitCamera", L"LevelBlocklist", L"", orbitLevelBlocklistWchar.get(), orbitLevelBlocklistStringLength);
		populateOrbitBlocklist(orbitLevelBlocklistWchar.get(), orbitLevelBlocklist);

		freecamInvertX = api->SetupIniBool(L"Freecam", L"InvertX", false);
		freecamInvertY = api->SetupIniBool(L"Freecam", L"InvertY", false);

		api->HookGame(GAME_HOOK_PHYSICS, PhysicsLoop);
		api->RegisterMenuAction(hModule, cameraNormalRegistration);
		api->RegisterMenuAction(hModule, cameraOrbitRegistration);
		api->RegisterMenuAction(hModule, cameraFreeRegistration);

		api->HookGame(GAME_HOOK_MAP_CHANGE, mapDoorChangeOrbitCameraReset);
		api->HookGame(GAME_HOOK_DOOR_CHANGE, mapDoorChangeOrbitCameraReset);
		api->HookGame(GAME_HOOK_PLAYER_DEATH, deathChangeOrbitCameraReset);

		DisableThreadLibraryCalls(hModule);
	}
	return TRUE;
}
