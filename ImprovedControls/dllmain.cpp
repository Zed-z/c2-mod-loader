#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>

ModApi* api = nullptr;

bool type1flip = false;
bool hybridControls = false;
bool limbusMode = false;

#include <bitset>

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
    int saveSlotOffset = api->AddressGetInt(ADDR_CURRENT_SAVE_SLOT) * ADDR_SAVE_SLOT_OFFSET;
    return api->AddressGetInt(ADDR_CONTROL_SCHEME_SLOT + saveSlotOffset);
}

void SetControlScheme(int scheme) {
    int saveSlotOffset = api->AddressGetInt(ADDR_CURRENT_SAVE_SLOT) * ADDR_SAVE_SLOT_OFFSET;
    api->AddressSetInt(ADDR_CONTROL_SCHEME_SLOT + saveSlotOffset, scheme);
    api->AddressSetInt(ADDR_CONTROL_SCHEME_COPY1 + saveSlotOffset, scheme);
    api->AddressSetInt(ADDR_CONTROL_SCHEME_COPY2 + saveSlotOffset, scheme);
}

void __stdcall PhysicsStep() {
    bool changeControls = GetAsyncKeyState(VK_CAPITAL) & 1;

    Inputs inputs = api->GetInputs();
    Inputs inputsPressed = api->GetInputsPressed();

    bool controlScheme = (bool)GetControlScheme();

    if (changeControls) {
        if (controlScheme == CTRL_TYPE_1) {
			api->LogInfo("Changing control scheme to Type 2");
        }
        else {
			api->LogInfo("Changing control scheme to Type 1");
		}
        SetControlScheme(!controlScheme);
    }

    //api->LogDebug(inputs.toString());

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

        bool anyInput = inputsPressed.up || inputsPressed.down || inputsPressed.left || inputsPressed.right;
        int analogStrength = api->AddressGetInt(ADDR_ANALOG_STRENGTH);

        // Started moving
        if (anyInput) {
			api->LogDebug("Input started: " + InputsString(inputsPressed));
			api->LogDebug("Analog strength: " + std::to_string(analogStrength));

            // Keyboard is used - analog strength is 181
            if (analogStrength == 181) {
				api->LogDebug("Using keyboard controls (Limbus Mode)");
                SetControlScheme(CTRL_TYPE_2);
            }

            // Analog stick is being used
            else {
				api->LogDebug("Using analog controls (Limbus Mode)");
                SetControlScheme(CTRL_TYPE_1);
            }

            api->LogDebug("Control scheme is: " + std::string(ControlSchemeNames[GetControlScheme()]));

        }
    }

    // Flip pressed
    if (type1flip) {

        if (!type1flipIsFlipping) {
            if (inputsPressed.flip) {
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

	// Max stick strength
	/*int analogStrength = api->AddressGetInt(ADDR_ANALOG_STRENGTH);
    if (analogStrength > 0 && analogStrength <= 180) {
		api->AddressSetInt(ADDR_ANALOG_STRENGTH, 180);
    }*/
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadModApi();
        if (!api) return FALSE;

        type1flip = api->SetupIniBool(L"Config", L"Type1Flip", true);
        hybridControls = api->SetupIniBool(L"Config", L"HybridControls", false);
        limbusMode = api->SetupIniBool(L"Config", L"LimbusMode", false);

        api->HookPhysics(PhysicsStep);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
