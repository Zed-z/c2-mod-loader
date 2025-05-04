#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>

ModApi* api = nullptr;

int addr_fog_distance = 0x4B7B48;
int original_fog_distance = 0;

int addr_render_distance = 0x4B7B18;
int original_render_distance = 0;

bool is_active = false;

#define MAX_DISTANCE 0x7FFFFFFF

void RenderExtend() {

    original_fog_distance = api->GetAddress(addr_fog_distance);
    original_render_distance = api->GetAddress(addr_render_distance);

    api->SetAddress(addr_fog_distance, MAX_DISTANCE);
    api->SetAddress(addr_render_distance, MAX_DISTANCE);

    is_active = true;
}

void RenderRevert() {
    api->SetAddress(addr_fog_distance, original_fog_distance);
    api->SetAddress(addr_render_distance, original_render_distance);

    is_active = false;
}

DWORD WINAPI PatchThread(LPVOID) {
    while (true) {

        // Render distance changed failsafe
        if (is_active && api->GetAddress(addr_render_distance) != MAX_DISTANCE) {
            RenderExtend();
            api->Log("Render distance reapplied!");
        }

        // Toggle
        if (GetAsyncKeyState(VK_F1) & 1) {

            // Activate
            if (!is_active) {
                RenderExtend();
                api->Log("Render distance extended!");
            }

            // Deactivate
            else {
                RenderRevert();
                api->Log("Render distance reverted!");
            }
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
