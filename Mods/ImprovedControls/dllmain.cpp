#include "ModApi.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <windows.h>

ModApi *api = nullptr;

bool type1Flip = false;
bool type1FlipIsFlipping = false;
int type1FlipTimer = 0;

int GetControlScheme() {
	SaveSlot *currentSaveSlot = api->GetCurrentSaveSlot();
	return currentSaveSlot->controlMethod;
}

void SetControlScheme(int scheme) {
	SaveSlot *currentSaveSlot = api->GetCurrentSaveSlot();
	currentSaveSlot->controlMethod = scheme;
}

void __stdcall PhysicsStep() {
	Inputs inputs = api->GetInputs();
	Inputs inputsPressed = api->GetInputsPressed();

	bool controlScheme = (bool)GetControlScheme();

	// Flip pressed
	if (type1Flip) {

		if (!type1FlipIsFlipping) {
			if (inputsPressed.flip) {
				if (controlScheme == CTRL_TYPE_1) {
					SetControlScheme(CTRL_TYPE_2);
					type1FlipIsFlipping = true;
					type1FlipTimer = PHYSICS_FPS;
				}
			}
		} else {
			if (--type1FlipTimer <= 0) {
				SetControlScheme(CTRL_TYPE_1);
				type1FlipIsFlipping = false;
			}
		}
	}
}

void __stdcall toggleType1Flip() {
	type1Flip = !type1Flip;
	api->WriteIniBool(L"Config", L"Type1Flip", type1Flip);
}

MenuActionRegistration __stdcall toggleType1FlipRegistration() {
	static std::string label;
	label = std::string("Type 1 Flip: ") + (type1Flip ? "Enabled" : "Disabled");
	return {label.c_str(), "Do a 180 with Type 1 controls by double pressing Camera / 180.", toggleType1Flip, true};
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		api = LoadModApi();
		if (!api)
			return FALSE;

		type1Flip = api->SetupIniBool(L"Config", L"Type1Flip", true);

		api->RegisterMenuAction(hModule, toggleType1FlipRegistration);

		api->HookGame(GAME_HOOK_POST_STEP, PhysicsStep);

		DisableThreadLibraryCalls(hModule);
	}
	return TRUE;
}
