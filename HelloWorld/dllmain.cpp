#include "ModApi.h"
#include <Windows.h>
#include <iostream>

ModApi* api = nullptr;

void __stdcall helloWorld() {
    api->ShowToast("Hello world!");
}

MenuActionRegistration __stdcall helloWorldRegistration() {
    return { "Hello World", "Show a toast notification.", helloWorld, true};
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadModApi();
        if (!api) return FALSE;

        api->LogInfo("Hello world!");

		// Register menu action
        api->RegisterMenuAction(hModule, helloWorldRegistration);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
