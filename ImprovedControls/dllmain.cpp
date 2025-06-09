#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>

ModApi* api = nullptr;

bool type1flip = false;
bool hybridControls = false;
bool limbusMode = false;

#define CTRL_TYPE_1 1
#define CTRL_TYPE_2 0
std::string ControlSchemeNames[] = { "Type 1", "Type 2" };

#define CONTROL_SCHEME_1 0x52A5F4
#define CONTROL_SCHEME_2 0x60438C
#define CONTROL_SCHEME_3 0x52AE64

#define ANALOG_STRENGTH 0x52A54C

#define PHYSICS_FPS 30

#include <bitset>

Inputs inputs, prevInputs;

int hybridControlsTimer = 0;
bool type1flipIsFlipping = false;
int type1flipTimer = 0;


std::string InputsString(Inputs inputs) {
	std::bitset<32> bits(inputs.raw);
    std::ostringstream oss;
    oss << "<"
		<< "Bits: " << bits << ", "
		<< "Up: " << inputs.up << ", Down: " << inputs.down
		<< ", Left: " << inputs.left << ", Right: " << inputs.right << ", "
        << "Flip: " << inputs.flip << ", Step Left: " << inputs.stepLeft
        << ", Step Right: " << inputs.stepRight << ", Jump: " << inputs.jump
        << ", Attack: " << inputs.attack << ", Inv Left: " << inputs.invLeft
        << ", Inv Use: " << inputs.invUse << ", Inv Right: " << inputs.invRight
        << ">";
    return oss.str();
}

int GetControlScheme() {
    return api->AddressGetInt(CONTROL_SCHEME_1);
}

void SetControlScheme(int scheme) {
    api->AddressSetInt(CONTROL_SCHEME_1, scheme);
    api->AddressSetInt(CONTROL_SCHEME_2, scheme);
    api->AddressSetInt(CONTROL_SCHEME_3, scheme);
}

void __stdcall PhysicsStep() {
    bool changeControls = GetAsyncKeyState(VK_CAPITAL) & 1;

    prevInputs = inputs;
    inputs = api->GetInputs();

    bool controlScheme = (bool)GetControlScheme();

    if (changeControls) {
        if (controlScheme == CTRL_TYPE_1) {
			api->Log("Changing control scheme to Type 2");
        }
        else {
			api->Log("Changing control scheme to Type 1");
		}
        SetControlScheme(!controlScheme);
    }

    //api->Log(inputs.toString());

    // Hybrid controls
    if (hybridControls) {
        if (inputs.raw) {
            hybridControlsTimer++;
        }
        else {
            hybridControlsTimer = 0;
        }

        if (hybridControlsTimer > PHYSICS_FPS) {
            SetControlScheme(CTRL_TYPE_1);
        }
        else {
            SetControlScheme(CTRL_TYPE_2);
        }
    }

    // Limbus controls
    if (limbusMode) {

        bool anyInput = inputs.up || inputs.down || inputs.left || inputs.right;
        bool anyInputPrev = prevInputs.up || prevInputs.down || prevInputs.left || prevInputs.right;
        int analogStrength = api->AddressGetInt(ANALOG_STRENGTH);

        // Started moving
        if (anyInput && !anyInputPrev) {
			api->Log("Input started: " + InputsString(inputs));
			api->Log("Analog strength: " + std::to_string(analogStrength));

            // Keyboard is used - analog strength is 181
            if (analogStrength == 181) {
				api->Log("Using keyboard controls (Limbus Mode)");
                SetControlScheme(CTRL_TYPE_2);
            }

            // Analog stick is being used
            else {
				api->Log("Using analog controls (Limbus Mode)");
                SetControlScheme(CTRL_TYPE_1);
            }

            api->Log("Control scheme is: " + ControlSchemeNames[GetControlScheme()]);

        }
    }

    // Flip pressed
    if (type1flip) {

        if (!type1flipIsFlipping) {
            if (inputs.flip && !prevInputs.flip) {
                if (controlScheme == CTRL_TYPE_1) {
                    SetControlScheme(CTRL_TYPE_2);
                    type1flipIsFlipping = true;
                    type1flipTimer = PHYSICS_FPS;
                }
            }
        }
        else {
            if (--type1flipTimer <= 0) {
                SetControlScheme(CTRL_TYPE_1);
                type1flipIsFlipping = false;
            }
        }
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;

        type1flip = api->ReadIniBool(L"Config", L"Type1Flip", true);
        api->WriteIniBool(L"Config", L"Type1Flip", type1flip);

        hybridControls = api->ReadIniBool(L"Config", L"HybridControls", false);
        api->WriteIniBool(L"Config", L"HybridControls", hybridControls);

        limbusMode = api->ReadIniBool(L"Config", L"LimbusMode", false);
        api->WriteIniBool(L"Config", L"LimbusMode", limbusMode);

        api->HookPhysics(PhysicsStep);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
