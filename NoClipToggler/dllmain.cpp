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
StratEntity* croc = nullptr;

RotPos3i cameraRotPos;
double cameraYaw = 0;
double cameraPitch = 0;
Vec3i cameraLookAt;

int noclip_y = 0;

const double PI = 3.141;


void __stdcall toggleFreecam() {
    freecam_enabled = !freecam_enabled;

    camera = api->GetEntity(ADDR_CAMERA_OBJ);
    croc = api->GetEntity(ADDR_CROC_OBJ);

    if (camera == nullptr) {
        freecam_enabled = false;
        api->Log("No camera found!");
        api->ShowToast("No camera found!");
        return;
    }

    if (freecam_enabled) {

        cameraRotPos = camera->OldRotPos;
        cameraLookAt.x = api->AddressGetInt(ADDR_CAMERA_LOOKAT_X);
        cameraLookAt.y = api->AddressGetInt(ADDR_CAMERA_LOOKAT_Y);
        cameraLookAt.z = api->AddressGetInt(ADDR_CAMERA_LOOKAT_Z);

        camera->flags1 &= ~(1 << 23);

        // Pause player movement
        if (croc != nullptr) {
            croc->flags0 |= (1 << 4);
        }

        // Get rotation from lookat
        cameraYaw = -(double)(api->AddressGetInt(ADDR_CAMERA_ROT_Y)) / 2048.0 * PI;
        cameraPitch = (double)(api->AddressGetInt(ADDR_CAMERA_ROT_X)) / 2048.0 * PI;

        api->Log("Freecam enabled!");
        api->ShowToast("Freecam enabled!");
    }
    else {

        camera->flags1 |= (1 << 23);

        if (croc != nullptr) {
            croc->flags0 &= ~(1 << 4);
        }
        
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


int RadiansToGameRotation(double radians_input) {
    radians_input = fmod(radians_input, 2 * PI);
    while (radians_input <= -PI) {
        radians_input += 2 * PI;
    }
    while (radians_input > PI) {
        radians_input -= 2 * PI;
    }

    
    double scaled_value = radians_input * (2048.0 / PI);

    int game_rotation_value = static_cast<int>(round(scaled_value));

    if (game_rotation_value == 2048) {
        game_rotation_value = -2048;
    }

    return game_rotation_value;
}


void __stdcall PhysicsLoop() {

    Inputs inputs = api->GetInputs();
    Inputs inputsPressed = api->GetInputsPressed();

    // Toggle
    if (inputs.stepLeft && inputs.stepRight && inputsPressed.attack) {
        toggleNoclip();
    }
    if (inputs.stepLeft&& inputs.stepRight && inputsPressed.flip) {
        toggleFreecam();
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
            camera = nullptr;
            croc = nullptr;
            api->Log("No camera found!");
            api->ShowToast("No camera found!");
            return;
        }

        int input_x = inputs.right - inputs.left;
        int input_z = -(inputs.down - inputs.up);
        int input_y = inputs.jump - inputs.attack;
        int input_rot_yaw = inputs.stepRight - inputs.stepLeft;
        int input_rot_pitch = inputs.invRight - inputs.invLeft;

        cameraYaw += input_rot_yaw * 0.1;

        cameraPitch += input_rot_pitch * 0.05;
        cameraPitch = min(max(cameraPitch, -0.9), 0.9);

        double forwards_x = -cos(cameraYaw);
        double forwards_z = -sin(cameraYaw);

        double sidewards_x = cos(cameraYaw + PI / 2);
        double sidewards_z = sin(cameraYaw + PI / 2);

        cameraRotPos.position.x += forwards_x * input_x * 100;
        cameraRotPos.position.z += forwards_z * input_x * 100;

        cameraRotPos.position.x += sidewards_x * input_z * 100;
        cameraRotPos.position.z += sidewards_z * input_z * 100;

        cameraRotPos.position.y += input_y * 100;

        cameraLookAt.x = cameraRotPos.position.x + (sin(-cameraYaw) * cos(-cameraPitch) * 100.0);
        cameraLookAt.y = cameraRotPos.position.y + (sin(-cameraPitch) * 100.0);
        cameraLookAt.z = cameraRotPos.position.z + (cos(-cameraYaw) * cos(-cameraPitch) * 100.0);

        camera->OldRotPos = cameraRotPos;
        camera->newRotPos = cameraRotPos;
        api->AddressSetInt(ADDR_CAMERA_POS_X, cameraRotPos.position.x);
        api->AddressSetInt(ADDR_CAMERA_POS_Y, cameraRotPos.position.y);
        api->AddressSetInt(ADDR_CAMERA_POS_Z, cameraRotPos.position.z);

        api->AddressSetInt(ADDR_CAMERA_LOOKAT_X, cameraLookAt.x);
        api->AddressSetInt(ADDR_CAMERA_LOOKAT_Y, cameraLookAt.y);
        api->AddressSetInt(ADDR_CAMERA_LOOKAT_Z, cameraLookAt.z);

        api->AddressSetInt(ADDR_CAMERA_ROT_X, RadiansToGameRotation(-cameraPitch));
        api->AddressSetInt(ADDR_CAMERA_ROT_Y, RadiansToGameRotation(-cameraYaw));

        //api->Log("Camera Pos: " + std::to_string(cameraRotPos.position.x) + " " + std::to_string(cameraRotPos.position.y) + " " + std::to_string(cameraRotPos.position.z));
        //api->Log("Camera LookAt: " + std::to_string(cameraLookAt.x) + " " + std::to_string(cameraLookAt.y) + " " + std::to_string(cameraLookAt.z));
        //api->Log("Camera Rot: " + std::to_string(cameraYaw) + " " + std::to_string(cameraPitch));
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
