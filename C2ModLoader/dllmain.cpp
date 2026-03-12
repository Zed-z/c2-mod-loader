#define IS_MOD_LOADER
#include "ModApi.h"
#include "Config.h"
#include "ImGuiHandler.h"
#include "MouseCaptureRemover.h"
#include "Utils.h"
#include "Resource.h"
#include "Launcher/Launcher.h"
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

static HANDLE g_mainThreadHandle = nullptr;

namespace {

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

static DWORD WINAPI ModLoaderMainThread(LPVOID param) {
    HMODULE hModule = (HMODULE)param;

    if (g_mainThreadHandle) {
        SuspendThread(g_mainThreadHandle);
    }

    std::string gameVersionMessage = std::string("Game version: ") + GameVersions[api->GetGameVersion()];
    api->LogInfo(gameVersionMessage.c_str());

    SetupDirectories();

    mods = GetMods();

    if (!skipLauncher) {
        bool result = Launcher::ShowLauncherWindow(hModule);
        if (!result) {
            api->LogInfo("Exiting modloader.");
            ExitProcess(0);
            return 0;
        }
    }

    // Reload config after launcher in case settings were changed
    LoadConfig();

    if (loaderEnabled) {
        LoadMods(mods);
    }

    CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        Sleep(1000);
        std::wstring gameName = L"Croc 2";
        HWND hwnd = FindWindow(NULL, gameName.c_str());
        if (hwnd) {
            std::wstring newTitle = gameName + L" (" + LOADER_NAME_L + L")";
            SetWindowText(hwnd, newTitle.c_str());
        }
        return 0;
        }, nullptr, 0, nullptr);

    ApiSetup();
    SetupCheats();

    if (guiEnabled) {
        CreateThread(nullptr, 0, ImGuiInitThread, hModule, 0, nullptr);
    }

    if (freeMouse) {
        CreateThread(nullptr, 0, MouseInitThread, nullptr, 0, nullptr);
    }

    CreateThread(nullptr, 0, HotkeyThread, nullptr, 0, nullptr);

    // Camera tilt on main menu
    CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        double timer = 0;

        while (true) {
            Sleep(100);

            LevelInfo levelInfo = api->GetLevelInfo();
            if (levelInfo.tribe == 0 && levelInfo.level == 0 && levelInfo.map == 0) {
                StratEntity* camera = api->GetEntity(ADDR_ROOT_OBJ);
                if (camera != nullptr) {
                    double camMod = sin(timer);
                    camera->newRotPos = { 0, (int)(8388608 + camMod * 100000), 0, 54394, -1024, 30854 };
                }
            }

            timer += 0.1;
        }
    }, nullptr, 0, nullptr);

    if (g_mainThreadHandle) {
        ResumeThread(g_mainThreadHandle);
        CloseHandle(g_mainThreadHandle);
        g_mainThreadHandle = nullptr;
    }

    return 0;
}

}

// Entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        api = GetModApi();
        ClearLog();

        LoadConfig();

        HANDLE duplicatedMainThread = nullptr;
        DuplicateHandle(
            GetCurrentProcess(),
            GetCurrentThread(),
            GetCurrentProcess(),
            &duplicatedMainThread,
            THREAD_SUSPEND_RESUME,
            FALSE,
            0
        );
        g_mainThreadHandle = duplicatedMainThread;

        DisableThreadLibraryCalls(hModule);

        HANDLE hModLoaderThread = CreateThread(nullptr, 0, ModLoaderMainThread, hModule, 0, nullptr);
        if (!hModLoaderThread && g_mainThreadHandle) {
            CloseHandle(g_mainThreadHandle);
            g_mainThreadHandle = nullptr;
        }
    }
    }
    return TRUE;
}
