#include "ModApi.h"
#include <Windows.h>
#include <iostream>

ModApi* api = nullptr;

void __stdcall helloWorld() {
    api->ShowToast("Hello world!");
}

DWORD WINAPI ModInitThread(LPVOID) {
    api->RegisterMenuAction("Hello World", helloWorld);
    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;

        api->Log("Hello world!");

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, ModInitThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
