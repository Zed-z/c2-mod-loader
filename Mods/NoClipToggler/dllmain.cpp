#include "ModApi.h"

#include <iostream>
#include <sstream>
#include <windows.h>

ModApi *api = nullptr;

// Noclip
const uintptr_t patchAddress = 0x00489BE5;
const BYTE originalBytes[5] = {0xE8, 0x26, 0xA5, 0xF7, 0xFF};
const BYTE patchBytes[5] = {0x90, 0x90, 0x90, 0x90, 0x90};

// Disable falling
const uintptr_t fallingCode = 0x0047FD9F; // add ecx, -28
const BYTE fallingCodeOriginal[] = {0x83, 0xC1, 0xE4};
const BYTE fallingCodePatch[] = {0x90, 0x90, 0x90};

// Jump falloff (disable for moon jump)
const uintptr_t jumpFalloffCode = 0x0047FDCF; // add ecx, -14
const BYTE jumpFalloffCodeOriginal[] = {0x83, 0xC1, 0xEC};
const BYTE jumpFalloffCodePatch[] = {0x90, 0x90, 0x90};

// Disable fall timer (to prevent death)
const uintptr_t fallTimerCode = 0x00488162; // add [eax], 00001000
const BYTE fallTimerCodeOriginal[] = {0x81, 0x00, 0x00, 0x10, 0x00, 0x00};
const BYTE fallTimerCodePatch[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

bool noclip_enabled = false;

int noclip_y = 0;

void __stdcall toggleNoclip() {
	noclip_enabled = !noclip_enabled;

	StratEntity *croc = api->GetEntity(ADDR_CROC_OBJ);
	if (croc == nullptr) {
		noclip_enabled = false;
		return;
	}
	noclip_y = croc->newRotPos.position.y;

	if (noclip_enabled) {
		api->PatchBytes(patchAddress, patchBytes, sizeof(patchBytes));
		api->PatchBytes(fallingCode, fallingCodePatch, sizeof(fallingCodePatch));
		api->PatchBytes(jumpFalloffCode, jumpFalloffCodePatch, sizeof(jumpFalloffCodePatch));
		// api->PatchBytes(fallTimerCode, fallTimerCodePatch, sizeof(fallTimerCodePatch));
		api->LogInfo("Noclip enabled!");
		api->ShowInfoToast("Noclip enabled!");
	} else {
		api->PatchBytes(patchAddress, originalBytes, sizeof(originalBytes));
		api->PatchBytes(fallingCode, fallingCodeOriginal, sizeof(fallingCodeOriginal));
		api->PatchBytes(jumpFalloffCode, jumpFalloffCodeOriginal, sizeof(jumpFalloffCodeOriginal));
		// api->PatchBytes(fallTimerCode, fallTimerCodeOriginal, sizeof(fallTimerCodeOriginal));
		api->LogInfo("Noclip disabled!");
		api->ShowInfoToast("Noclip disabled!");
	}
}

void __stdcall PhysicsLoop() {

	// Inputs
	Inputs inputs = api->GetInputs();
	Inputs inputsPressed = api->GetInputsPressed();
	Inputs inputsReleased = api->GetInputsReleased();

	// Entities
	StratEntity *croc = api->GetEntity(ADDR_CROC_OBJ);

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

		api->HookPhysics(PhysicsLoop);
		api->RegisterMenuAction(hModule, toggleNoclipRegistration);

		DisableThreadLibraryCalls(hModule);
	}
	return TRUE;
}
