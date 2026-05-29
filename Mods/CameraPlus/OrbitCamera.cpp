#include "OrbitCamera.h"

#include "Utils.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "ModApi.h"
extern ModApi *api;

namespace {

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

bool orbitDisableRestrictions;
bool centerOnType2;
bool centerOnType2Current;
bool orbitVehicleRearViewCamera;
bool orbitImprovedBossCamera;
bool orbitButtonResetPitch;
bool orbitButtonResetZoom;

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
	cameraYaw = GameRotationToRadians(croc->newRotPos.rotation.y >> 0xc) - PI;
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
	if (orbitButtonResetPitch)
		orbitCameraResetPitch();
	if (orbitButtonResetZoom)
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
	if (orbitDisableRestrictions)
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
	StratEntity *croc = api->GetEntity(crocObjRef);
	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *boss = api->GetEntity(bossObjRef);
	StratEntity *dialog = api->GetEntity(dialogObjRef);

	// Flavio override
	if (strcmp(croc->name, "FlavioCroc") == 0) {
		cameraDistanceCurrent = 6000.0;
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
				cameraDistanceCurrent = 1200.0;
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
				cameraDistanceCurrent = 8000.0;
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
			orbitCameraResetYaw();
			orbitCameraResetPitch();
			cameraYaw += PI;
		}
	}

	if (croc != nullptr && strcmp(croc->name, "WalkingCroc") == 0) {
		if (croc->localVars[CROC_VAR_STATE] == CROC_STATE_CLIMBING_WALL) {
			api->LogDebug("Climbing wall");
			orbitCameraResetYaw();
			orbitCameraResetPitch();
		}
	}
}

bool orbitHasLastCrocPos = false;
Vec3i orbitLastCrocPos;

} // namespace

namespace OrbitCamera {

void Setup() {
	orbitDisableRestrictions = api->SetupIniBool(L"OrbitCamera", L"DisableRestrictions", false);

	centerOnType2 = api->SetupIniBool(L"OrbitCamera", L"CenterOnType2", true);
	centerOnType2Current = centerOnType2;

	orbitVehicleRearViewCamera = api->SetupIniBool(L"OrbitCamera", L"VehicleRearViewCamera", true);
	orbitImprovedBossCamera = api->SetupIniBool(L"OrbitCamera", L"ImprovedBossCamera", true);

	orbitButtonResetPitch = api->SetupIniBool(L"OrbitCamera", L"ButtonResetPitch", true);
	orbitButtonResetZoom = api->SetupIniBool(L"OrbitCamera", L"ButtonResetZoom", false);

	orbitInvertX = api->SetupIniBool(L"OrbitCamera", L"InvertX", false);
	orbitInvertY = api->SetupIniBool(L"OrbitCamera", L"InvertY", false);
	orbitAutoTurnSaved = api->SetupIniBool(L"OrbitCamera", L"AutoTurn", true);
	orbitAutoTurnStrength = min(max(api->SetupIniInt(L"OrbitCamera", L"AutoTurnStrength", 40), 0), 100);
	orbitAutoTurnMinSpeed = max(api->SetupIniInt(L"OrbitCamera", L"AutoTurnMinSpeed", 65), 0);

	const int zoomControlsValue = min(max(api->SetupIniInt(L"OrbitCamera", L"ZoomControls", 1), static_cast<int>(ZOOM_NONE)), static_cast<int>(ZOOM_RIGHT_STICK_CLICK));
	orbitZoomControls = static_cast<ZoomControls>(zoomControlsValue);

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
	cameraYaw = -(double)(cameraRot->y) / 2048.0 * PI;
	cameraPitch = (double)(cameraRot->x) / 2048.0 * PI;
}

void SwitchOut() {
}

void Step() {
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
			cameraDistance += ((modernInput.leftTrigger - modernInput.rightTrigger) / triggerScale) * CAMERA_ZOOM_SPEED;
			cameraDistance = min(max(cameraDistance, CAMERA_DISTANCE_MIN), CAMERA_DISTANCE_MAX);
			break;
		}
		case ZOOM_RIGHT_STICK_CLICK: {
			static bool wasRightStickClicked = false;
			if (modernInput.rightStick.click && !wasRightStickClicked) {
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
			} else if (!modernInput.rightStick.click) {
				wasRightStickClicked = false;
			}
			break;
		}
		}
	}

	cameraDistanceCurrent = cameraDistance;

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

} // namespace OrbitCamera
