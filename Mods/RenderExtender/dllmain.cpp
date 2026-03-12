#include "ModApi.h"
#include <windows.h>
#include <iostream>
#include <sstream>

ModApi* api = nullptr;

uintptr_t addr_fog_distance, addr_render_distance;

uintptr_t addr_base_fog_distance, addr_base_render_distance;
int base_fog_distance, base_render_distance;

bool is_active;
#define RENDER_MULTIPLIER_DEFAULT 3
#define RENDER_MULTIPLER_LIMIT 10
int render_multiplier = RENDER_MULTIPLIER_DEFAULT;

void RenderApply() {

    int multiplier = is_active ? render_multiplier : 1;
    int fogOffset = base_render_distance - base_fog_distance;
    api->AddressSetInt(addr_fog_distance, base_render_distance * multiplier - fogOffset);
    api->AddressSetInt(addr_render_distance, base_render_distance * multiplier);

}

void __stdcall toggleExtender() {
    is_active = !is_active;
    api->WriteIniInt(L"Config", L"Enabled", is_active);
    RenderApply();

    if (is_active) {
        api->LogInfo("Render distance extended!");
        std::string toastMessage = "Render distance extended! (x" + std::to_string(render_multiplier) + ")";
        api->ShowInfoToast(toastMessage.c_str());
    }
    else {
        api->LogInfo("Render distance reverted!");
        api->ShowInfoToast("Render distance reverted!");
    }
}

void __stdcall decreaseExtender() {
    if (is_active && render_multiplier > 1) {
        render_multiplier--;
        RenderApply();
        api->WriteIniInt(L"Config", L"RenderMultiplier", render_multiplier);
        std::string logMessage = "Render distance decreased to: x" + std::to_string(render_multiplier);
        std::string toastMessage = "Render distance: x" + std::to_string(render_multiplier);
        api->LogInfo(logMessage.c_str());
        api->ShowInfoToast(toastMessage.c_str());
    }
}

void __stdcall increaseExtender() {
    if (is_active && render_multiplier < RENDER_MULTIPLER_LIMIT) {
        render_multiplier++;
        RenderApply();
        api->WriteIniInt(L"Config", L"RenderMultiplier", render_multiplier);
        std::string logMessage = "Render distance increased to: x" + std::to_string(render_multiplier);
        std::string toastMessage = "Render distance: x" + std::to_string(render_multiplier);
        api->LogInfo(logMessage.c_str());
        api->ShowInfoToast(toastMessage.c_str());
    }
}

static DWORD WINAPI HotkeyThread(LPVOID) {
    DWORD pid = GetCurrentProcessId();

    bool prevF1 = false;
    bool prevMinus = false;
    bool prevPlus = false;

    while (true) {
        DWORD foregroundPid = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
        bool isForeground = pid == foregroundPid;

        if (isForeground) {

            // Toggle extender
            bool currF1 = GetAsyncKeyState(VK_F1) & 0x8000;
            if (currF1 && !prevF1) toggleExtender();
            prevF1 = currF1;

            // Decrease distance
            bool currMinus = GetAsyncKeyState(VK_OEM_MINUS) & 0x8000;
            if (currMinus && !prevMinus) decreaseExtender();
            prevMinus = currMinus;

            // Increase distance
            bool currPlus = GetAsyncKeyState(VK_OEM_PLUS) & 0x8000;
            if (currPlus && !prevPlus) increaseExtender();
            prevPlus = currPlus;
        }

        Sleep(20);
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

    std::string logMessage = "Render distance changed to: " + std::to_string(base_render_distance);
    api->LogDebug(logMessage.c_str());

    // Reapply change
    RenderApply();
}


MenuActionRegistration __stdcall toggleExtenderRegistration() {
	return { is_active ? "Disable Extender" : "Enable Extender", is_active ? "Disable the extender." : "Enable the extender.", toggleExtender, true };
}

MenuActionRegistration __stdcall decreaseExtenderRegistration() {
    static std::string tooltip;
    tooltip = "Decrease render distance (currently: x" + std::to_string(render_multiplier) + ")";
    return { "Decrease Distance", tooltip.c_str(), decreaseExtender, is_active && render_multiplier > 1};
}

MenuActionRegistration __stdcall increaseExtenderRegistration() {
    static std::string tooltip;
    tooltip = "Increase render distance (currently: x" + std::to_string(render_multiplier) + ")";
    return { "Increate Distance", tooltip.c_str(), increaseExtender, is_active && render_multiplier < RENDER_MULTIPLER_LIMIT };
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadModApi();
        if (!api) return FALSE;



        addr_fog_distance = api->ResolveAddress(ADDR_FOG_DISTANCE);
        addr_render_distance = api->ResolveAddress(ADDR_RENDER_DISTANCE);

        // Hook function
        api->HookFunction(hookAddr, hookLength, &OnDistancesWritten);


        is_active = api->SetupIniBool(L"Config", L"Enabled", true);
        render_multiplier = api->SetupIniInt(L"Config", L"RenderMultiplier", RENDER_MULTIPLIER_DEFAULT);

        // Register menu actions
        api->RegisterMenuAction(hModule, toggleExtenderRegistration);
        api->RegisterMenuAction(hModule, decreaseExtenderRegistration);
        api->RegisterMenuAction(hModule, increaseExtenderRegistration);

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, HotkeyThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
