#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>

ModApi* api = nullptr;

uintptr_t addr_fog_distance, addr_render_distance;

uintptr_t addr_base_fog_distance, addr_base_render_distance;
int base_fog_distance, base_render_distance;

bool is_active = false;
#define RENDER_MULTIPLIER_DEFAULT 3
#define RENDER_MULTIPLER_LIMIT 10
int render_multiplier = RENDER_MULTIPLIER_DEFAULT;

void RenderApply() {

    int multiplier = is_active ? render_multiplier : 1;
    int fogOffset = base_render_distance - base_fog_distance;
    api->AddressSetInt(addr_fog_distance, base_render_distance * multiplier - fogOffset);
    api->AddressSetInt(addr_render_distance, base_render_distance * multiplier);
    
}

DWORD WINAPI PatchThread(LPVOID) {
    while (true) {

        bool toggleKey = GetAsyncKeyState(VK_F1) & 1;
        bool minusHeld = GetAsyncKeyState(VK_OEM_MINUS) & 1;
        bool plusHeld = GetAsyncKeyState(VK_OEM_PLUS) & 1;

        // Change render distance
        if (is_active) {
            if (minusHeld && render_multiplier > 1) {
                render_multiplier--;
                RenderApply();
                api->WriteIniInt(L"Config", L"RenderMultiplier", render_multiplier);
                api->Log("Render distance decreased to: x" + std::to_string(render_multiplier));
            }
            if (plusHeld && render_multiplier < RENDER_MULTIPLER_LIMIT) {
                render_multiplier++;
                RenderApply();
                api->WriteIniInt(L"Config", L"RenderMultiplier", render_multiplier);
                api->Log("Render distance increased to: x" + std::to_string(render_multiplier));
            }
        }

        // Toggle
        if (toggleKey) {

            is_active = !is_active;
            api->WriteIniInt(L"Config", L"Enabled", is_active);
            RenderApply();

            if (is_active) {
                api->Log("Render distance extended!");
            }
            else {
                api->Log("Render distance reverted!");
            }
        }

        Sleep(10);
    }
    return 0;
}


/*
Croc2.exe+23D04 - 89 15 487B4B00        - mov [Croc2.exe+B7B48],edx { (16384) }
Croc2.exe+23D0A - 8B 40 40              - mov eax,[eax+40]
Croc2.exe+23D0D - A3 187B4B00           - mov [Croc2.exe+B7B18],eax { (28672) }
*/
uintptr_t hookAddr = 0x00423D0D;
size_t hookLength = 5;

void __stdcall OnDistancesWritten() {
    // Read both values out of memory
    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    base_fog_distance = *(int*)(base + 0xB7B48);
    base_render_distance = *(int*)(base + 0xB7B18);

    api->Log("Render distance changed!");

    // Reapply change
    RenderApply();
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;



        addr_fog_distance = api->ResolveAddress(ADDR_FOG_DISTANCE);
        addr_render_distance = api->ResolveAddress(ADDR_RENDER_DISTANCE);

        // Hook function
        api->HookFunction(hookAddr, hookLength, &OnDistancesWritten);


        is_active = api->ReadIniInt(L"Config", L"Enabled", 1);
        api->WriteIniInt(L"Config", L"Enabled", is_active);

        render_multiplier = api->ReadIniInt(L"Config", L"RenderMultiplier", RENDER_MULTIPLIER_DEFAULT);
        api->WriteIniInt(L"Config", L"RenderMultiplier", render_multiplier);

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, PatchThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
