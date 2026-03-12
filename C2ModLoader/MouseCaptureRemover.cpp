#include "ModApi.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include "MinHook.h"

extern ModApi* api;

bool freeMouse;

typedef HCURSOR(WINAPI* SetCursorFunc)(HCURSOR);
SetCursorFunc oSetCursor = nullptr;

HCURSOR WINAPI hkSetCursor(HCURSOR hCursor) {
    return hCursor;
}

typedef int (WINAPI* ShowCursorFunc)(BOOL);
ShowCursorFunc oShowCursor = nullptr;

int WINAPI hkShowCursor(BOOL bShow) {
    // Initialize fake cursor display counter
    static int fakeCursorCount = 1;

    // Simulate the internal counter behavior
    if (bShow) {
        fakeCursorCount++;
    }
    else {
        fakeCursorCount--;
    }

    return fakeCursorCount;
}

typedef BOOL(WINAPI* SetCursorPosFunc)(int, int);
SetCursorPosFunc oSetCursorPos = nullptr;

BOOL WINAPI hkSetCursorPos(int X, int Y) {
    return TRUE;
}

typedef BOOL(WINAPI* ClipCursorFunc)(const RECT*);
ClipCursorFunc oClipCursor = nullptr;

BOOL WINAPI hkClipCursor(const RECT* lpRect) {
    return TRUE;
}

typedef HWND(WINAPI* SetCaptureFunc)(HWND);
SetCaptureFunc oSetCapture = nullptr;

HWND WINAPI hkSetCapture(HWND hWnd) {
    return NULL;
}

typedef BOOL(WINAPI* ReleaseCaptureFunc)();
ReleaseCaptureFunc oReleaseCapture = nullptr;

BOOL WINAPI hkReleaseCapture() {
    return TRUE;
}

DWORD WINAPI InitHooks(LPVOID lpParam) {
    MH_Initialize();

    MH_CreateHook((void*)&SetCursor, (void*)&hkSetCursor, (void**)&oSetCursor);
    MH_CreateHook((void*)&ShowCursor, (void*)&hkShowCursor, (void**)&oShowCursor);
    MH_CreateHook((void*)&SetCursorPos, (void*)&hkSetCursorPos, (void**)&oSetCursorPos);
    MH_CreateHook((void*)&ClipCursor, (void*)&hkClipCursor, (void**)&oClipCursor);
    MH_CreateHook((void*)&SetCapture, (void*)&hkSetCapture, (void**)&oSetCapture);
    MH_CreateHook((void*)&ReleaseCapture, (void*)&hkReleaseCapture, (void**)&oReleaseCapture);

    MH_EnableHook(MH_ALL_HOOKS);
    api->LogDebug("Mouse capture API hooks installed!");

    return TRUE;
}

/*
    Adapted from hdc0's Python script
    https://pastebin.com/6xms0KcG
*/
DWORD WINAPI MouseInitThread(LPVOID lpParam) {

    int gameVersion = api->GetGameVersion();


    // Replace calls to "CreateDevice" with "add esp, 10h"
    uintptr_t ADDR_CALL_CREATEDEVICE_MOUSE{};
    switch (gameVersion) {
    case GAMEVER_US:
        ADDR_CALL_CREATEDEVICE_MOUSE = 0x400000 + 0x11482;
        break;
    case GAMEVER_EU:
        ADDR_CALL_CREATEDEVICE_MOUSE = 0x400000 + 0x11562;
        break;
    }
    if (gameVersion != GAMEVER_UNKNOWN) {
        BYTE patch1[] = { 0x83, 0xC4, 0x10 };
        api->PatchBytes(ADDR_CALL_CREATEDEVICE_MOUSE, patch1, sizeof(patch1));
        api->LogDebug("Patched CreateDevice!");
    }

    // Replace calls to "SetCursorPos" with "add esp, 8" and 3 nop bytes
    uintptr_t ADDR_CALL_SETCURSORPOS{};
    switch (gameVersion) {
    case GAMEVER_US:
        ADDR_CALL_SETCURSORPOS = 0x400000 + 0x2164A;
        break;
    case GAMEVER_EU:
        ADDR_CALL_SETCURSORPOS = 0x400000 + 0x21CE4;
        break;
    }
    if (gameVersion != GAMEVER_UNKNOWN) {
        BYTE patch2[] = { 0x83, 0xC4, 0x08, 0x90, 0x90, 0x90 };
        api->PatchBytes(ADDR_CALL_SETCURSORPOS, patch2, sizeof(patch2));
        api->LogDebug("Patched SetCursorPos!");
    }

    // Replace calls to "SetCursor" with "pop eax" and 5 nop bytes
    uintptr_t ADDRS_CALL_SETCURSOR[3] = {};
    switch (gameVersion) {
    case GAMEVER_US:
        ADDRS_CALL_SETCURSOR[0] = 0x400000 + 0x096C9;
        ADDRS_CALL_SETCURSOR[1] = 0x400000 + 0x24BFD;
        ADDRS_CALL_SETCURSOR[2] = 0x400000 + 0x24CA7;
        break;
    case GAMEVER_EU:
        ADDRS_CALL_SETCURSOR[0] = 0x400000 + 0x098F9;
        ADDRS_CALL_SETCURSOR[1] = 0x400000 + 0x2529D;
        ADDRS_CALL_SETCURSOR[2] = 0x400000 + 0x25347;
        break;
    }
    if (gameVersion != GAMEVER_UNKNOWN) {
        BYTE patch3[] = { 0x58, 0x90, 0x90, 0x90, 0x90, 0x90 };
        for (int i = 0; i < (sizeof(ADDRS_CALL_SETCURSOR) / sizeof(uintptr_t)); i++) {
            api->PatchBytes(ADDRS_CALL_SETCURSOR[i], patch3, sizeof(patch3));
        }
        api->LogDebug("Patched SetCursor!");
    }

    // Replace calls to "ShowCursor" with "pop eax" and 1 nop byte
    uintptr_t ADDRS_CALL_SHOWCURSOR[4] = {};
    switch (gameVersion) {
    case GAMEVER_US:
        ADDRS_CALL_SHOWCURSOR[0] = 0x400000 + 0x09608;
        ADDRS_CALL_SHOWCURSOR[1] = 0x400000 + 0x09657;
        ADDRS_CALL_SHOWCURSOR[2] = 0x400000 + 0x096B9;
        ADDRS_CALL_SHOWCURSOR[3] = 0x400000 + 0x21351;
        break;
    case GAMEVER_EU:
        ADDRS_CALL_SHOWCURSOR[0] = 0x400000 + 0x09838;
        ADDRS_CALL_SHOWCURSOR[1] = 0x400000 + 0x09887;
        ADDRS_CALL_SHOWCURSOR[2] = 0x400000 + 0x098E9;
        ADDRS_CALL_SHOWCURSOR[3] = 0x400000 + 0x219C1;
        break;
    }
    if (gameVersion != GAMEVER_UNKNOWN) {
        BYTE patch4[] = { 0x58, 0x90 };
        for (int i = 0; i < (sizeof(ADDRS_CALL_SHOWCURSOR) / sizeof(uintptr_t)); i++) {
            api->PatchBytes(ADDRS_CALL_SHOWCURSOR[i], patch4, sizeof(patch4));
        }
        api->LogDebug("Patched ShowCursor!");
    }


    CreateThread(nullptr, 0, InitHooks, nullptr, 0, nullptr);
    return TRUE;
}
