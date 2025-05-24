#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>

ModApi* api = nullptr;

#define INPUTS 0x52A590

// 1 - TYPE 1, 0 - TYPE 2
#define CTRL_TYPE_1 1
#define CTRL_TYPE_2 0
#define CONTROL_SCHEME_1 0x52A5F4
#define CONTROL_SCHEME_2 0x60438C

int inputs, prevInputs;

static DWORD WINAPI InputThread(LPVOID) {
    while (true) {

        bool changeControls = GetAsyncKeyState(VK_CAPITAL) & 1;

        prevInputs = inputs;
        inputs = api->AddressGetInt(INPUTS);

        bool controlScheme = (bool)(api->AddressGetInt(CONTROL_SCHEME_1));

        if (changeControls) {
            api->AddressSetInt(CONTROL_SCHEME_1, !controlScheme);
            api->AddressSetInt(CONTROL_SCHEME_2, !controlScheme);
        }

        bool prevFlip = prevInputs & 8192;
        bool flip = inputs & 8192;

        // Flip pressed
        if (flip && !prevFlip) {
            if (controlScheme == CTRL_TYPE_1) {
                api->AddressSetInt(CONTROL_SCHEME_1, CTRL_TYPE_2);
                api->AddressSetInt(CONTROL_SCHEME_2, CTRL_TYPE_2);

                Sleep(1000);

                api->AddressSetInt(CONTROL_SCHEME_1, CTRL_TYPE_1);
                api->AddressSetInt(CONTROL_SCHEME_2, CTRL_TYPE_1);
            }
        }

        Sleep(10);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, InputThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
