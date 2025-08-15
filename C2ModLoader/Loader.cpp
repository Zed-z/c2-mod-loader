#include "Loader.h"

extern ModApi* api;

std::vector<Mod> mods;

std::wstring Mod::getName() {
    return (this->info.name.length() > 0)
        ? (this->info.name)
        : (this->path.name);
}

std::vector<std::wstring> GetDisabledMods() {
    std::vector<std::wstring> disabledMods;

    constexpr int disabledModsStringLength = 65536;
    std::unique_ptr<wchar_t[]> disabledModsWchar = std::make_unique<wchar_t[]>(disabledModsStringLength);
    api->ReadIniString(L"Config", L"DisabledMods", L"", disabledModsWchar.get(), disabledModsStringLength);
    std::wstring disabledModsStr(disabledModsWchar.get());

    std::wstring token;
    std::wstringstream wss(disabledModsStr);
    while (std::getline(wss, token, L'|')) {
        disabledMods.push_back(token);
    }

    return disabledMods;
}

void SaveDisabledMods(std::vector<Mod> mods) {

    std::wstring disabledModsStr;

    bool first = true;
    for (const auto& mod : mods) {
        if (mod.enabled) continue;

        if (!first) disabledModsStr += L"|";
        disabledModsStr += mod.path.path;
        first = false;
    }

    api->WriteIniString(L"Config", L"DisabledMods", disabledModsStr.c_str());

}

std::vector<Mod> GetMods() {

    std::vector<Mod> mods;

    std::wstring modFolderWstr(MOD_FOLDER_L);

    // Get disabled mods
    std::vector<std::wstring> disabledMods = GetDisabledMods();

    // Get mod files
    std::wstring modFolder = modFolderWstr + L"\\*.asi";
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFile(modFolder.c_str(), &findFileData);

    // No files found
    if (hFind == INVALID_HANDLE_VALUE) return mods;

    // Add mod files
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            std::wstring wideModName(findFileData.cFileName);
            std::wstring modPath = modFolderWstr + L"\\" + wideModName;

            Mod mod;
            mod.info = GetFileVersionInfo(modPath);
            mod.path = GetPathInfo(modPath);
            mod.enabled = std::find(disabledMods.begin(), disabledMods.end(), modPath) == disabledMods.end();

            mods.push_back(mod);
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);

    FindClose(hFind);
    return mods;
}

void LoadMods(std::vector<Mod>& mods) {

    std::vector<Mod> failedMods;

    for (auto& mod : mods) {

        const std::string modName = WStringToString(mod.getName()) + " (" + WStringToString(mod.path.path) + ")";

        // Skip due to settings
        if (!mod.enabled) {
            api->LogInfo("Skipping mod: " + modName);
            continue;
        }

        // API version check
        if (mod.info.apiVersion == -1) {
            api->LogWarning("Mod: " + modName + " does not have a defined API version, issues may arise!");
        }

        if (mod.info.apiVersion > API_VERSION) {
            api->LogError("Failed to load mod: " + modName + " due to incorrect API version: v" + std::to_string(mod.info.apiVersion));
            failedMods.push_back(mod);
            continue;
        }

        // Load the mod file
        HMODULE loaded = LoadLibraryW(mod.path.path.c_str());
        if (!loaded) {
            DWORD err = GetLastError();
            api->LogError("Failed to load mod: " + modName + " with error code: " + std::to_string(err));
            failedMods.push_back(mod);
            continue;
        }

        mod.handle = loaded;
        api->LogInfo("Loaded mod: " + modName + " (API v" + std::to_string(mod.info.apiVersion) + ") - handle: " + std::to_string((int)mod.handle));
    }

    // Show message box when mods failed to load
    if (failedMods.size() > 0) {

        std::wstring message;

        if (failedMods.size() > 0) {
            message += L"\n\nFailed to load:";
            for (auto& mod : failedMods) {
                message += L"\n- " + mod.path.path;
            }
            message += L"\nCheck the log for details.";
        }

        MessageBoxW(NULL, message.c_str(), LOADER_NAME_L, MB_OK | MB_ICONERROR);
    }
}





std::vector<void(__stdcall*)()> physicsCallbacks;

void __stdcall RunPhysicsHooks() {
    for (auto& callback : physicsCallbacks) {
        callback();
    }
}

void ApiSetup() {

    // Hook physics callbacks
    /*
        Croc2.exe+4A591 - 39 1D 0CA24A00        - cmp [Croc2.exe+AA20C],ebx { (1) }
    */
    uintptr_t hookAddr = 0x0044A591;
    size_t hookLength = 6;

    api->HookFunction(hookAddr, hookLength, RunPhysicsHooks);
}