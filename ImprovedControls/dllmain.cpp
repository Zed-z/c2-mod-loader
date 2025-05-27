#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>

ModApi* api = nullptr;

bool type1flip = false;
bool hybridControls = false;
bool limbusMode = false;

#define INPUTS 0x52A590

#define CTRL_TYPE_1 1
#define CTRL_TYPE_2 0

#define CONTROL_SCHEME_1 0x52A5F4
#define CONTROL_SCHEME_2 0x60438C

#define ANALOG_STRENGTH 0x52A54C

#define PHYSICS_FPS 30

#include <bitset>
struct Inputs {
    int integer;
    std::bitset<32> bits;

    bool pause;

    bool up;
    bool down;
    bool left;
    bool right;

    bool effectiveUp;
    bool effectiveDown;
    bool effectiveLeft;
    bool effectiveRight;

    bool flip;
    bool stepLeft;
    bool stepRight;
    bool jump;
    bool attack;
    bool invLeft;
    bool invUse;
    bool invRight;

    std::string toString() const {
        std::ostringstream oss;
        oss << "<"
			<< "Bits: " << bits << ", "
			<< "Up: " << up << ", Down: " << down
			<< ", Left: " << left << ", Right: " << right << ", "
            << "Flip: " << flip << ", Step Left: " << stepLeft
            << ", Step Right: " << stepRight << ", Jump: " << jump
            << ", Attack: " << attack << ", Inv Left: " << invLeft
            << ", Inv Use: " << invUse << ", Inv Right: " << invRight
            << ">";
        return oss.str();
	}
};
Inputs GetInputs(int input) {
    Inputs result;
    result.integer = input;
    result.bits = std::bitset<32>(input);

    result.pause = input & 8;

    result.up = input & 16;
    result.right = input & 32;
    result.down = input & 64;
    result.left = input & 128;

    result.invLeft = input & 256;
    result.invRight = input & 512;

    result.stepLeft = input & 1024;
    result.stepRight = input & 2048;

    result.invUse = input & 4096;
    result.flip = input & 8192;
    
    result.jump = input & 16384;
    result.attack = input & 32768;

    result.effectiveUp = input & 65536;
    result.effectiveDown = input & 131072;
    result.effectiveLeft = input & 262144;
    result.effectiveRight = input & 524288;
    
    return result;
}

Inputs inputs, prevInputs;

int hybridControlsTimer = 0;
bool type1flipIsFlipping = false;
int type1flipTimer = 0;

/*
    Croc2.exe+4A591 - 39 1D 0CA24A00        - cmp [Croc2.exe+AA20C],ebx { (1) }
*/
uintptr_t hookAddr = 0x0044A591;
size_t hookLength = 6;

void __stdcall PhysicsStep() {
    bool changeControls = GetAsyncKeyState(VK_CAPITAL) & 1;

    prevInputs = inputs;
    inputs = GetInputs(api->AddressGetInt(INPUTS));

    bool controlScheme = (bool)(api->AddressGetInt(CONTROL_SCHEME_1));

    if (changeControls) {
        if (controlScheme == CTRL_TYPE_1) {
			api->Log("Changing control scheme to Type 2");
        }
        else {
			api->Log("Changing control scheme to Type 1");
		}
        api->AddressSetInt(CONTROL_SCHEME_1, !controlScheme);
        api->AddressSetInt(CONTROL_SCHEME_2, !controlScheme);
    }

    //api->Log(inputs.toString());

    // Hybrid controls
    if (hybridControls) {
        if (inputs.integer) {
            hybridControlsTimer++;
        }
        else {
            hybridControlsTimer = 0;
        }

        if (hybridControlsTimer > PHYSICS_FPS) {
            api->AddressSetInt(CONTROL_SCHEME_1, CTRL_TYPE_1);
            api->AddressSetInt(CONTROL_SCHEME_2, CTRL_TYPE_1);
        }
        else {
            api->AddressSetInt(CONTROL_SCHEME_1, CTRL_TYPE_2);
            api->AddressSetInt(CONTROL_SCHEME_2, CTRL_TYPE_2);
        }
    }

    // Limbus controls
    if (limbusMode) {

        bool anyInput = inputs.up || inputs.down || inputs.left || inputs.right;
        bool anyInputPrev = prevInputs.up || prevInputs.down || prevInputs.left || prevInputs.right;
        int analogStrength = api->AddressGetInt(ANALOG_STRENGTH);

        // Started moving
        if (anyInput && !anyInputPrev) {
			api->Log("Input started: " + inputs.toString());
			api->Log("Analog strength: " + std::to_string(analogStrength));

            // Keyboard is used - analog strength is 181
            if (analogStrength == 181) {
				api->Log("Using keyboard controls (Limbus Mode)");
                api->AddressSetInt(CONTROL_SCHEME_1, CTRL_TYPE_2);
                api->AddressSetInt(CONTROL_SCHEME_2, CTRL_TYPE_2);
            }

            // Analog stick is being used
            else {
				api->Log("Using analog controls (Limbus Mode)");
                api->AddressSetInt(CONTROL_SCHEME_1, CTRL_TYPE_1);
                api->AddressSetInt(CONTROL_SCHEME_2, CTRL_TYPE_1);
            }

            api->Log("Control scheme is: " + std::to_string(api->AddressGetInt(CONTROL_SCHEME_1)) + " / " + std::to_string(api->AddressGetInt(CONTROL_SCHEME_2)));

        }
    }

    // Flip pressed
    if (type1flip) {

        if (!type1flipIsFlipping) {
            if (inputs.flip && !prevInputs.flip) {
                if (controlScheme == CTRL_TYPE_1) {
                    api->AddressSetInt(CONTROL_SCHEME_1, CTRL_TYPE_2);
                    api->AddressSetInt(CONTROL_SCHEME_2, CTRL_TYPE_2);
                    type1flipIsFlipping = true;
                    type1flipTimer = PHYSICS_FPS;
                }
            }
        }
        else {
            if (--type1flipTimer <= 0) {
                api->AddressSetInt(CONTROL_SCHEME_1, CTRL_TYPE_1);
                api->AddressSetInt(CONTROL_SCHEME_2, CTRL_TYPE_1);
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

        api->HookFunction(hookAddr, hookLength, &PhysicsStep);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
