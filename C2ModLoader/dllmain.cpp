#define IS_MOD_LOADER
#include "ModApi.h"

#include <Windows.h>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>

#define LOADER_NAME "Croc 2 Mod Loader"
#define LOG_FILE "c2modloader.log"
#define MOD_FOLDER "mods"

void Log(const std::string& message) {
    std::ofstream log(LOG_FILE, std::ios::app);
    if (!log.is_open()) return;

    // Get current time
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo;
    localtime_s(&timeInfo, &now_c);

    // Format and write the timestamp and message
    log << "[" << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S") << "] " << message << std::endl;

    log.close();
}


// API
ModApi g_ModApi = {
    Log
};

extern "C" __declspec(dllexport) ModApi * GetModApi() {
    return &g_ModApi;
}


// Function to show a popup for each loaded mod
void ShowModPopup(std::vector<std::string> loadedMods) {
    std::string message = "Loaded mods:";
    for (auto& mod : loadedMods) {
        message += "\n- " + mod;
    }
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

    // Load mod files
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            // Convert the wide string (wchar_t*) to a std::string
            std::wstring wideModName(findFileData.cFileName);
            std::string modFileName(wideModName.begin(), wideModName.end());
            std::string modPath = std::string(folder) + "\\" + modFileName;

            // Load the .asi file
            LoadLibraryA(modPath.c_str());

            // Log and show popup for each loaded mod
            Log("Loaded mod: " + modPath);
            loadedMods.push_back(modFileName);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    ShowModPopup(loadedMods);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        LoadMods(MOD_FOLDER);
    }
    }
    return TRUE;
}
