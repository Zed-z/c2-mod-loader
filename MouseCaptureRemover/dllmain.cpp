#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include "MinHook.h"

ModApi* api = nullptr;
bool logHooks = false;

typedef HCURSOR(WINAPI* SetCursorFunc)(HCURSOR);
SetCursorFunc oSetCursor = nullptr;

HCURSOR WINAPI hkSetCursor(HCURSOR hCursor) {
    if (logHooks) api->Log("hkSetCursor");
    return hCursor;
}

typedef int (WINAPI* ShowCursorFunc)(BOOL);
ShowCursorFunc oShowCursor = nullptr;

int WINAPI hkShowCursor(BOOL bShow) {
    // Initialize fake cursor display counter
    static int fakeCursorCount = 1;

    if (logHooks) api->Log("hkShowCursor");

    // Simulate the internal counter behavior
    if (bShow) {
        ++fakeCursorCount;
    }
    else {
        --fakeCursorCount;
    }

    return fakeCursorCount;
}

typedef BOOL(WINAPI* SetCursorPosFunc)(int, int);
SetCursorPosFunc oSetCursorPos = nullptr;

BOOL WINAPI hkSetCursorPos(int X, int Y) {
    if (logHooks) api->Log("hkSetCursorPos");
    return TRUE;
}

typedef BOOL(WINAPI* ClipCursorFunc)(const RECT*);
ClipCursorFunc oClipCursor = nullptr;

BOOL WINAPI hkClipCursor(const RECT* lpRect) {
    if (logHooks) api->Log("hkClipCursor");
    return TRUE;
}

typedef HWND(WINAPI* SetCaptureFunc)(HWND);
SetCaptureFunc oSetCapture = nullptr;

HWND WINAPI hkSetCapture(HWND hWnd) {
    if (logHooks) api->Log("hkSetCapture");
    return NULL;
}

typedef BOOL(WINAPI* ReleaseCaptureFunc)();
ReleaseCaptureFunc oReleaseCapture = nullptr;

BOOL WINAPI hkReleaseCapture() {
    if (logHooks) api->Log("hkReleaseCapture");
    return TRUE;
}

void InitHooks() {
    MH_Initialize();
    MH_CreateHook(&SetCursor, &hkSetCursor, (void**)&oSetCursor);
    MH_CreateHook(&ShowCursor, &hkShowCursor, (void**)&oShowCursor);
    MH_CreateHook(&SetCursorPos, &hkSetCursorPos, (void**)&oSetCursorPos);
    MH_CreateHook(&ClipCursor, &hkClipCursor, (void**)&oClipCursor);
    MH_CreateHook(&SetCapture, &hkSetCapture, (void**)&oSetCapture);
    MH_CreateHook(&ReleaseCapture, &hkReleaseCapture, (void**)&oReleaseCapture);
    MH_EnableHook(MH_ALL_HOOKS);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;

        // Config
        logHooks = (bool)(api->ReadIniInt(L"Config", L"LogHooks", false));
        api->WriteIniInt(L"Config", L"LogHooks", (int)logHooks);

        // Release the cursor
        DisableThreadLibraryCalls(hModule);

        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            InitHooks(); // <-- hook APIs here
            api->Log("Mouse capture API hooks installed!");
            return 0;
            }, nullptr, 0, nullptr);
    }
    return TRUE;
}
