#pragma once
#include "Utils.h"
#include "ModApi.h"

#include <Windows.h>
#include <sstream>

struct Mod {
    FileVersionInfo info;
    PathInfo path;
    bool enabled = true;

    std::wstring getName();
};

extern std::vector<Mod> mods;

std::vector<std::wstring> GetDisabledMods();
void SaveDisabledMods(std::vector<Mod> disabledMods);

std::vector<Mod> GetMods();
void LoadMods(std::vector<Mod> mods);
