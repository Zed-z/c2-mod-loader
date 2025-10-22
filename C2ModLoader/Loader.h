#pragma once
#include "Utils.h"
#include "ModApi.h"

#include <Windows.h>
#include <sstream>

struct Mod {
    FileVersionInfo info;
    PathInfo path;
    bool enabled = true;
    HMODULE handle = nullptr;

    std::wstring getName();
};

extern std::vector<Mod> mods;


void SetupDirectories();


std::vector<std::wstring> GetDisabledMods();
void SaveDisabledMods(std::vector<Mod> disabledMods);


std::vector<Mod> GetMods();
void LoadMods(std::vector<Mod>& mods);


extern std::vector<void(__stdcall*)()> physicsCallbacks;
void __stdcall RunPhysicsHooks();


void ApiSetup();


inline HMODULE GetCallingModule() {
    HMODULE caller = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)_ReturnAddress(),
        &caller
    );
    return caller;
}


extern ModApi* api;
inline Mod* GetModByHandle(HMODULE handle) {
    for (auto& mod : mods) {
        if (mod.handle == handle) {
            return &mod;
        }
    }
    return nullptr;
}

inline Mod* GetMod() {
    HMODULE handle = GetCallingModule();
    return GetModByHandle(handle);
}
