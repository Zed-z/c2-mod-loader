#include "ModApi.h"
#include <Windows.h>
#include <iostream>


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        auto api = LoadSharedModApi();
        if (!api) return FALSE;

        // JIceCave1 
        const uint8_t pattern1[] = { 'J','I','C','E','C','A','V','E', 0 };
        const uint8_t replacement1[] = { 'J','I','C','E','C','A','V','E','1' };

        uintptr_t match1 = api->FindPattern(pattern1, sizeof(pattern1), 1);
        if (!match1) {
            api->Log("JIceCave1 pattern not found.");
            return FALSE;
        }

        if (api->PatchBytes(match1, replacement1, sizeof(replacement1))) {
            api->Log("JIceCave1 patch applied.");
        }
        else {
            api->Log("JIceCave1 patch failed.");
            return FALSE;
        }

        // JHub1
        const uint8_t pattern2[] = { 'J', 'H', 'U', 'B', '1', 0 };
        const uint8_t replacement2[] = { 'J', 'H', 'U', 'B', '3', '4' };

        uintptr_t match2 = api->FindPattern(pattern2, sizeof(pattern2), 1);
        if (!match2) {
            api->Log("JHub1 pattern not found.");
            return FALSE;
        }

        if (api->PatchBytes(match2, replacement2, sizeof(replacement2))) {
            api->Log("JHub1 patch applied.");
        }
        else {
            api->Log("JHub1 patch failed.");
            return FALSE;
        }

        // JHub2
        const uint8_t pattern3[] = { 'J', 'H', 'U', 'B', '2', 0 };
        const uint8_t replacement3[] = { 'J', 'H', 'U', 'B', '3', '4' };

        uintptr_t match3 = api->FindPattern(pattern3, sizeof(pattern3), 1);
        if (!match3) {
            api->Log("JHub2 pattern not found.");
            return FALSE;
        }

        if (api->PatchBytes(match3, replacement3, sizeof(replacement3))) {
            api->Log("JHub2 patch applied.");
        }
        else {
            api->Log("JHub2 patch failed.");
            return FALSE;
        }

    }
    return TRUE;
}
