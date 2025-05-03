#include "ModApi.h"
#include <Windows.h>
#include <iostream>

ModApi* api = nullptr;

void PatchGameValue(int address, int value, ModApi* api) {
    DWORD* addr = reinterpret_cast<DWORD*>(address);
    if (!IsBadWritePtr(addr, sizeof(DWORD))) {
        *addr = value;
        api->Log("Memory at 0x004B7B48 patched to 999999");
    }
    else {
        api->Log("Failed to write to 0x004B7B48 (invalid pointer)");
    }
}

DWORD WINAPI PatchThread(LPVOID) {
    while (true) {
        PatchGameValue(0x4B7B48, 0x7FFFFFFF, api);
        PatchGameValue(0x4B7B18, 0x7FFFFFFF, api);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;
        
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, PatchThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
