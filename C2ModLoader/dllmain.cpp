#define IS_MOD_LOADER
#include "ModApi.h"
#include "Config.h"
#include "ImGuiHandler.h"
#include "MouseCaptureRemover.h"
#include "Utils.h"
#include "Resource.h"
#include "Launcher.h"
#include "Loader.h"
#include "CheatsManager.h"

#include <Windows.h>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>

ModApi* api;

bool loaderEnabled = true;
bool skipLauncher = false;
bool guiEnabled = true;

// Keyboard input thread
static DWORD WINAPI HotkeyThread(LPVOID) {
    DWORD pid = GetCurrentProcessId();

    bool prevTab = false;

    while (true) {
        DWORD foregroundPid = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
        bool isForeground = pid == foregroundPid;

        if (isForeground) {
            
            // Toggle GUI
            bool currTab = GetAsyncKeyState(VK_TAB) & 0x8000;
            if (currTab && !prevTab) {
                showGui = !showGui;
                api->WriteIniInt(L"GUI", L"ShowGui", (int)showGui);
            }
            prevTab = currTab;
        }

        Sleep(20);
    }
    return 0;
}

// Entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        api = GetModApi();
        ClearLog();

        // Config
        loaderEnabled = api->ReadIniBool(L"Config", L"LoaderEnabled", true);
        api->WriteIniBool(L"Config", L"LoaderEnabled", loaderEnabled);

        skipLauncher = api->ReadIniBool(L"Config", L"SkipLauncher", false);
        api->WriteIniBool(L"Config", L"SkipLauncher", skipLauncher);

        guiEnabled = api->ReadIniBool(L"GUI", L"GuiEnabled", true);
        api->WriteIniBool(L"GUI", L"GuiEnabled", guiEnabled);

        showGui = api->ReadIniBool(L"GUI", L"ShowGui", false);
        api->WriteIniBool(L"GUI", L"ShowGui", showGui);

        showLog = api->ReadIniBool(L"GUI", L"ShowLog", false);
        api->WriteIniBool(L"GUI", L"ShowLog", showLog);

        showInputs = api->ReadIniBool(L"GUI", L"ShowInputs", false);
        api->WriteIniBool(L"GUI", L"ShowInputs", showInputs);

        showObjectList = api->ReadIniBool(L"GUI", L"ShowObjectList", false);
        api->WriteIniBool(L"GUI", L"ShowObjectList", showObjectList);

        incompatibleWarningShown = api->ReadIniBool(L"GUI", L"IncompatibleWarningShown", false);
        api->WriteIniBool(L"GUI", L"IncompatibleWarningShown", incompatibleWarningShown);

        logInfo = api->ReadIniBool(L"Logging", L"LogInfo", true);
        api->WriteIniBool(L"Logging", L"LogInfo", logInfo);

        logDebug = api->ReadIniBool(L"Logging", L"LogDebug", false);
        api->WriteIniBool(L"Logging", L"LogDebug", logDebug);

        logWarnings = api->ReadIniBool(L"Logging", L"LogWarnings", false);
        api->WriteIniBool(L"Logging", L"LogWarnings", logWarnings);

        logErrors = api->ReadIniBool(L"Logging", L"LogErrors", true);
        api->WriteIniBool(L"Logging", L"LogErrors", logErrors);

        freeMouse = api->ReadIniBool(L"Mouse", L"FreeMouse", true);
        api->WriteIniBool(L"Mouse", L"FreeMouse", freeMouse);

        logHooks = api->ReadIniBool(L"Mouse", L"LogHooks", false);
        api->WriteIniBool(L"Mouse", L"LogHooks", logHooks);

        api->LogInfo("Game version: " + GameVersions[api->GetGameVersion()]);

        // Quit if loader disabled
        if (!loaderEnabled) {
            api->LogInfo("Loader disabled, quitting!");
            return TRUE;
        }

        // Initialize mod lists
        mods = GetMods();

        // Show launcher window
        if (!skipLauncher) {

            bool result = !ShowLauncherWindow(hModule);
            SaveDisabledMods(mods);
            if (result) {
                api->LogInfo("Exiting modloader.");
                ExitProcess(0);
            }
            
            // Load mods
            LoadMods(mods);
        }
        else {

            // Load mods
            LoadMods(mods);
        }

        // Initialize API
        ApiSetup();

        // Initialize cheats
        SetupCheats();

        // Call other components
        DisableThreadLibraryCalls(hModule);

        if (guiEnabled) {
            CreateThread(nullptr, 0, ImGuiInitThread, nullptr, 0, nullptr);
        }

        if (freeMouse) {
            CreateThread(nullptr, 0, MouseInitThread, nullptr, 0, nullptr);
        }

        CreateThread(nullptr, 0, HotkeyThread, nullptr, 0, nullptr);
    }
    }
    return TRUE;
}
