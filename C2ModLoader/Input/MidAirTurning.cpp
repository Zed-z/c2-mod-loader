#include "MidAirTurning.h"

#include "Input/Input.h"
#include "Input/TypeSwitching.h"
#include "ModApi.h"

extern ModApi *api;

namespace {

int GetControlScheme() {
	SaveSlot *currentSaveSlot = api->GetCurrentSaveSlot();
	return currentSaveSlot->controlMethod;
}

} // namespace

namespace Input::MidAirTurning {

bool midAirTurning;

void __stdcall doMidAirTurning() {
	if (!midAirTurning || GetControlScheme() != CTRL_TYPE_2)
		return;

	StratEntity *croc = api->GetEntity(crocObjRef);
	if (croc != nullptr && strcmp(croc->name, "WalkingCroc") == 0) {
		if (croc->localVars[CROC_VAR_IN_AIR] == 4096) {
			Inputs inputs = api->GetInputs();
			if (inputs.stepLeft)
				croc->newRotPos.rotation.y += 0x40000;
			if (inputs.stepRight)
				croc->newRotPos.rotation.y -= 0x40000;
		}
	}
}

void Setup() {
	midAirTurning = api->SetupIniBool(L"Input", L"MidAirTurning", true);
	api->HookGame(GAME_HOOK_POST_STEP, &doMidAirTurning);
}

} // namespace Input::MidAirTurning
