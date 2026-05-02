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
	if (croc == nullptr) {
		noclip_enabled = false;
		return;
	}
	noclip_y = croc->newRotPos.position.y;

	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		// Noclip
		// Croc2.exe+89BE5 - E8 26A5F7FF           - call Croc2.exe+4110
		const BYTE noclipDisabled[5] = {0xE8, 0x26, 0xA5, 0xF7, 0xFF};
		const BYTE noclipEnabled[5] = {0x90, 0x90, 0x90, 0x90, 0x90};
		api->PatchBytes(0x489BE5, noclip_enabled ? noclipEnabled : noclipDisabled, sizeof(noclipEnabled));

		// Disable falling
		// Croc2.exe+7FD9F - 83 C1 E4              - add ecx,-1C
		const BYTE fallingDisabled[] = {0x83, 0xC1, 0xE4};
		const BYTE fallingEnabled[] = {0x90, 0x90, 0x90};
		api->PatchBytes(0x47FD9F, noclip_enabled ? fallingEnabled : fallingDisabled, sizeof(fallingEnabled));
		break;
	}
	case GAMEVER_EU: {
		api->LogWarning("Not implemented.");
		break;
	}
	case GAMEVER_DEMO: {
		api->LogWarning("Not implemented.");
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

	// Entities
	StratEntity *croc = api->GetEntity(crocObjRef);

	// Toggle
	if (inputs.stepLeft && inputs.stepRight && inputsPressed.attack) {
		toggleNoclip();
	}

	// Go up and down
	if (croc == nullptr) {
		noclip_enabled = false;
	}
	if (noclip_enabled) {
		noclip_y += 100 * (inputs.stepRight - inputs.stepLeft);
		croc->newRotPos.position.y = noclip_y;
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

		api->HookGame(GAME_HOOK_PHYSICS, PhysicsLoop);
		api->RegisterMenuAction(hModule, toggleNoclipRegistration);

		DisableThreadLibraryCalls(hModule);
	}
	return TRUE;
}
