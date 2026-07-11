#include "AirTurning.h"

#include "Input/Input.h"
#include "Input/TypeSwitching.h"
#include "ModApi.h"
#include "Utils/Angle.h"
#include <string>
extern ModApi *api;

namespace {

int GetControlScheme() {
	SaveSlot *currentSaveSlot = api->GetCurrentSaveSlot();
	return currentSaveSlot->controlMethod;
}

} // namespace

namespace Input::AirTurning {

bool airStrafing;
bool airTurning;

bool hasLastCrocPos = false;
Vec3i lastCrocPos = {0, 0, 0};
float autoTurnMinSpeed = 65.0f;
float autoTurnStrength = 256.0f;

void __stdcall doAirTurning() {
	if (!airTurning || GetControlScheme() != CTRL_TYPE_1)
		return;

	StratEntity *croc = api->GetEntity(crocObjRef);
	if (croc != nullptr && strcmp(croc->name, "WalkingCroc") == 0 && croc->localVars[CROC_VAR_IN_AIR] == 4096) {
		if (!hasLastCrocPos) {
			lastCrocPos = croc->newRotPos.position;
			hasLastCrocPos = true;
		}

		const int moveX = croc->newRotPos.position.x - lastCrocPos.x;
		const int moveZ = croc->newRotPos.position.z - lastCrocPos.z;
		const double moveDistanceSq = (double)moveX * (double)moveX + (double)moveZ * (double)moveZ;
		const double minMoveDistanceSq = (double)autoTurnMinSpeed * (double)autoTurnMinSpeed;

		if (moveDistanceSq >= minMoveDistanceSq) {
			const double moveYaw = atan2((double)moveX, (double)moveZ);
			const double targetYaw = moveYaw;
			const double yawLerpSpeed = ((double)autoTurnStrength / 100.0) * 0.08;
			croc->newRotPos.rotation.y = Angle::RadiansToGameRotation(Angle::LerpAngle(Angle::GameRotationToRadians(croc->newRotPos.rotation.y >> 12), targetYaw, yawLerpSpeed)) << 12;
		}

		lastCrocPos = croc->newRotPos.position;
	}
}

const int turnSpeed = 64 << 12;

void __stdcall doAirStrafing() {
	if (!airStrafing || GetControlScheme() != CTRL_TYPE_2)
		return;

	StratEntity *croc = api->GetEntity(crocObjRef);
	if (croc != nullptr && strcmp(croc->name, "WalkingCroc") == 0) {
		if (croc->localVars[CROC_VAR_IN_AIR] == 4096) {
			Inputs inputs = api->GetInputs();
			if (inputs.stepLeft)
				croc->newRotPos.rotation.y += turnSpeed;
			if (inputs.stepRight)
				croc->newRotPos.rotation.y -= turnSpeed;
		}
	}
}

void Setup() {
	airTurning = api->SetupIniBool(L"Input", L"AirTurning", false);
	airStrafing = api->SetupIniBool(L"Input", L"AirStrafing", true);
	api->HookGame(GAME_HOOK_POST_STEP, &doAirTurning);
	api->HookGame(GAME_HOOK_POST_STEP, &doAirStrafing);
}

} // namespace Input::AirTurning
