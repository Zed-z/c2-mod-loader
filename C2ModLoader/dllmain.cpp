#define IS_MOD_LOADER
#include "ModApi.h"
#include "Config.h"
#include "ImGuiHandler.h"

#include <Windows.h>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>

ModApi* api;

void ClearLog() {

    char modulePath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(NULL), modulePath, MAX_PATH);
    std::string path(modulePath);
    size_t pos = path.find_last_of("\\/");
    std::string logPath = path.substr(0, pos) + "\\" + LOG_FILE;

    std::ofstream log(logPath, std::ios::trunc);
    if (!log.is_open()) return;

    log.close();
}


// Function to show a popup for each loaded mod
void ShowModPopup(std::vector<std::string> loadedMods, std::vector<std::string> failedMods) {

    // Successfully loaded
    std::string message = "Loaded mods:";
    for (auto& mod : loadedMods) {
        message += "\n- " + mod;
    }

    // Failed to load
    if (failedMods.size() > 0) {
        message += "\n\nFailed to load:";
        for (auto& mod : failedMods) {
            message += "\n- " + mod;
        }
        message += "\nCheck the log for details.";
    }

    // Show message box
    MessageBoxA(NULL, message.c_str(), LOADER_NAME, MB_OK | MB_ICONINFORMATION);
}

// Function to load all .asi mods from the mods folder
void LoadMods(const char* folder) {

    // Get mod files
    std::wstring modFolder = std::wstring(folder, folder + strlen(folder)) + L"\\*.asi";
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(modFolder.c_str(), &findFileData);

    // No files found
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    std::vector<std::string> loadedMods;
    std::vector<std::string> failedMods;

    // Load mod files
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            // Convert the wide string (wchar_t*) to a std::string
            std::wstring wideModName(findFileData.cFileName);
            std::string modFileName(wideModName.begin(), wideModName.end());
            std::string modPath = std::string(folder) + "\\" + modFileName;

            // Load the .asi file
            HMODULE mod = LoadLibraryA(modPath.c_str());
            if (!mod) {
                // Mod failed to load
                DWORD err = GetLastError();
                api->Log("Failed to load mod: " + modPath + " with error code: " + std::to_string(err));
                failedMods.push_back(modFileName);
            }
            else {
                // Log and show popup for each loaded mod
                api->Log("Loaded mod: " + modPath);
                loadedMods.push_back(modFileName);
            }     
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    ShowModPopup(loadedMods, failedMods);
}




static DWORD WINAPI HotkeyThread(LPVOID) {
    RegisterHotKey(NULL, 1, 0, VK_TAB);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            switch (msg.wParam) {
            case 1: // TAB

                showLog = !showLog;
                api->WriteIniInt(L"Config", L"ShowLog", (int)showLog);

                break;
            }
        }
    }
    return 0;
}




BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        api = GetModApi();
        ClearLog();
        LoadMods(MOD_FOLDER);

        // Config
        showLog = (bool)(api->ReadIniInt(L"Config", L"ShowLog", false));
        api->WriteIniInt(L"Config", L"ShowLog", (int)showLog);

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, ImGuiInitThread, api, 0, nullptr);
        CreateThread(nullptr, 0, HotkeyThread, nullptr, 0, nullptr);
    }
    }
    return TRUE;
}
