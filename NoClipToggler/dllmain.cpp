#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>

ModApi* api = nullptr;

const uintptr_t patchAddress = 0x00489BE5;
const BYTE originalBytes[5] = { 0xE8, 0x26, 0xA5, 0xF7, 0xFF };
const BYTE patchBytes[5] = { 0x90, 0x90, 0x90, 0x90, 0x90 };

bool noclip_enabled = false;

DWORD WINAPI PatchThread(LPVOID) {
    while (true) {

        // Toggle
        if (GetAsyncKeyState(VK_F2) & 1) {
            noclip_enabled = !noclip_enabled;

            if (noclip_enabled) {
                api->PatchBytes(patchAddress, patchBytes, sizeof(patchBytes));
                api->Log("Noclip enabled!");
            }
            else {
                api->PatchBytes(patchAddress, originalBytes, sizeof(originalBytes));
                api->Log("Noclip disabled!");
            }
        }

        // Go up and down
        if (noclip_enabled) {
            if (GetAsyncKeyState(VK_PRIOR)) {// Page up
                auto addr = api->ResolveAddress(ADDR_CROC_POS_Y);
                api->AddressSetInt(
                    addr,
                    api->AddressGetInt(addr) + 100
                );
            }

            if (GetAsyncKeyState(VK_NEXT) & 1) {// Page down
                auto addr = api->ResolveAddress(ADDR_CROC_POS_Y);
                api->AddressSetInt(
                    addr,
                    api->AddressGetInt(addr) - 100
                );
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
