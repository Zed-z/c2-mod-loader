#include "ModApi.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

ModApi *api = nullptr;

bool easyMode = false;

bool inAir = false;
bool inAirPrev = false;

bool inAirCurrently = false;
int inAirFrames = 0;
bool inAirAttacked = false;

bool isHovering = false;
int hoverTimer = 0;
int hoverDuration = 0;

void startHover(int length) {
	isHovering = true;
	hoverDuration = length;
	hoverTimer = 0;
	api->LogDebug(("Hover started with length " + std::to_string(hoverDuration)).c_str());
}

void endHover() {
	isHovering = false;
	inAirCurrently = false;
	api->LogDebug("Hover ended");
}

void __stdcall Step() {

	StratEntity *croc = api->GetEntity(crocObjRef);
	Inputs inputs = api->GetInputs();
	Inputs inputsPressed = api->GetInputsPressed();
	Inputs inputsReleased = api->GetInputsReleased();

	if (croc == nullptr || strcmp(croc->name, "WalkingCroc") != 0)
		return;

	inAirPrev = inAir;
	inAir = croc->localVars[CROC_VAR_IN_AIR] == 4096;

	// bool startedJump = !inAirPrev && inAir && inputsPressed.jump;
	bool startedJump = !inAirPrev && inAir && croc->verticalVelocity > 0;
	bool endedJump = inAirPrev && !inAir;

	if (isHovering) {
		if (!inputs.jump || hoverTimer >= hoverDuration) {
			endHover();
			return;
		}

		hoverTimer++;

		croc->localVars[CROC_VAR_STATE] = CROC_STATE_NORMAL;
		croc->localVars[CROC_VAR_FALL_TIMER] = 0;
		croc->verticalVelocity = 50;
		croc->newRotPos.position.y -= 40;

		return;
	}

	if (startedJump) {
		// api->LogDebug("Jump started");
		inAirCurrently = true;
		inAirAttacked = false;
		inAirFrames = 0;
		return;
	}
	if (endedJump) {
		// api->LogDebug("Jump ended");
		inAirCurrently = false;
		return;
	}

	if (inAirCurrently) {
		inAirFrames++;

		if (croc->localVars[CROC_VAR_STATE] == CROC_STATE_ATTACKING && !inAirAttacked) {
			inAirAttacked = true;
			// api->LogDebug(("Attack started in air on frame " + std::to_string(inAirFrames)).c_str());
		}

		if (inAirAttacked && inputsPressed.jump) {
			if (easyMode) {
				startHover(60);
			} else {
				switch (inAirFrames) {
				case 11: {
					startHover(60);
					break;
				}
				case 10: {
					startHover(30);
					break;
				}
				case 9: {
					startHover(15);
					break;
				}
				}
			}
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		api = LoadModApi();
		if (!api)
			return FALSE;

		easyMode = api->SetupIniBool(L"Config", L"EasyMode", false);

		api->HookGame(GAME_HOOK_POST_STEP, Step);

		DisableThreadLibraryCalls(hModule);
	}
	return TRUE;
}
