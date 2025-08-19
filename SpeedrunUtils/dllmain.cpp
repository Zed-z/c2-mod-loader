#include "ModApi.h"
#include <Windows.h>
#include <iostream>

ModApi* api = nullptr;

void __stdcall resetBossFlags() {
    api->AddressSetInt(0x622D50, 0);
    api->ShowInfoToast("Reset boss flags successfully.");
}

MenuActionRegistration __stdcall resetBossFlagsRegistration() {
    return { "Reset boss flags", "Reset boss warp flags.", resetBossFlags, true};
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadModApi();
        if (!api) return FALSE;

		// Register menu action
        api->RegisterMenuAction(hModule, resetBossFlagsRegistration);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
