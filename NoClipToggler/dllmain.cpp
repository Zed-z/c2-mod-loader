#include "ModApi.h"

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

enum class CameraMode {
    Normal, Orbit, Freecam
};
CameraMode cameraMode = CameraMode::Normal;
bool noclipCameraFlag = false;

RotPos3i cameraRotPos;
double cameraYaw = 0;
double cameraPitch = 0;
Vec3i cameraLookAt;

int noclip_y = 0;

const double PI = 3.141;

const double CAMERA_ORBIT_Y_OFFSET = 800.0;
const double CAMERA_ORBIT_DISTANCE = 2400.0;


static double LerpAngle(double a, double b, double t) {
    double diff = fmod(b - a + PI, 2 * PI) - PI;
    return a + diff * t;
}

void __stdcall toggleFreecam() {
	switch (cameraMode) {
	case CameraMode::Normal: cameraMode = CameraMode::Orbit; break;
	case CameraMode::Orbit: cameraMode = CameraMode::Freecam; break;
	case CameraMode::Freecam: cameraMode = CameraMode::Normal; break;
	}

    StratEntity* camera = api->GetEntity(ADDR_CAMERA_OBJ);
    StratEntity* croc = api->GetEntity(ADDR_CROC_OBJ);

    if (camera == nullptr) {
        cameraMode = CameraMode::Normal;
        api->LogError("Camera not found!");
        api->ShowErrorToast("No camera found!");
        return;
    }

    if (croc == nullptr) {
        cameraMode = CameraMode::Normal;
        api->LogError("Croc not found!");
        api->ShowErrorToast("Croc not found!");
        return;
    }

    switch (cameraMode) {
    case CameraMode::Normal: {

        api->LogInfo("Camera mode: Normal");
        api->ShowInfoToast("Camera mode: Normal");

        break;
    }
    case CameraMode::Orbit: {

        // Get camera position and lookat
        cameraRotPos = camera->OldRotPos;
        cameraLookAt.x = api->AddressGetInt(ADDR_CAMERA_LOOKAT_X);
        cameraLookAt.y = api->AddressGetInt(ADDR_CAMERA_LOOKAT_Y);
        cameraLookAt.z = api->AddressGetInt(ADDR_CAMERA_LOOKAT_Z);

        // Get rotation from lookat
        cameraYaw = -(double)(api->AddressGetInt(ADDR_CAMERA_ROT_Y)) / 2048.0 * PI;
        cameraPitch = (double)(api->AddressGetInt(ADDR_CAMERA_ROT_X)) / 2048.0 * PI;

        api->LogInfo("Camera mode: Orbit");
        api->ShowInfoToast("Camera mode: Orbit");

        break;
    }
    case CameraMode::Freecam: {

        // Get camera position and lookat
        cameraRotPos = camera->OldRotPos;
        cameraLookAt.x = api->AddressGetInt(ADDR_CAMERA_LOOKAT_X);
        cameraLookAt.y = api->AddressGetInt(ADDR_CAMERA_LOOKAT_Y);
        cameraLookAt.z = api->AddressGetInt(ADDR_CAMERA_LOOKAT_Z);

        // Get rotation from lookat
        cameraYaw = -(double)(api->AddressGetInt(ADDR_CAMERA_ROT_Y)) / 2048.0 * PI;
        cameraPitch = (double)(api->AddressGetInt(ADDR_CAMERA_ROT_X)) / 2048.0 * PI;

        api->LogInfo("Camera mode: Freecam");
        api->ShowInfoToast("Camera mode: Freecam");

        break;
    }
    }

    if (cameraMode == CameraMode::Freecam) {
        // Change camera flag
        noclipCameraFlag = camera->flags1 & (1 << 23);
        camera->flags1 &= ~(1 << 23);

        // Pause player movement
        croc->flags0 |= (1 << 4);
    }
    else {
        camera->flags1 |= (noclipCameraFlag << 23);
        croc->flags0 &= ~(1 << 4);
    }
}


void __stdcall toggleNoclip() {
    noclip_enabled = !noclip_enabled;

    StratEntity* croc = api->GetEntity(ADDR_CROC_OBJ);
    if (croc == nullptr) {
        noclip_enabled = false;
        return;
    }
    noclip_y = croc->newRotPos.position.y;

    if (noclip_enabled) {
        api->PatchBytes(patchAddress, patchBytes, sizeof(patchBytes));
        api->PatchBytes(fallingCode, fallingCodePatch, sizeof(fallingCodePatch));
        api->PatchBytes(jumpFalloffCode, jumpFalloffCodePatch, sizeof(jumpFalloffCodePatch));
        //api->PatchBytes(fallTimerCode, fallTimerCodePatch, sizeof(fallTimerCodePatch));
        api->LogInfo("Noclip enabled!");
        api->ShowInfoToast("Noclip enabled!");
    }
    else {
        api->PatchBytes(patchAddress, originalBytes, sizeof(originalBytes));
        api->PatchBytes(fallingCode, fallingCodeOriginal, sizeof(fallingCodeOriginal));
        api->PatchBytes(jumpFalloffCode, jumpFalloffCodeOriginal, sizeof(jumpFalloffCodeOriginal));
        //api->PatchBytes(fallTimerCode, fallTimerCodeOriginal, sizeof(fallTimerCodeOriginal));
        api->LogInfo("Noclip disabled!");
        api->ShowInfoToast("Noclip disabled!");
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

    // Inputs
    Inputs inputs = api->GetInputs();
    Inputs inputsPressed = api->GetInputsPressed();

    // Entities
    StratEntity* camera = api->GetEntity(ADDR_CAMERA_OBJ);
    StratEntity* croc = api->GetEntity(ADDR_CROC_OBJ);

    // Toggle
    if (inputs.stepLeft && inputs.stepRight && inputsPressed.attack) {
        toggleNoclip();
    }
    if (inputs.stepLeft&& inputs.stepRight && inputsPressed.flip) {
        toggleFreecam();
    }

    // Go up and down
    if (croc == nullptr) {
        noclip_enabled = false;
    }
    if (noclip_enabled) {
        noclip_y += 100 * (inputs.stepRight - inputs.stepLeft);
        croc->newRotPos.position.y = noclip_y;
    }

    // Camera
    switch (cameraMode) {
    case CameraMode::Normal: {
        break;
    }
    case CameraMode::Orbit: {

        if (inputs.flip) {
            return;
        }

        if (camera == nullptr) {
            cameraMode = CameraMode::Normal;
            api->LogError("No camera found!");
            api->ShowErrorToast("No camera found!");
            return;
        }
        
        if (croc == nullptr) {
            cameraMode = CameraMode::Normal;
            api->LogError("No player found!");
            api->ShowErrorToast("No player found!");
            return;
        }

        int input_rot_yaw = inputs.stepRight - inputs.stepLeft;
        int input_rot_pitch = inputs.invRight - inputs.invLeft;

        cameraYaw += -input_rot_yaw * 0.1;
        cameraPitch += input_rot_pitch * 0.05;
        cameraPitch = min(max(cameraPitch, -0.9), 0.9);

        double offsetX = cos(cameraPitch) * sin(cameraYaw) * CAMERA_ORBIT_DISTANCE;
        double offsetY = sin(cameraPitch) * CAMERA_ORBIT_DISTANCE;
        double offsetZ = cos(cameraPitch) * cos(cameraYaw) * CAMERA_ORBIT_DISTANCE;

        cameraRotPos.position.x = croc->newRotPos.position.x + static_cast<int>(offsetX);
        cameraRotPos.position.y = CAMERA_ORBIT_Y_OFFSET + croc->newRotPos.position.y + static_cast<int>(offsetY);
        cameraRotPos.position.z = croc->newRotPos.position.z + static_cast<int>(offsetZ);

        cameraLookAt.x = croc->newRotPos.position.x;
        cameraLookAt.y = CAMERA_ORBIT_Y_OFFSET + croc->newRotPos.position.y;
        cameraLookAt.z = croc->newRotPos.position.z;

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

        break;
    }
    case CameraMode::Freecam: {

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

        cameraRotPos.position.x += forwards_x * input_x * 100;
        cameraRotPos.position.z += forwards_z * input_x * 100;

        double sidewards_x = cos(cameraYaw + PI / 2);
        double sidewards_z = sin(cameraYaw + PI / 2);

        cameraRotPos.position.x += sidewards_x * input_z * 100;
        cameraRotPos.position.y += input_y * 100;
        cameraRotPos.position.z += sidewards_z * input_z * 100;

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

        break;
    }
    }
}


MenuActionRegistration __stdcall toggleNoclipRegistration() {
    return { noclip_enabled ? "Disable Noclip" : "Enable Noclip", noclip_enabled ? "Disable noclip." : "Enable noclip.", toggleNoclip, true};
}

MenuActionRegistration __stdcall toggleFreecamRegistration() {
    return { "Change Camera Mode", "Change the current camera mode.", toggleFreecam, true };
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadModApi();
        if (!api) return FALSE;

		api->HookPhysics(PhysicsLoop);
		api->RegisterMenuAction(hModule, toggleNoclipRegistration);
		api->RegisterMenuAction(hModule, toggleFreecamRegistration);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
