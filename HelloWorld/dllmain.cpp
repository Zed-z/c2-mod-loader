#include "ModApi.h"
#include <Windows.h>
#include <iostream>

ModApi* api = nullptr;

void __stdcall helloWorld() {
    api->ShowToast("Hello world!");
}

MenuActionRegistration __stdcall helloWorldRegistration() {
    return { "Hello World", helloWorld, true };
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;

        api->Log("Hello world!");

		// Register menu action
        api->RegisterMenuAction(hModule, helloWorldRegistration);

        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
