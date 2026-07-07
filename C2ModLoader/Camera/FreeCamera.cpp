#include "FreeCamera.h"

#include <algorithm>
#include <cmath>

#include "Camera/Utils.h"
#include "Input/Controls.h"
#include "Input/Input.h"

#include "ModApi.h"
extern ModApi *api;

using std::min, std::max, std::sin, std::cos;

namespace {

bool noclipCameraFlag = false;
bool disableCrocFlag = false;

bool invertX;
bool invertY;

RotPos3i _cameraRotPos;
Vec3i _cameraLookAt;

double cameraYaw = 0;
double cameraPitch = 0;

} // namespace

namespace Camera::FreeCamera {

using namespace Camera::Utils;

void Setup() {
	invertX = api->SetupIniBool(L"Freecam", L"InvertX", false);
	invertY = api->SetupIniBool(L"Freecam", L"InvertY", false);
}

void SwitchIn() {
	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *croc = api->GetEntity(crocObjRef);

	// Get camera position and lookat
	_cameraRotPos = camera->newRotPos;
	_cameraLookAt.x = cameraLookAt->x;
	_cameraLookAt.y = cameraLookAt->y;
	_cameraLookAt.z = cameraLookAt->z;

	// Get rotation from lookat
	cameraYaw = -(double)(cameraRot->y) / 2048.0 * PI;
	cameraPitch = (double)(cameraRot->x) / 2048.0 * PI;

	// Change camera flag
	noclipCameraFlag = camera->flags1 & (1 << 23);
	camera->flags1 &= ~(1 << 23);

	// Pause player movement
	disableCrocFlag = croc->flags0 & (1 << 4);
	croc->flags0 |= (1 << 4);
}

void SwitchOut() {
	StratEntity *camera = api->GetEntity(cameraObjRef);

	if (noclipCameraFlag) {
		camera->flags1 |= (1 << 23);
	} else {
		camera->flags1 &= ~(1 << 23);
	}

	StratEntity *croc = api->GetEntity(crocObjRef);

	if (disableCrocFlag) {
		croc->flags0 |= (1 << 4);
	} else {
		croc->flags0 &= ~(1 << 4);
	}
}

void Step() {
	// Inputs
	Inputs inputs = api->GetInputs();
	ModernInput modernInput = api->GetModernInputState();
	const bool modernInputEnabled = modernInput.config.enabled;
	const float stickScale = modernInput.config.stickScale;
	const float triggerScale = modernInput.config.triggerScale;

	// Entities
	StratEntity *camera = api->GetEntity(cameraObjRef);
	StratEntity *croc = api->GetEntity(crocObjRef);

	if (camera == nullptr)
		return;

	float input_x, input_y, input_z;
	float input_rot_yaw, input_rot_pitch;
	if (modernInputEnabled) {
		input_x = min(max(
						  -(Input::getAnalogStickX(Input::Controls::config.movement) / stickScale) + ((float)Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveRight) - (float)Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveLeft)),
						  -1.0f),
			1.0f);
		input_z = min(max(
						  (Input::getAnalogStickY(Input::Controls::config.movement) / stickScale) - ((float)Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveDown) - (float)Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveUp)),
						  -1.0f),
			1.0f);
		input_y = min(max(
						  ((float)Input::getButtonPressed(Input::Controls::config.jump)) - ((float)Input::getButtonPressed(Input::Controls::config.attack)) + ((float)Input::getKeyboardKeyPressed(Input::Controls::config.keyJump) - (float)Input::getKeyboardKeyPressed(Input::Controls::config.keyAttack)),
						  -1.0f),
			1.0f);
		input_rot_yaw = min(max(
								-(Input::getAnalogStickX(Input::Controls::config.camera) / stickScale) * (invertX ? -1 : 1) + ((float)Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraRight) - (float)Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraLeft)) * (invertX ? -1 : 1),
								-1.0f),
			1.0f);
		input_rot_pitch = min(max(
								  -(Input::getAnalogStickY(Input::Controls::config.camera) / stickScale) * (invertY ? -1 : 1) + ((float)Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraDown) - (float)Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraUp)) * (invertY ? -1 : 1),
								  -1.0f),
			1.0f);
	} else {
		input_x = inputs.right - inputs.left;
		input_z = -(inputs.down - inputs.up);
		input_y = inputs.jump - inputs.attack;
		input_rot_yaw = (inputs.stepRight - inputs.stepLeft) * (invertX ? -1 : 1);
		input_rot_pitch = (inputs.invRight - inputs.invLeft) * (invertY ? -1 : 1);
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
}

} // namespace Camera::FreeCamera
