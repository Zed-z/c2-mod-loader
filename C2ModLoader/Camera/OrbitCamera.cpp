#include "OrbitCamera.h"

#include "Input/Controls.h"
#include "Input/Input.h"
#include "Utils/Angle.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "ModApi.h"
extern ModApi *api;

using std::cos, std::sin, std::min, std::max, std::atan2;

namespace {
bool invertX;
bool invertY;

bool autoTurnSaved;
bool autoTurn;
int autoTurnStrength;
int autoTurnMinSpeed;

bool disableRestrictions;
bool centerOnType2;
bool centerOnType2Current;
bool vehicleRearViewCamera;
bool improvedBossCamera;
bool buttonResetPitch;
bool buttonResetZoom;

const double CAMERA_ORBIT_Y_OFFSET = 800.0;

const double CAMERA_YAW_SPEED = 0.15;
const double CAMERA_PITCH_SPEED = 0.05;
const double CAMERA_ZOOM_SPEED = 80.0;

double cameraYaw = 0;
double cameraPitch = 0;

const double CAMERA_DISTANCE_MIN = 1200.0;
const double CAMERA_DISTANCE_DEFAULT = 2400.0;
const double CAMERA_DISTANCE_MAX = 3600.0;
double cameraDistance = CAMERA_DISTANCE_DEFAULT;
double cameraDistanceCurrent = CAMERA_DISTANCE_DEFAULT;

const double WALKING_PITCH_MIN = -0.2;
const double WALKING_PITCH_DEFAULT = 0.3;
const double WALKING_PITCH_MAX = 0.9;
double walkingPitch = WALKING_PITCH_DEFAULT;

const double HANGING_PITCH_MIN = -0.9;
const double HANGING_PITCH_DEFAULT = -0.5;
const double HANGING_PITCH_MAX = -0.1;
double hangingPitch = HANGING_PITCH_DEFAULT;

RotPos3i _cameraRotPos;

void __stdcall orbitCameraResetYaw() {
	StratEntity *croc = api->GetEntity(crocObjRef);
	if (croc == nullptr) {
		api->LogDebug("[orbitCameraReset] Croc not found!");
		return;
	}
	cameraYaw = Angle::GameRotationToRadians(croc->newRotPos.rotation.y >> 0xc) - Angle::PI;
}

void __stdcall orbitCameraResetDistance() {
	cameraDistance = CAMERA_DISTANCE_DEFAULT;
}

void __stdcall orbitCameraResetPitch() {
	walkingPitch = WALKING_PITCH_DEFAULT;
	hangingPitch = HANGING_PITCH_DEFAULT;
}

void __stdcall orbitCameraResetTransitions() {
	orbitCameraResetYaw();
	orbitCameraResetPitch();
}

void __stdcall orbitCameraResetButton() {
	orbitCameraResetYaw();
	if (buttonResetPitch)
		orbitCameraResetPitch();
	if (buttonResetZoom)
		orbitCameraResetDistance();
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

bool orbitCameraDisabled() {
	if (disableRestrictions)
		return false;

	LevelInfo currentLevel = api->GetLevelInfo();
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
	if (vehicleRearViewCamera ? !api->GetInputs().flip : true) {
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

	// Event camera active
	if (
		currentWadFile != nullptr && *currentWadFile != nullptr &&
		(*currentWadFile)->chunkData != nullptr) {
		WadChunkData chunkData = (*currentWadFile)->chunkData->data;
		if (chunkData.cameraStackCount > 0)
			return true;
	}

	return false;
}

void orbitCameraOverridesPreAutoTurn(int input_rot_yaw, int input_rot_pitch) {
	uint32_t controlMethod = api->GetCurrentSaveSlot()->controlMethod;
	StratEntity *croc = api->GetEntity(crocObjRef);
	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *boss = api->GetEntity(bossObjRef);
	StratEntity *dialog = api->GetEntity(dialogObjRef);

	// Flavio override
	if (strcmp(croc->name, "FlavioCroc") == 0) {
		cameraDistanceCurrent = 6000.0;
		cameraPitch = min(max(cameraPitch, 0.75), 0.9);
		autoTurn = false;
	}

	// Boss Cameras
	if (improvedBossCamera) {
		// Dante
		if (levelInfo->tribe == 4 && levelInfo->level == 2 && levelInfo->map == 1 && levelInfo->type == WAD_TYPE_BOSS && dialog != nullptr) {
			StratEntity *dante = api->FindEntity("DFDante");
			if (dante != nullptr) {
				if (input_rot_yaw == 0 && input_rot_pitch == 0) {
					Vec3i positionDiff = {
						dante->newPosition.x - croc->newPosition.x,
						dante->newPosition.y - croc->newPosition.y,
						dante->newPosition.z - croc->newPosition.z};
					cameraYaw = atan2(positionDiff.x, positionDiff.z) + Angle::PI;
					autoTurn = false;
				}
			}
		}
		// Goo Man Chu
		if (levelInfo->tribe == 4 && levelInfo->level == 6 && levelInfo->map == 1 && levelInfo->type == WAD_TYPE_LEVEL) {
			StratEntity *gooManChu = api->GetEntity(dialogObjRef);
			if (gooManChu != nullptr && strcmp(gooManChu->name, "Blank") == 0) {
				cameraDistanceCurrent = 1200.0;
				if (input_rot_yaw == 0 && input_rot_pitch == 0) {
					Vec3i positionDiff = {
						gooManChu->newPosition.x - croc->newPosition.x,
						gooManChu->newPosition.y - croc->newPosition.y,
						gooManChu->newPosition.z - croc->newPosition.z};
					cameraYaw = atan2(positionDiff.x, positionDiff.z) + Angle::PI;
					autoTurn = false;
				}
			}
		}
		// Flavio
		if (controlMethod == CTRL_TYPE_1 && strcmp(croc->name, "FlavioCroc") == 0) {
			StratEntity *flavio = api->GetEntity(bossObjRef);
			if (flavio != nullptr) {
				cameraDistanceCurrent = 8000.0;
				cameraPitch = min(max(cameraPitch, 0.75), 0.9);
				Vec3i positionDiff = {
					flavio->newPosition.x - croc->newPosition.x,
					flavio->newPosition.y - croc->newPosition.y,
					flavio->newPosition.z - croc->newPosition.z};
				cameraYaw = atan2(positionDiff.x, positionDiff.z) + Angle::PI;
				autoTurn = false;
			}
		}
	}

	// Vehicle rear view camera
	if (vehicleRearViewCamera) {
		bool isVehicle = strcmp(croc->name, "Croc In a Boat") == 0 || strcmp(croc->name, "Croc In a Car") == 0;
		if (isVehicle && api->GetInputs().flip) {
			orbitCameraResetYaw();
			orbitCameraResetPitch();
			cameraYaw += Angle::PI;
		}
	}

	if (croc != nullptr && strcmp(croc->name, "WalkingCroc") == 0) {
		if (croc->localVars[CROC_VAR_STATE] == CROC_STATE_CLIMBING_WALL) {
			orbitCameraResetYaw();
			orbitCameraResetPitch();
		}
	}
}

bool orbitHasLastCrocPos = false;
Vec3i orbitLastCrocPos;

} // namespace

namespace Camera::OrbitCamera {

void Setup() {
	disableRestrictions = api->SetupIniBool(L"OrbitCamera", L"DisableRestrictions", false);

	centerOnType2 = api->SetupIniBool(L"OrbitCamera", L"CenterOnType2", true);
	centerOnType2Current = centerOnType2;

	vehicleRearViewCamera = api->SetupIniBool(L"OrbitCamera", L"VehicleRearViewCamera", true);
	improvedBossCamera = api->SetupIniBool(L"OrbitCamera", L"ImprovedBossCamera", true);

	buttonResetPitch = api->SetupIniBool(L"OrbitCamera", L"ButtonResetPitch", true);
	buttonResetZoom = api->SetupIniBool(L"OrbitCamera", L"ButtonResetZoom", false);

	invertX = api->SetupIniBool(L"OrbitCamera", L"InvertX", false);
	invertY = api->SetupIniBool(L"OrbitCamera", L"InvertY", false);
	autoTurnSaved = api->SetupIniBool(L"OrbitCamera", L"AutoTurn", true);
	autoTurnStrength = min(max(api->SetupIniInt(L"OrbitCamera", L"AutoTurnStrength", 40), 0), 100);
	autoTurnMinSpeed = max(api->SetupIniInt(L"OrbitCamera", L"AutoTurnMinSpeed", 65), 0);

	api->HookGame(GAME_HOOK_MAP_CHANGE, mapDoorChangeOrbitCameraReset);
	api->HookGame(GAME_HOOK_DOOR_CHANGE, mapDoorChangeOrbitCameraReset);
	api->HookGame(GAME_HOOK_PLAYER_DEATH, deathChangeOrbitCameraReset);
}

void SwitchIn() {
	StratEntity *camera = api->GetEntity(cameraObjRef);

	// Get camera position and lookat
	_cameraRotPos = camera->newRotPos;
	orbitHasLastCrocPos = false;

	// Get rotation from lookat
	cameraYaw = -(double)(cameraRot->y) / 2048.0 * Angle::PI;
	cameraPitch = (double)(cameraRot->x) / 2048.0 * Angle::PI;
}

void SwitchOut() {
}

void Step() {
	// Inputs
	Inputs inputs = api->GetInputs();
	uint32_t controlMethod = api->GetCurrentSaveSlot()->controlMethod;

	ModernInput modernInput = api->GetModernInputState();
	const bool modernInputEnabled = modernInput.config.enabled;
	const float stickScale = modernInput.config.stickScale;
	const float triggerScale = modernInput.config.triggerScale;

	float input_rot_yaw, input_rot_pitch;
	if (modernInputEnabled) {
		input_rot_yaw = min(max(
								-(Input::getAnalogStickX(Input::Controls::config.camera) / stickScale) * (invertX ? -1 : 1) + ((float)Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraRight) - (float)Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraLeft)) * (invertX ? -1 : 1),
								-1.0f),
			1.0f);
		input_rot_pitch = min(max(
								  -(Input::getAnalogStickY(Input::Controls::config.camera) / stickScale) * (invertY ? -1 : 1) + ((float)Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraDown) - (float)Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraUp)) * (invertY ? -1 : 1),
								  -1.0f),
			1.0f);
	} else {
		input_rot_yaw = (inputs.stepRight - inputs.stepLeft) * (invertX ? -1 : 1);
		input_rot_pitch = (inputs.invRight - inputs.invLeft) * (invertY ? -1 : 1);
	}

	// Entities
	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *croc = api->GetEntity(crocObjRef);

	autoTurn = autoTurnSaved;

	if (camera == nullptr)
		return;

	if (croc == nullptr)
		return;

	if (inputs.flip || *binocsActive) {
		orbitCameraResetButton();
	}

	// Type 2 camera behavior
	if (controlMethod == CTRL_TYPE_2) {
		if (inputs.effectiveDown || inputs.effectiveUp || inputs.effectiveLeft || inputs.effectiveRight) {
			centerOnType2Current = centerOnType2;
		}

		if (input_rot_yaw != 0 || input_rot_pitch != 0) {
			centerOnType2Current = false;
		}

		if (centerOnType2Current) {
			orbitCameraResetYaw();
		}
	}

	// Zoom controls
	if (modernInputEnabled) {
		static bool wasRightStickClicked = false;
		bool cameraButton = (Input::getButtonPressed(Input::Controls::config.cameraZoom) || Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraZoom));
		if (cameraButton && !wasRightStickClicked) {
			wasRightStickClicked = true;
			if (cameraDistance == CAMERA_DISTANCE_MAX) {
				cameraDistance = CAMERA_DISTANCE_MIN;
			} else if (cameraDistance == CAMERA_DISTANCE_MIN) {
				cameraDistance = CAMERA_DISTANCE_DEFAULT;
			} else if (cameraDistance == CAMERA_DISTANCE_DEFAULT) {
				cameraDistance = CAMERA_DISTANCE_MAX;
			} else {
				cameraDistance = CAMERA_DISTANCE_DEFAULT;
			}
		} else if (!cameraButton) {
			wasRightStickClicked = false;
		}
	}

	cameraDistanceCurrent = cameraDistance;

	// Camera rotation
	cameraYaw += -input_rot_yaw * CAMERA_YAW_SPEED;
	cameraPitch = walkingPitch;

	// Pitch per player state
	int crocState = (croc != nullptr && strcmp(croc->name, "WalkingCroc") == 0) ? croc->localVars[CROC_VAR_STATE] : CROC_STATE_NORMAL;
	switch (crocState) {
	default:
		walkingPitch = min(max(walkingPitch + input_rot_pitch * CAMERA_PITCH_SPEED, WALKING_PITCH_MIN), WALKING_PITCH_MAX);
		cameraPitch = walkingPitch;
		break;
	case CROC_STATE_HANGING_CEILING:
		hangingPitch = min(max(hangingPitch + input_rot_pitch * CAMERA_PITCH_SPEED, HANGING_PITCH_MIN), HANGING_PITCH_MAX);
		cameraPitch = hangingPitch;
		break;
	}

	orbitCameraOverridesPreAutoTurn(input_rot_yaw, input_rot_pitch);

	// Auto turn
	if (!orbitHasLastCrocPos) {
		orbitLastCrocPos = croc->newRotPos.position;
		orbitHasLastCrocPos = true;
	}

	const int moveX = croc->newRotPos.position.x - orbitLastCrocPos.x;
	const int moveZ = croc->newRotPos.position.z - orbitLastCrocPos.z;
	const double moveDistanceSq = (double)moveX * (double)moveX + (double)moveZ * (double)moveZ;
	const double minMoveDistanceSq = (double)autoTurnMinSpeed * (double)autoTurnMinSpeed;

	if (autoTurn && input_rot_yaw == 0 && moveDistanceSq >= minMoveDistanceSq) {
		const double moveYaw = atan2((double)moveX, (double)moveZ);
		const double targetYaw = moveYaw + Angle::PI;
		const double yawLerpSpeed = ((double)autoTurnStrength / 100.0) * 0.08;
		cameraYaw = Angle::LerpAngle(cameraYaw, targetYaw, yawLerpSpeed);
	}

	orbitLastCrocPos = croc->newRotPos.position;

	if (orbitCameraDisabled()) {
		orbitCameraResetButton();
		return;
	}

	// Apply camera position
	double offsetX = cos(cameraPitch) * sin(cameraYaw) * cameraDistanceCurrent;
	double offsetY = sin(cameraPitch) * cameraDistanceCurrent;
	double offsetZ = cos(cameraPitch) * cos(cameraYaw) * cameraDistanceCurrent;

	_cameraRotPos.position.x = croc->newRotPos.position.x + static_cast<int>(offsetX);
	_cameraRotPos.position.y = static_cast<int>(CAMERA_ORBIT_Y_OFFSET) + croc->newRotPos.position.y + static_cast<int>(offsetY);
	_cameraRotPos.position.z = croc->newRotPos.position.z + static_cast<int>(offsetZ);

	camera->OldRotPos = _cameraRotPos;
	camera->newRotPos = _cameraRotPos;
	cameraPos->x = _cameraRotPos.position.x;
	cameraPos->y = _cameraRotPos.position.y;
	cameraPos->z = _cameraRotPos.position.z;
}

} // namespace Camera::OrbitCamera
