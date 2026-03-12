#include "ModApi.h"
#include <windows.h>
#include <iostream>
#include <sstream>
#include <vector>

ModApi* api = nullptr;

bool type1Flip = false;
bool type1FlipIsFlipping = false;
int type1FlipTimer = 0;

enum TypeSwitchMode {
	None = 0,
	Manual = 1,
	Automatic = 2
};
std::string typeSwitchModeNames[] = { "None", "Manual", "Automatic" };
TypeSwitchMode typeSwitchMode;

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
    Inputs inputs = api->GetInputs();
    Inputs inputsPressed = api->GetInputsPressed();

    bool controlScheme = (bool)GetControlScheme();

    // Auto control mode
    switch (typeSwitchMode) {
    case TypeSwitchMode::Manual: {
        bool changeControls = GetAsyncKeyState(VK_CAPITAL) & 1;

        if (changeControls) {
            SetControlScheme(!controlScheme);
            std::string controlSchemeMessage = "Control scheme: " + std::string(ControlSchemeNames[!controlScheme]);
            api->LogInfo(controlSchemeMessage.c_str());
            api->ShowInfoToast(controlSchemeMessage.c_str());
        }
        break;
    }
    case TypeSwitchMode::Automatic: {
        bool anyInput = inputsPressed.up || inputsPressed.down || inputsPressed.left || inputsPressed.right;
        int analogStrength = api->AddressGetInt(ADDR_ANALOG_STRENGTH);

        // Started moving
        if (anyInput) {

            // Keyboard is used - analog strength is 181
            if (analogStrength == 181) {
                api->LogDebug("Using keyboard controls (Auto Mode)");
                SetControlScheme(CTRL_TYPE_2);
            }

            // Analog stick is being used
            else {
                api->LogDebug("Using analog controls (Auto Mode)");
                SetControlScheme(CTRL_TYPE_1);
            }

            std::string controlSchemeMessage = "Control scheme: " + std::string(ControlSchemeNames[GetControlScheme()]);
            api->LogDebug(controlSchemeMessage.c_str());

        }
        break;
    }
    }

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
        }
        else {
            if (--type1FlipTimer <= 0) {
                SetControlScheme(CTRL_TYPE_1);
                type1FlipIsFlipping = false;
            }
        }
    }

	// Max stick strength
	/*int analogStrength = api->AddressGetInt(ADDR_ANALOG_STRENGTH);
    if (analogStrength > 0 && analogStrength <= 180) {
		api->AddressSetInt(ADDR_ANALOG_STRENGTH, 180);
    }*/
}

void __stdcall toggleType1Flip() {
    type1Flip = !type1Flip;
	api->WriteIniBool(L"Config", L"Type1Flip", type1Flip);
}

MenuActionRegistration __stdcall toggleType1FlipRegistration() {
    static std::string label;
    label = std::string("Type 1 Flip: ") + (type1Flip ? "Enabled" : "Disabled");
    return { label.c_str(), "Do a 180 with Type 1 controls by double pressing Camera / 180.", toggleType1Flip, true};
}

void __stdcall toggleTypeSwitchMode() {
    switch (typeSwitchMode) {
    case TypeSwitchMode::None:
        typeSwitchMode = TypeSwitchMode::Manual;
        break;
    case TypeSwitchMode::Manual:
        typeSwitchMode = TypeSwitchMode::Automatic;
        break;
    case TypeSwitchMode::Automatic:
        typeSwitchMode = TypeSwitchMode::None;
        break;
    }
    api->WriteIniInt(L"Config", L"TypeSwitchMode", (int)typeSwitchMode);

    std::string typeSwitchMessage = "Type Switch Mode: " + typeSwitchModeNames[typeSwitchMode];
    api->LogInfo(typeSwitchMessage.c_str());
    api->ShowInfoToast(typeSwitchMessage.c_str());
}

MenuActionRegistration __stdcall toggleTypeSwitchRegistration() {
    static std::string label;
    label = std::string("Type Switch: ") + typeSwitchModeNames[typeSwitchMode];
    return { label.c_str(), "Enable manual [CAPSLOCK] or automatic control type switching.", toggleTypeSwitchMode, true};
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadModApi();
        if (!api) return FALSE;

        type1Flip = api->SetupIniBool(L"Config", L"Type1Flip", true);
        typeSwitchMode = (TypeSwitchMode)api->SetupIniInt(L"Config", L"TypeSwitchMode", TypeSwitchMode::Automatic);

        api->RegisterMenuAction(hModule, toggleType1FlipRegistration);
        api->RegisterMenuAction(hModule, toggleTypeSwitchRegistration);

        api->HookPhysics(PhysicsStep);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
