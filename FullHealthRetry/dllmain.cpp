#include "ModApi.h"
#include <Windows.h>
#include <iostream>

ModApi* api = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;

        // Slot 1
        BYTE injectedCode[] = {
            0x50,                                     // push eax
            0xA1, 0xCC, 0x62, 0x60, 0x00,             // mov eax, [Croc2.exe+2062CC] (absolute addr)
            0x89, 0x81, 0xD0, 0x42, 0x60, 0x00,       // mov [ecx+Croc2.exe+2042D0], eax
            0x58,                                     // pop eax
        };
        api->InjectCode(0x407EF7, 10, injectedCode, sizeof(injectedCode));
    }
    return TRUE;
}
