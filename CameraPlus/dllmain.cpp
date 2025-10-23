#include "ModApi.h"
#include <Windows.h>
#include <iostream>

ModApi* api = nullptr;

enum class CameraMode {
    None = -1,
    Normal = 0,
    Orbit = 1,
    Freecam = 2
};
CameraMode cameraMode = CameraMode::Normal;
bool noclipCameraFlag = false;

RotPos3i cameraRotPos;
double cameraYaw = 0;
double cameraPitch = 0;
Vec3i cameraLookAt;

const double PI = 3.141;

const double CAMERA_ORBIT_Y_OFFSET = 800.0;
const double CAMERA_ORBIT_DISTANCE = 2400.0;

bool orbitInvertX;
bool orbitInvertY;

bool freecamInvertX;
bool freecamInvertY;

static double LerpAngle(double a, double b, double t) {
    double diff = fmod(b - a + PI, 2 * PI) - PI;
    return a + diff * t;
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

void __stdcall cameraSet(CameraMode mode = CameraMode::None) {
    if (mode == CameraMode::None) {
        switch (cameraMode) {
        case CameraMode::Normal: cameraMode = CameraMode::Orbit; break;
        case CameraMode::Orbit: cameraMode = CameraMode::Freecam; break;
        case CameraMode::Freecam: cameraMode = CameraMode::Normal; break;
        }
    }
    else {
        cameraMode = mode;
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
        cameraRotPos = camera->newRotPos;

        // Get rotation from lookat
        cameraYaw = -(double)(api->AddressGetInt(ADDR_CAMERA_ROT_Y)) / 2048.0 * PI;
        cameraPitch = (double)(api->AddressGetInt(ADDR_CAMERA_ROT_X)) / 2048.0 * PI;

        api->LogInfo("Camera mode: Orbit");
        api->ShowInfoToast("Camera mode: Orbit");

        break;
    }
    case CameraMode::Freecam: {

        // Get camera position and lookat
        cameraRotPos = camera->newRotPos;
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

void __stdcall cameraSetNormal() {
    cameraSet(CameraMode::Normal);
}

void __stdcall cameraSetOrbit() {
    cameraSet(CameraMode::Orbit);
}

void __stdcall cameraSetFreecam() {
    cameraSet(CameraMode::Freecam);
}

void __stdcall PhysicsLoop() {

    // Inputs
    Inputs inputs = api->GetInputs();
    Inputs inputsPressed = api->GetInputsPressed();
    Inputs inputsReleased = api->GetInputsReleased();

    // Entities
    StratEntity* camera = api->GetEntity(ADDR_CAMERA_OBJ);
    StratEntity* croc = api->GetEntity(ADDR_CROC_OBJ);

    // Toggle
    if (inputs.stepLeft && inputs.stepRight && inputsPressed.flip) {
        cameraSet();
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
        if (inputsReleased.flip) {
            cameraRotPos = camera->newRotPos;
            const double x = (double)(api->AddressGetInt(ADDR_CAMERA_ROT_Y));
            cameraYaw = ((x + 2048.0) / 4096.0) * (2 * PI);
            cameraPitch = (double)(api->AddressGetInt(ADDR_CAMERA_ROT_X)) / 2048.0 * PI;
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

        int input_rot_yaw = (inputs.stepRight - inputs.stepLeft) * (orbitInvertX ? -1 : 1);
        int input_rot_pitch = (inputs.invRight - inputs.invLeft) * (orbitInvertY ? -1 : 1);

        cameraYaw += -input_rot_yaw * 0.1;
        cameraPitch += input_rot_pitch * 0.05;
        cameraPitch = min(max(cameraPitch, 0.1), 0.9);

        double offsetX = cos(cameraPitch) * sin(cameraYaw) * CAMERA_ORBIT_DISTANCE;
        double offsetY = sin(cameraPitch) * CAMERA_ORBIT_DISTANCE;
        double offsetZ = cos(cameraPitch) * cos(cameraYaw) * CAMERA_ORBIT_DISTANCE;

        cameraRotPos.position.x = croc->newRotPos.position.x + static_cast<int>(offsetX);
        cameraRotPos.position.y = static_cast<int>(CAMERA_ORBIT_Y_OFFSET) + croc->newRotPos.position.y + static_cast<int>(offsetY);
        cameraRotPos.position.z = croc->newRotPos.position.z + static_cast<int>(offsetZ);

        camera->OldRotPos = cameraRotPos;
        camera->newRotPos = cameraRotPos;
        api->AddressSetInt(ADDR_CAMERA_POS_X, cameraRotPos.position.x);
        api->AddressSetInt(ADDR_CAMERA_POS_Y, cameraRotPos.position.y);
        api->AddressSetInt(ADDR_CAMERA_POS_Z, cameraRotPos.position.z);

        break;
    }
    case CameraMode::Freecam: {

        int input_x = inputs.right - inputs.left;
        int input_z = -(inputs.down - inputs.up);
        int input_y = inputs.jump - inputs.attack;
        int input_rot_yaw = (inputs.stepRight - inputs.stepLeft) * (freecamInvertX ? -1 : 1);
        int input_rot_pitch = (inputs.invRight - inputs.invLeft) * (freecamInvertY ? -1 : 1);

        cameraYaw += input_rot_yaw * 0.1;
        cameraPitch += input_rot_pitch * 0.05;
        cameraPitch = min(max(cameraPitch, -0.9), 0.9);

        double forwards_x = -cos(cameraYaw);
        double forwards_z = -sin(cameraYaw);

        cameraRotPos.position.x += static_cast<int>(forwards_x * input_x * 100);
        cameraRotPos.position.z += static_cast<int>(forwards_z * input_x * 100);

        double sidewards_x = cos(cameraYaw + PI / 2);
        double sidewards_z = sin(cameraYaw + PI / 2);

        cameraRotPos.position.x += static_cast<int>(sidewards_x * input_z * 100);
        cameraRotPos.position.y += static_cast<int>(input_y * 100);
        cameraRotPos.position.z += static_cast<int>(sidewards_z * input_z * 100);

        cameraLookAt.x = cameraRotPos.position.x + static_cast<int>(sin(-cameraYaw) * cos(-cameraPitch) * 100.0);
        cameraLookAt.y = cameraRotPos.position.y + static_cast<int>(sin(-cameraPitch) * 100.0);
        cameraLookAt.z = cameraRotPos.position.z + static_cast<int>(cos(-cameraYaw) * cos(-cameraPitch) * 100.0);

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

MenuActionRegistration __stdcall cameraNormalRegistration() {
    return { "Normal Camera", "The standard camera mode.", cameraSetNormal, cameraMode != CameraMode::Normal };
}

MenuActionRegistration __stdcall cameraOrbitRegistration() {
    return { "Orbit Camera", "A modern manual camera mode.", cameraSetOrbit, cameraMode != CameraMode::Orbit };
}

MenuActionRegistration __stdcall cameraFreeRegistration() {
    return { "Free Camera", "Move the camera anywhere you want.", cameraSetFreecam, cameraMode != CameraMode::Freecam };
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadModApi();
        if (!api) return FALSE;

        orbitInvertX = api->SetupIniBool(L"OrbitCamera", L"InvertX", false);
        orbitInvertY = api->SetupIniBool(L"OrbitCamera", L"InvertY", false);

        freecamInvertX = api->SetupIniBool(L"Freecam", L"InvertX", false);
        freecamInvertY = api->SetupIniBool(L"Freecam", L"InvertY", false);

        api->HookPhysics(PhysicsLoop);
        api->RegisterMenuAction(hModule, cameraNormalRegistration);
        api->RegisterMenuAction(hModule, cameraOrbitRegistration);
        api->RegisterMenuAction(hModule, cameraFreeRegistration);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
