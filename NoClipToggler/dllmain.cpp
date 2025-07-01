#include "ModApi.h"
#include "GameStructs.h"
#include <Windows.h>
#include <iostream>
#include <sstream>

ModApi* api = nullptr;

// Noclip
const uintptr_t patchAddress = 0x00489BE5;
const BYTE originalBytes[5] = { 0xE8, 0x26, 0xA5, 0xF7, 0xFF };
const BYTE patchBytes[5] = { 0x90, 0x90, 0x90, 0x90, 0x90 };

// Disable falling
const uintptr_t fallingCode = 0x0047FD9F; // add ecx, -28
const BYTE fallingCodeOriginal[] = { 0x83, 0xC1, 0xE4 };
const BYTE fallingCodePatch[] = { 0x90, 0x90, 0x90 };

// Jump falloff (disable for moon jump)
const uintptr_t jumpFalloffCode = 0x0047FDCF; // add ecx, -14
const BYTE jumpFalloffCodeOriginal[] = { 0x83, 0xC1, 0xEC };
const BYTE jumpFalloffCodePatch[] = { 0x90, 0x90, 0x90 };

// Disable fall timer (to prevent death)
const uintptr_t fallTimerCode = 0x00488162; // add [eax], 00001000
const BYTE fallTimerCodeOriginal[] = { 0x81, 0x00, 0x00, 0x10, 0x00, 0x00 };
const BYTE fallTimerCodePatch[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

bool noclip_enabled = false;
bool freecam_enabled = false;

StratEntity* camera = nullptr;
RotPos3i cameraRotPos;

int noclip_y = 0;

Inputs inputs, prevInputs;


void __stdcall toggleFreecam() {
    freecam_enabled = !freecam_enabled;

    auto addr = api->ResolveAddress(ADDR_CAMERA_OBJ);
    if (IsBadReadPtr((void*)addr, sizeof(uintptr_t))) {
        freecam_enabled = false;
        return;
    }
    camera = (StratEntity*)addr;
    if (IsBadReadPtr(camera, sizeof(StratEntity)) || IsBadReadPtr(camera->next, sizeof(StratEntity)) || camera->next == nullptr) {
        freecam_enabled = false;
        api->Log("No camera found!");
        api->ShowToast("No camera found!");
        return;
    }

    if (freecam_enabled) {
        camera = camera->next;
        cameraRotPos = camera->OldRotPos;

        api->Log("Freecam enabled!");
        api->ShowToast("Freecam enabled!");
    }
    else {

        api->Log("Freecam disabled!");
        api->ShowToast("Freecam disabled!");
    }
}


void __stdcall toggleNoclip() {
    noclip_enabled = !noclip_enabled;

    auto addr = api->ResolveAddress(ADDR_CROC_POS_Y);
    if (IsBadReadPtr((void*)addr, sizeof(uintptr_t))) {
        noclip_enabled = false;
        return;
    }
    noclip_y = api->AddressGetInt(addr);

    if (noclip_enabled) {
        api->PatchBytes(patchAddress, patchBytes, sizeof(patchBytes));
        api->PatchBytes(fallingCode, fallingCodePatch, sizeof(fallingCodePatch));
        api->PatchBytes(jumpFalloffCode, jumpFalloffCodePatch, sizeof(jumpFalloffCodePatch));
        //api->PatchBytes(fallTimerCode, fallTimerCodePatch, sizeof(fallTimerCodePatch));
        api->Log("Noclip enabled!");
        api->ShowToast("Noclip enabled!");
    }
    else {
        api->PatchBytes(patchAddress, originalBytes, sizeof(originalBytes));
        api->PatchBytes(fallingCode, fallingCodeOriginal, sizeof(fallingCodeOriginal));
        api->PatchBytes(jumpFalloffCode, jumpFalloffCodeOriginal, sizeof(jumpFalloffCodeOriginal));
        //api->PatchBytes(fallTimerCode, fallTimerCodeOriginal, sizeof(fallTimerCodeOriginal));
        api->Log("Noclip disabled!");
        api->ShowToast("Noclip disabled!");
    }
}


void __stdcall PhysicsLoop() {

    prevInputs = inputs;
    inputs = api->GetInputs();

    // Toggle
    if (inputs.stepLeft && inputs.stepRight && !prevInputs.attack && inputs.attack) {
        toggleNoclip();
    }

    // Go up and down
    if (noclip_enabled) {
        if (inputs.stepRight) {
            noclip_y += 100;
        }

        if (inputs.stepLeft) {
            noclip_y -= 100;
        }

        auto addr = api->ResolveAddress(ADDR_CROC_POS_Y);
        if (IsBadReadPtr((void*)addr, sizeof(uintptr_t))) {
            noclip_enabled = false;
            return;
        }
        api->AddressSetInt(
            addr,
            noclip_y
        );
    }

    // Freecam
    if (freecam_enabled) {

        if (IsBadReadPtr(camera, sizeof(StratEntity)) || camera == nullptr) {
            noclip_enabled = false;
            api->Log("No camera found!");
            api->ShowToast("No camera found!");
            return;
        }

        if (inputs.left) {
            cameraRotPos.position.x -= 100;
        }

        if (inputs.right) {
            cameraRotPos.position.x += 100;
        }

        if (inputs.up) {
            cameraRotPos.position.z -= 100;
        }

        if (inputs.down) {
            cameraRotPos.position.z += 100;
        }

        if (inputs.stepLeft) {
            cameraRotPos.position.y -= 100;
        }

        if (inputs.stepRight) {
            cameraRotPos.position.y += 100;
        }

        camera->newRotPos = cameraRotPos;
        
    }

}


MenuActionRegistration __stdcall toggleNoclipRegistration() {
    return { noclip_enabled ? "Disable Noclip" : "Enable Noclip", noclip_enabled ? "Disable noclip." : "Enable noclip.", toggleNoclip, true};
}

MenuActionRegistration __stdcall toggleFreecamRegistration() {
    return { freecam_enabled ? "Disable Freecam" : "Enable Freecam", freecam_enabled ? "Disable freecam." : "Enable freecam.", toggleFreecam, true };
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;

		api->HookPhysics(PhysicsLoop);
		api->RegisterMenuAction(hModule, toggleNoclipRegistration);
		api->RegisterMenuAction(hModule, toggleFreecamRegistration);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
