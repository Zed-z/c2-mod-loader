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

double cameraYaw = 0;
const double MIN_CAMERA_PITCH = -0.2;
const double MAX_CAMERA_PITCH = 0.9;
const double DEFAULT_CAMERA_PITCH = 0.3;
double cameraPitch = DEFAULT_CAMERA_PITCH;
RotPos3i _cameraRotPos;
Vec3i _cameraLookAt;

constexpr double PI = 3.14159265358979323846;

const double CAMERA_ORBIT_Y_OFFSET = 800.0;
const double ORBIT_MIN_DISTANCE = 1200.0;
const double ORBIT_MAX_DISTANCE = 3600.0;
const double DEFAULT_ORBIT_DISTANCE = 2400.0;
double orbitCameraDistance = DEFAULT_ORBIT_DISTANCE;
double orbitCameraCurrentDistance = DEFAULT_ORBIT_DISTANCE;

const double CAMERA_YAW_SPEED = 0.15;
const double CAMERA_PITCH_SPEED = 0.05;
const double CAMERA_ZOOM_SPEED = 80.0;

bool orbitInvertX;
bool orbitInvertY;
bool orbitAutoTurnSaved;
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
bool orbitDisableRestrictions;
bool centerOnType2;
bool centerOnType2Current;
bool orbitVehicleRearViewCamera;
bool orbitImprovedBossCamera;
bool orbitButtonResetPitch;
bool orbitButtonResetZoom;

double GameRotationToRadians(int game_rotation) {
	return (game_rotation * PI) / 2048.0;
}

void __stdcall orbitCameraResetYaw() {
	StratEntity *croc = api->GetEntity(crocObjRef);
	if (croc == nullptr) {
		api->LogDebug("[orbitCameraReset] Croc not found!");
		return;
	}
	cameraYaw = GameRotationToRadians(croc->newRotPos.rotation.y >> 0xc) - PI;
}

void __stdcall orbitCameraResetDistance() {
	orbitCameraDistance = DEFAULT_ORBIT_DISTANCE;
}

void __stdcall orbitCameraResetPitch() {
	cameraPitch = DEFAULT_CAMERA_PITCH;
}

void __stdcall orbitCameraResetTransitions() {
	orbitCameraResetYaw();
	orbitCameraResetPitch();
}

void __stdcall orbitCameraResetButton() {
	orbitCameraResetYaw();
	if (orbitButtonResetPitch)
		orbitCameraResetPitch();
	if (orbitButtonResetZoom)
		orbitCameraResetDistance();
}

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

bool orbitCameraDisabled() {
	if (orbitDisableRestrictions)
		return false;

	LevelInfo currentLevel = api->GetLevelInfo();
	const bool orbitBlockedOnLevel = std::any_of(
		orbitLevelBlocklist.begin(),
		orbitLevelBlocklist.end(),
		[&](const LevelInfo &blocked) {
			return blocked.tribe == currentLevel.tribe && blocked.level == currentLevel.level && blocked.map == currentLevel.map;
		});
	if (orbitBlockedOnLevel)
		return true;

	StratEntity *croc = api->GetEntity(crocObjRef);
	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *boss = api->GetEntity(bossObjRef);
	StratEntity *dialog = api->GetEntity(dialogObjRef);

	// Disable during in-game events
	if (*movementAllowedState != 0)
		return true;

	// Vehicles
	if (strcmp(croc->name, "ClockworkGobbo") == 0)
		return true;
	if (strcmp(croc->name, "Croc Snowball PC") == 0)
		return true;
	if (strcmp(croc->name, "Croc Snowball") == 0)
		return true;
	if (strcmp(croc->name, "HangGlider PC") == 0)
		return true;
	if (strcmp(croc->name, "DPPlane") == 0)
		return true;
	if (orbitVehicleRearViewCamera ? !api->GetInputs().flip : true) {
		bool isVehicle = strcmp(croc->name, "Croc In a Boat") == 0 || strcmp(croc->name, "Croc In a Car") == 0;
		if (isVehicle)
			return true;
	}

	// Cameras
	if (strcmp(camera->name, "MineCartCam2") == 0)
		return true;
	if (strcmp(camera->name, "GooManShoo Camera") == 0)
		return true;
	if (strcmp(camera->name, "DPCamera") == 0)
		return true;
	if (strcmp(camera->name, "Snowballfixedcam") == 0)
		return true;
	if (strcmp(camera->name, "SMP Camera") == 0)
		return true;

	// Binoc break fixes
	if (strcmp(camera->name, "CrocCannonCamera") == 0) // Keith
		return true;
	if (boss != nullptr && strcmp(boss->name, "VenusVonFlyTrappe") == 0 && strcmp(camera->name, "Blank") == 0) // Venus
		return true;

	// Climbing
	if (!(croc->flags0 & (1 << 5)) && (croc->flags1 & (1 << 26)))
		return true;

	return false;
}

void orbitCameraOverridesPreAutoTurn(int input_rot_yaw, int input_rot_pitch) {
	StratEntity *croc = api->GetEntity(crocObjRef);
	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *boss = api->GetEntity(bossObjRef);
	StratEntity *dialog = api->GetEntity(dialogObjRef);

	// Flavio override
	if (strcmp(croc->name, "FlavioCroc") == 0) {
		orbitCameraCurrentDistance = 6000.0;
		cameraPitch = min(max(cameraPitch, 0.75), 0.9);
		orbitAutoTurn = false;
	}

	// Boss Cameras
	if (orbitImprovedBossCamera) {
		// Dante
		if (levelInfo->tribe == 4 && levelInfo->level == 2 && levelInfo->map == 1 && levelInfo->type == WAD_TYPE_BOSS && dialog != nullptr) {
			StratEntity *dante = api->FindEntity("DFDante");
			if (dante != nullptr) {
				if (input_rot_yaw == 0 && input_rot_pitch == 0) {
					Vec3i positionDiff = {
						dante->newPosition.x - croc->newPosition.x,
						dante->newPosition.y - croc->newPosition.y,
						dante->newPosition.z - croc->newPosition.z};
					cameraYaw = atan2(positionDiff.x, positionDiff.z) + PI;
					orbitAutoTurn = false;
				}
			}
		}
		// Goo Man Chu
		if (levelInfo->tribe == 4 && levelInfo->level == 6 && levelInfo->map == 1 && levelInfo->type == WAD_TYPE_LEVEL) {
			StratEntity *gooManChu = api->GetEntity(dialogObjRef);
			if (gooManChu != nullptr && strcmp(gooManChu->name, "Blank") == 0) {
				orbitCameraCurrentDistance = 1200.0;
				if (input_rot_yaw == 0 && input_rot_pitch == 0) {
					Vec3i positionDiff = {
						gooManChu->newPosition.x - croc->newPosition.x,
						gooManChu->newPosition.y - croc->newPosition.y,
						gooManChu->newPosition.z - croc->newPosition.z};
					cameraYaw = atan2(positionDiff.x, positionDiff.z) + PI;
					orbitAutoTurn = false;
				}
			}
		}
		// Flavio
		if (strcmp(croc->name, "FlavioCroc") == 0) {
			StratEntity *flavio = api->GetEntity(bossObjRef);
			if (flavio != nullptr) {
				orbitCameraCurrentDistance = 8000.0;
				cameraPitch = min(max(cameraPitch, 0.75), 0.9);
				Vec3i positionDiff = {
					flavio->newPosition.x - croc->newPosition.x,
					flavio->newPosition.y - croc->newPosition.y,
					flavio->newPosition.z - croc->newPosition.z};
				cameraYaw = atan2(positionDiff.x, positionDiff.z) + PI;
				orbitAutoTurn = false;
			}
		}
	}

	// Vehicle rear view camera
	if (orbitVehicleRearViewCamera) {
		bool isVehicle = strcmp(croc->name, "Croc In a Boat") == 0 || strcmp(croc->name, "Croc In a Car") == 0;
		if (isVehicle && api->GetInputs().flip) {
			api->LogDebug("Vehicle rear view camera active");
			orbitCameraResetYaw();
			orbitCameraResetPitch();
			cameraYaw += PI;
		}
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

	switch (cameraMode) {
	case CameraMode::Normal: {

		api->LogInfo("Camera mode: Normal");
		api->ShowInfoToast("Camera mode: Normal");

		break;
	}
	case CameraMode::Orbit: {

		// Get camera position and lookat
		_cameraRotPos = camera->newRotPos;
		orbitHasLastCrocPos = false;

		// Get rotation from lookat
		cameraYaw = -(double)(cameraRot->y) / 2048.0 * PI;
		cameraPitch = (double)(cameraRot->x) / 2048.0 * PI;

		api->LogInfo("Camera mode: Orbit");
		api->ShowInfoToast("Camera mode: Orbit");

		break;
	}
	case CameraMode::Freecam: {

		// Get camera position and lookat
		_cameraRotPos = camera->newRotPos;
		_cameraLookAt.x = cameraLookAt->x;
		_cameraLookAt.y = cameraLookAt->y;
		_cameraLookAt.z = cameraLookAt->z;

		// Get rotation from lookat
		cameraYaw = -(double)(cameraRot->y) / 2048.0 * PI;
		cameraPitch = (double)(cameraRot->x) / 2048.0 * PI;

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

static DWORD WINAPI delayedOrbitCameraResetThread(LPVOID param) {
	int delay = (int)param;
	Sleep(delay);
	orbitCameraResetTransitions();
	return 0;
}

void __stdcall mapDoorChangeOrbitCameraReset() {
	CreateThread(nullptr, 0, delayedOrbitCameraResetThread, (LPVOID)100, 0, nullptr);
}

void __stdcall deathChangeOrbitCameraReset() {
	CreateThread(nullptr, 0, delayedOrbitCameraResetThread, (LPVOID)700, 0, nullptr);
}

void __stdcall PhysicsLoop() {

	// Don't touch the camera on the main menu
	if (levelInfo->tribe == 0)
		return;

	// Inputs
	Inputs inputs = api->GetInputs();
	Inputs inputsPressed = api->GetInputsPressed();
	Inputs inputsReleased = api->GetInputsReleased();
	uint32_t controlMethod = api->GetCurrentSaveSlot()->controlMethod;

	ModernInput modernInput = api->GetModernInputState();
	const bool modernInputEnabled = modernInput.config.enabled;
	const float stickScale = modernInput.config.stickScale;
	const float triggerScale = modernInput.config.triggerScale;

	// Entities
	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *croc = api->GetEntity(crocObjRef);

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

		orbitAutoTurn = orbitAutoTurnSaved;

		if (camera == nullptr)
			return;

		if (croc == nullptr)
			return;

		if (inputs.flip || *binocsActive) {
			orbitCameraResetButton();
		}

		if (controlMethod == CTRL_TYPE_2) {
			if (inputs.effectiveDown || inputs.effectiveUp || inputs.effectiveLeft || inputs.effectiveRight) {
				centerOnType2Current = centerOnType2;
			}

			if (modernInput.rightStick.x != 0 || modernInput.rightStick.y != 0) {
				centerOnType2Current = false;
			}

			if (centerOnType2Current) {
				orbitCameraResetYaw();
			}
		}

		// Zoom controls
		if (modernInputEnabled) {
			switch (orbitZoomControls) {
			case ZOOM_TRIGGERS: {
				orbitCameraDistance += ((modernInput.leftTrigger - modernInput.rightTrigger) / triggerScale) * CAMERA_ZOOM_SPEED;
				orbitCameraDistance = min(max(orbitCameraDistance, ORBIT_MIN_DISTANCE), ORBIT_MAX_DISTANCE);
				break;
			}
			case ZOOM_RIGHT_STICK_CLICK: {
				static bool wasRightStickClicked = false;
				if (modernInput.rightStick.click && !wasRightStickClicked) {
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
				} else if (!modernInput.rightStick.click) {
					wasRightStickClicked = false;
				}
				break;
			}
			}
		}

		orbitCameraCurrentDistance = orbitCameraDistance;

		// Camera rotation
		float input_rot_yaw, input_rot_pitch;
		if (modernInputEnabled) {
			input_rot_yaw = ((float)modernInput.rightStick.x / stickScale) * (orbitInvertX ? -1 : 1);
			input_rot_pitch = ((float)modernInput.rightStick.y / stickScale) * (orbitInvertY ? -1 : 1);
		} else {
			input_rot_yaw = (inputs.stepRight - inputs.stepLeft) * (orbitInvertX ? -1 : 1);
			input_rot_pitch = (inputs.invRight - inputs.invLeft) * (orbitInvertY ? -1 : 1);
		}

		cameraYaw += -input_rot_yaw * CAMERA_YAW_SPEED;
		cameraPitch += input_rot_pitch * CAMERA_PITCH_SPEED;
		cameraPitch = min(max(cameraPitch, MIN_CAMERA_PITCH), MAX_CAMERA_PITCH);

		orbitCameraOverridesPreAutoTurn(input_rot_yaw, input_rot_pitch);

		// Auto turn
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

		if (orbitCameraDisabled()) {
			orbitCameraResetButton();
			return;
		}

		// Apply camera position
		double offsetX = cos(cameraPitch) * sin(cameraYaw) * orbitCameraCurrentDistance;
		double offsetY = sin(cameraPitch) * orbitCameraCurrentDistance;
		double offsetZ = cos(cameraPitch) * cos(cameraYaw) * orbitCameraCurrentDistance;

		_cameraRotPos.position.x = croc->newRotPos.position.x + static_cast<int>(offsetX);
		_cameraRotPos.position.y = static_cast<int>(CAMERA_ORBIT_Y_OFFSET) + croc->newRotPos.position.y + static_cast<int>(offsetY);
		_cameraRotPos.position.z = croc->newRotPos.position.z + static_cast<int>(offsetZ);

		camera->OldRotPos = _cameraRotPos;
		camera->newRotPos = _cameraRotPos;
		cameraPos->x = _cameraRotPos.position.x;
		cameraPos->y = _cameraRotPos.position.y;
		cameraPos->z = _cameraRotPos.position.z;

		break;
	}
	case CameraMode::Freecam: {

		if (camera == nullptr)
			return;

		float input_x, input_y, input_z;
		float input_rot_yaw, input_rot_pitch;
		if (modernInputEnabled) {
			input_x = -((float)modernInput.leftStick.x / stickScale);
			input_z = ((float)modernInput.leftStick.y / stickScale);
			input_y = ((float)modernInput.rightTrigger / triggerScale) - ((float)modernInput.leftTrigger / triggerScale);
			input_rot_yaw = ((float)modernInput.rightStick.x / stickScale) * (freecamInvertX ? -1 : 1);
			input_rot_pitch = ((float)modernInput.rightStick.y / stickScale) * (freecamInvertY ? -1 : 1);
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

		_cameraRotPos.position.x += static_cast<int>(forwards_x * input_x * 100);
		_cameraRotPos.position.z += static_cast<int>(forwards_z * input_x * 100);

		double sidewards_x = cos(cameraYaw + PI / 2);
		double sidewards_z = sin(cameraYaw + PI / 2);

		_cameraRotPos.position.x += static_cast<int>(sidewards_x * input_z * 100);
		_cameraRotPos.position.y += static_cast<int>(input_y * 100);
		_cameraRotPos.position.z += static_cast<int>(sidewards_z * input_z * 100);

		_cameraLookAt.x = _cameraRotPos.position.x + static_cast<int>(sin(-cameraYaw) * cos(-cameraPitch) * 100.0);
		_cameraLookAt.y = _cameraRotPos.position.y + static_cast<int>(sin(-cameraPitch) * 100.0);
		_cameraLookAt.z = _cameraRotPos.position.z + static_cast<int>(cos(-cameraYaw) * cos(-cameraPitch) * 100.0);

		camera->OldRotPos = _cameraRotPos;
		camera->newRotPos = _cameraRotPos;
		cameraPos->x = _cameraRotPos.position.x;
		cameraPos->y = _cameraRotPos.position.y;
		cameraPos->z = _cameraRotPos.position.z;
		cameraLookAt->x = _cameraLookAt.x;
		cameraLookAt->y = _cameraLookAt.y;
		cameraLookAt->z = _cameraLookAt.z;
		cameraRot->x = RadiansToGameRotation(-cameraPitch);
		cameraRot->y = RadiansToGameRotation(-cameraYaw);
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
		orbitAutoTurnSaved = api->SetupIniBool(L"OrbitCamera", L"AutoTurn", true);
		orbitAutoTurnStrength = min(max(api->SetupIniInt(L"OrbitCamera", L"AutoTurnStrength", 40), 0), 100);
		orbitAutoTurnMinSpeed = max(api->SetupIniInt(L"OrbitCamera", L"AutoTurnMinSpeed", 65), 0);

		const int zoomControlsValue = min(max(api->SetupIniInt(L"OrbitCamera", L"ZoomControls", 1), static_cast<int>(ZOOM_NONE)), static_cast<int>(ZOOM_RIGHT_STICK_CLICK));
		orbitZoomControls = static_cast<ZoomControls>(zoomControlsValue);

		constexpr int orbitLevelBlocklistStringLength = 65536;
		std::unique_ptr<wchar_t[]> orbitLevelBlocklistWchar = std::make_unique<wchar_t[]>(orbitLevelBlocklistStringLength);
		api->ReadIniString(L"OrbitCamera", L"LevelBlocklist", L"", orbitLevelBlocklistWchar.get(), orbitLevelBlocklistStringLength);
		populateOrbitBlocklist(orbitLevelBlocklistWchar.get(), orbitLevelBlocklist);

		orbitDisableRestrictions = api->SetupIniBool(L"OrbitCamera", L"DisableRestrictions", false);

		centerOnType2 = api->SetupIniBool(L"OrbitCamera", L"CenterOnType2", true);
		centerOnType2Current = centerOnType2;

		orbitVehicleRearViewCamera = api->SetupIniBool(L"OrbitCamera", L"VehicleRearViewCamera", true);
		orbitImprovedBossCamera = api->SetupIniBool(L"OrbitCamera", L"ImprovedBossCamera", true);

		orbitButtonResetPitch = api->SetupIniBool(L"OrbitCamera", L"ButtonResetPitch", true);
		orbitButtonResetZoom = api->SetupIniBool(L"OrbitCamera", L"ButtonResetZoom", false);

		freecamInvertX = api->SetupIniBool(L"Freecam", L"InvertX", false);
		freecamInvertY = api->SetupIniBool(L"Freecam", L"InvertY", false);

		api->HookGame(GAME_HOOK_POST_STEP, PhysicsLoop);
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
