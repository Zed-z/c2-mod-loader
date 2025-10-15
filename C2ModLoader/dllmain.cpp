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
        loaderEnabled = api->SetupIniBool(L"Config", L"LoaderEnabled", true);
        skipLauncher = api->SetupIniBool(L"Config", L"SkipLauncher", false);
        guiEnabled = api->SetupIniBool(L"GUI", L"GuiEnabled", true);
        showGui = api->SetupIniBool(L"GUI", L"ShowGui", false);
        showLog = api->SetupIniBool(L"GUI", L"ShowLog", false);
        showInputs = api->SetupIniBool(L"GUI", L"ShowInputs", false);
        showObjectList = api->SetupIniBool(L"GUI", L"ShowObjectList", false);
        showCoords = api->SetupIniBool(L"GUI", L"ShowCoords", false);
        incompatibleWarningShown = api->SetupIniBool(L"GUI", L"IncompatibleWarningShown", false);

        showLogInfo = api->SetupIniBool(L"Logging", L"Info", true);
        showLogDebug = api->SetupIniBool(L"Logging", L"Debug", false);
        showLogWarning = api->SetupIniBool(L"Logging", L"Warning", true);
        showLogError = api->SetupIniBool(L"Logging", L"Error", true);

        showToastInfo = api->SetupIniBool(L"Toasts", L"Info", true);
        showToastDebug = api->SetupIniBool(L"Toasts", L"Debug", false);
        showToastWarning = api->SetupIniBool(L"Toasts", L"Warning", true);
        showToastError = api->SetupIniBool(L"Toasts", L"Error", true);

        freeMouse = api->SetupIniBool(L"Mouse", L"FreeMouse", true);
        logHooks = api->SetupIniBool(L"Mouse", L"LogHooks", false);

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
