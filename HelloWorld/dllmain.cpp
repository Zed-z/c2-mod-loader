#include "ModApi.h"
#include <Windows.h>
#include <iostream>


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        auto api = LoadSharedModApi();
        if (!api) return FALSE;
        api->Log("Hello world!");
    }
    return TRUE;
}
