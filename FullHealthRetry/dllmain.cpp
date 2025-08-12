#include "ModApi.h"
#include <Windows.h>
#include <iostream>

ModApi* api = nullptr;

bool fullHealthOnRetry = true;
bool fullHealthOnLevelEntry = true;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadModApi();
        if (!api) return FALSE;

        fullHealthOnRetry = api->ReadIniInt(L"Config", L"FullHealthOnRetry", true);
        api->WriteIniInt(L"Config", L"FullHealthOnRetry", fullHealthOnRetry);

        fullHealthOnLevelEntry = api->ReadIniInt(L"Config", L"FullHealthOnLevelEntry", true);
        api->WriteIniInt(L"Config", L"FullHealthOnLevelEntry", fullHealthOnLevelEntry);

        // Instead of setting current lives to 5 on game over,
        // Get the live limit (Croc2.exe+2062CC) of the current slot (ecx)
        // And set the lives to that value instead
        if (fullHealthOnRetry) {
            BYTE injectedCode[] = {
                0x50,                                     // push eax
                0x8B, 0x81, 0xCC, 0x42, 0x60, 0x00,       // mov eax, [ecx+Croc2.exe+2062CC]
                0x89, 0x81, 0xD0, 0x42, 0x60, 0x00,       // mov [ecx+Croc2.exe+2042D0], eax
                0x58,                                     // pop eax
            };
            api->InjectCode(0x407EF7, 10, injectedCode, sizeof(injectedCode), INJECT_REPLACE);
        }

        // On level entry,
        // Get the live limit (Croc2.exe+2062CC) of the current slot (Croc2.exe+2220FC * 2000)
        // And set the lives to that value
        if (fullHealthOnLevelEntry) {
            BYTE injectedCode[] = {
                0x50,                                     // push eax
                0x51,                                     // push ecx
                0x8B, 0x0D, 0xFC, 0x20, 0x62, 0x00,       // mov ecx, [Croc2.exe+2220FC]
                0x69, 0xC9, 0x00, 0x20, 0x00, 0x00,       // imul ecx, ecx, 00002000
                0x8B, 0x81, 0xCC, 0x42, 0x60, 0x00,       // mov eax, [ecx+Croc2.exe+2042CC]
                0x89, 0x81, 0xD0, 0x42, 0x60, 0x00,       // mov [ecx+Croc2.exe+2042D0], eax
                0x59,                                     // pop ecx
                0x58,                                     // pop eax
            };
            api->InjectCode(0x418DB5, 6, injectedCode, sizeof(injectedCode), INJECT_BEFORE);
        }
    }
    return TRUE;
}
