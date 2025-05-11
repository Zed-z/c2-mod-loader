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






#include <cstdint>
#define JMP_SIZE 5

// Allows you to put arbitrary C++ code
bool HookFunction(uintptr_t target, size_t length, void(__stdcall* func)()) {

    // Target needs to be able to fit a jump
    if (length < JMP_SIZE) return false;


    // Allocate memory for injection
    // Trampoline: an injected bit of code made just to call a function
    // original instructions + function call + jmp back
    size_t trampoline_size = length + JMP_SIZE + JMP_SIZE;
    BYTE* trampoline = (BYTE*)VirtualAlloc(
        nullptr, trampoline_size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    if (!trampoline) return false;


    // Copy original instructions
    memcpy(trampoline, (void*)target, length);


    // Append function call 
    BYTE* p = trampoline + length;
    p[0] = 0xE8; // CALL <4 byte function address - relative>

    int32_t relative_function = (int32_t)(
        (uintptr_t)func - ((uintptr_t)p + JMP_SIZE)
    );

    memcpy(p + 1, &relative_function, 4);
    p += JMP_SIZE;


    // Append JMP back to target + length
    p[0] = 0xE9; // CALL <4 byte function address - relative>

    int32_t relative_jumpback = (int32_t)(
        (int64_t)(target + length) - ((uintptr_t)p + JMP_SIZE)
    );

    memcpy(p + 1, &relative_jumpback, 4);


    // Prepare patch
    BYTE patch[16];
    memset(patch, 0x90, length); // NOPs

    patch[0] = 0xE9;
    int32_t relative_trampoline = (int32_t)(
        (uintptr_t)trampoline - (target + JMP_SIZE)
    );
    memcpy(patch + 1, &relative_trampoline, 4);

    
    // Apply patch
    DWORD old;
    VirtualProtect((void*)target, length, PAGE_EXECUTE_READWRITE, &old);
    memcpy((void*)target, patch, length);
    VirtualProtect((void*)target, length, old, &old);

    return true;
}








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


        /*
        Croc2.exe+23D04 - 89 15 487B4B00        - mov [Croc2.exe+B7B48],edx { (16384) }
        Croc2.exe+23D0A - 8B 40 40              - mov eax,[eax+40]
        Croc2.exe+23D0D - A3 187B4B00           - mov [Croc2.exe+B7B18],eax { (28672) }
        */
        uintptr_t hookAddr = 0x00423D0D;
        size_t hookLength = 5;

        HookFunction(hookAddr, hookLength, &OnDistancesWritten);


        is_active = api->ReadIniInt(L"Config", L"Enabled", 1);
        api->WriteIniInt(L"Config", L"Enabled", is_active);

        render_multiplier = api->ReadIniInt(L"Config", L"RenderMultiplier", RENDER_MULTIPLIER_DEFAULT);
        api->WriteIniInt(L"Config", L"RenderMultiplier", render_multiplier);

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, PatchThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
