#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>


static bool FileExists(const std::wstring& path) {
    std::ifstream f(path);
    return f.good();
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        auto api = LoadModApi();
        if (!api) return FALSE;


        WCHAR exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring exePathStr{ exePath };
        size_t pos = exePathStr.find_last_of(L"\\/");
        exePathStr = exePathStr.substr(0, pos + 1);


        if (!FileExists(exePathStr + L"modfiles\\music\\JIceCave1.asf")) {
            api->LogError("<game_dir>\\modfiles\\music\\JIceCave1.asf not found!");
            api->ShowErrorToast("JIceCave1.asf not found!");
        }
        else {

            // JIceCave1 
            const uint8_t pattern1[] = { 'J','I','C','E','C','A','V','E', 0 };
            const uint8_t replacement1[] = { 'J','I','C','E','C','A','V','E','1' };

            uintptr_t match1 = api->FindPattern(pattern1, sizeof(pattern1), 1);
            if (!match1) {
                api->LogError("JIceCave pattern not found.");
                return FALSE;
            }

            if (api->PatchBytes(match1, replacement1, sizeof(replacement1))) {
                api->LogDebug("JIceCave patch applied.");
            }
            else {
                api->LogError("JIceCave patch failed.");
                return FALSE;
            }
        }

        
        if (!FileExists(exePathStr + L"modfiles\\music\\JHub34.asf")) {
            api->LogError("<game_dir>\\modfiles\\music\\JHub34.asf not found!");
            api->ShowErrorToast("JHub34.asf not found!");
        }
        else {

            // JHub1
            const uint8_t pattern2[] = { 'J', 'H', 'U', 'B', '1', 0 };
            const uint8_t replacement2[] = { 'J', 'H', 'U', 'B', '3', '4' };

            uintptr_t match2 = api->FindPattern(pattern2, sizeof(pattern2), 1);
            if (!match2) {
                api->LogError("JHub1 pattern not found.");
                return FALSE;
            }

            if (api->PatchBytes(match2, replacement2, sizeof(replacement2))) {
                api->LogDebug("JHub1 patch applied.");
            }
            else {
                api->LogError("JHub1 patch failed.");
                return FALSE;
            }

            // JHub2
            const uint8_t pattern3[] = { 'J', 'H', 'U', 'B', '2', 0 };
            const uint8_t replacement3[] = { 'J', 'H', 'U', 'B', '3', '4' };

            uintptr_t match3 = api->FindPattern(pattern3, sizeof(pattern3), 1);
            if (!match3) {
                api->LogError("JHub2 pattern not found.");
                return FALSE;
            }

            if (api->PatchBytes(match3, replacement3, sizeof(replacement3))) {
                api->LogDebug("JHub2 patch applied.");
            }
            else {
                api->LogError("JHub2 patch failed.");
                return FALSE;
            }
        }
    }
    return TRUE;
}
