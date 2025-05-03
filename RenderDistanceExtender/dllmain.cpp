#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>

ModApi* api = nullptr;

DWORD* addr_fog_distance = reinterpret_cast<DWORD*>(0x4B7B48);
int original_fog_distance = 0;

DWORD* addr_render_distance = reinterpret_cast<DWORD*>(0x4B7B18);
int original_render_distance = 0;

bool is_active = false;

int GetGameValue(DWORD* address, ModApi* api) {
    if (!IsBadWritePtr(address, sizeof(DWORD))) {

        std::ostringstream stream;
        stream << "Memory at " << address << " is " << *address;
        api->Log(stream.str());

        return *address;
    }
    else {

        std::ostringstream stream;
        stream << "Failed to read " << address << " (invalid pointer)";
        api->Log(stream.str());

        return -1;
    }
}

void PatchGameValue(DWORD* address, int value, ModApi* api) {
    if (!IsBadWritePtr(address, sizeof(DWORD))) {
        *address = value;

        std::ostringstream stream;
        stream << "Memory at " << address << " patched to " << value;
        api->Log(stream.str());
    }
    else {

        std::ostringstream stream;
        stream << "Failed to write to " << address << " (invalid pointer)";
        api->Log(stream.str());
    }
}

DWORD WINAPI PatchThread(LPVOID) {
    while (true) {
        if (GetAsyncKeyState(VK_F1) & 1) {

            // Activate
            if (!is_active) {
                original_fog_distance = GetGameValue(addr_fog_distance, api);
                original_render_distance = GetGameValue(addr_render_distance, api);

                PatchGameValue(addr_fog_distance, 0x7FFFFFFF, api);
                PatchGameValue(addr_render_distance, 0x7FFFFFFF, api);
            }

            // Deactivate
            else {
                PatchGameValue(addr_fog_distance, original_fog_distance, api);
                PatchGameValue(addr_render_distance, original_render_distance, api);
            }

            is_active = !is_active;
        }
        Sleep(50);
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
