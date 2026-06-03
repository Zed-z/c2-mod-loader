#include "ModApi.h"

#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>

ModApi *api = nullptr;

bool noclip_enabled = false;

int noclip_y = 0;

void __stdcall toggleNoclip() {
	noclip_enabled = !noclip_enabled;

	StratEntity *croc = api->GetEntity(crocObjRef);
	if (croc != nullptr) {
		noclip_y = croc->newRotPos.position.y;
	}

	// Skip collisions
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		// Croc2.exe+89BE5 - E8 26A5F7FF           - call Croc2.exe+4110
		const BYTE noclipDisabled[5] = {0xE8, 0x26, 0xA5, 0xF7, 0xFF};
		const BYTE noclipEnabled[5] = {0x90, 0x90, 0x90, 0x90, 0x90};
		api->PatchBytes(0x489BE5, noclip_enabled ? noclipEnabled : noclipDisabled, sizeof(noclipEnabled));
		break;
	}
	case GAMEVER_EU: {
		// Croc2.exe+8A765 - E8 A699F7FF           - call Croc2.exe+4110
		const BYTE noclipDisabled[5] = {0xE8, 0xA6, 0x99, 0xF7, 0xFF};
		const BYTE noclipEnabled[5] = {0x90, 0x90, 0x90, 0x90, 0x90};
		api->PatchBytes(0x48A765, noclip_enabled ? noclipEnabled : noclipDisabled, sizeof(noclipEnabled));
		break;
	}
	case GAMEVER_DEMO: {
		// Croc2.exe+890A5 - E8 66B0F7FF           - call Croc2.exe+4110
		const BYTE noclipDisabled[5] = {0xE8, 0x66, 0xB0, 0xF7, 0xFF};
		const BYTE noclipEnabled[5] = {0x90, 0x90, 0x90, 0x90, 0x90};
		api->PatchBytes(0x4890A5, noclip_enabled ? noclipEnabled : noclipDisabled, sizeof(noclipEnabled));
		break;
	}
	}

	api->LogInfo((std::string("Noclip: ") + (noclip_enabled ? "Enabled" : "Disabled")).c_str());
	api->ShowInfoToast((std::string("Noclip: ") + (noclip_enabled ? "Enabled" : "Disabled")).c_str());
}

void __stdcall PhysicsLoop() {

	// Inputs
	Inputs inputs = api->GetInputs();
	Inputs inputsPressed = api->GetInputsPressed();
	Inputs inputsReleased = api->GetInputsReleased();

	// Toggle
	if (inputs.stepLeft && inputs.stepRight && inputsPressed.attack) {
		toggleNoclip();
	}

	// Croc movement
	StratEntity *croc = api->GetEntity(crocObjRef);
	if (noclip_enabled && croc != nullptr) {
		noclip_y += 100 * (inputs.jump - inputs.attack);
		croc->newRotPos.position.y = noclip_y;
		croc->localVars[CROC_VAR_FALL_TIMER] = 0;
		croc->verticalVelocity = 0;
	}
}

MenuActionRegistration __stdcall toggleNoclipRegistration() {
	return {noclip_enabled ? "Disable Noclip" : "Enable Noclip", noclip_enabled ? "Disable noclip." : "Enable noclip.", toggleNoclip, true};
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		api = LoadModApi();
		if (!api)
			return FALSE;

		api->HookGame(GAME_HOOK_POST_STEP, PhysicsLoop);
		api->RegisterMenuAction(hModule, toggleNoclipRegistration);

		DisableThreadLibraryCalls(hModule);
	}
	return TRUE;
}
