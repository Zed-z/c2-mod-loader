#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>

ModApi* api = nullptr;

uintptr_t addr_fog_distance, addr_render_distance;
int original_fog_distance, original_render_distance;

#define MAX_DISTANCE 0x7FFFFFFF

bool is_active = false;
int render_distance = MAX_DISTANCE;


void RenderExtend() {

    original_fog_distance = api->AddressGetInt(addr_fog_distance);
    original_render_distance = api->AddressGetInt(addr_render_distance);

    api->AddressSetInt(addr_fog_distance, render_distance);
    api->AddressSetInt(addr_render_distance, render_distance);

    is_active = true;
    api->WriteIniInt(L"Config", L"Enabled", is_active);
}

void RenderRevert() {

    api->AddressSetInt(addr_fog_distance, original_fog_distance);
    api->AddressSetInt(addr_render_distance, original_render_distance);

    is_active = false;
    api->WriteIniInt(L"Config", L"Enabled", is_active);
}

DWORD WINAPI PatchThread(LPVOID) {
    while (true) {

        // Render distance changed failsafe
        if (is_active && api->AddressGetInt(addr_render_distance) != render_distance) {
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
        
        addr_fog_distance = api->ResolveAddress(ADDR_FOG_DISTANCE);
        addr_render_distance = api->ResolveAddress(ADDR_RENDER_DISTANCE);

        is_active = api->ReadIniInt(L"Config", L"Enabled", 1);
        api->WriteIniInt(L"Config", L"Enabled", is_active);

        render_distance = api->ReadIniInt(L"Config", L"Enabled", MAX_DISTANCE);
        api->WriteIniInt(L"Config", L"Enabled", render_distance);

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, PatchThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
