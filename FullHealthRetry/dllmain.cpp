#include "ModApi.h"
#include <Windows.h>
#include <iostream>

ModApi* api = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;

        // Instead of setting current lives to 5 on game over
        // Get the live limit (Croc2.exe+2062CC) of the current slot (ecx)
        // And set the lives to that instead
        BYTE injectedCode[] = {
            0x50,                                     // push eax
            0x8B, 0x81, 0xCC, 0x42, 0x60, 0x00,       // mov eax, [ecx+Croc2.exe+2062CC]
            0x89, 0x81, 0xD0, 0x42, 0x60, 0x00,       // mov [ecx+Croc2.exe+2042D0], eax
            0x58,                                     // pop eax
        };
        api->InjectCode(0x407EF7, 10, injectedCode, sizeof(injectedCode));
    }
    return TRUE;
}
