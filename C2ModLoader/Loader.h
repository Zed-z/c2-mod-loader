#pragma once
#include "FileOverrides.h"
#include "ModApi.h"
#include "Utils.h"

#include <sstream>
#include <string>
#include <windows.h>

struct Mod {
	FileVersionInfo info;
	PathInfo path;
	bool enabled = true;
	HMODULE handle = nullptr;
	std::string fileHash;

	std::wstring overridePath;
	std::vector<FileOverrideEntry> fileOverrides;

	std::wstring getName();
};

void LoadConfig();

extern Mod modLoader;
extern std::vector<Mod> mods;
extern int modsLoaded;
extern std::string loadedModsHash;

void SetupDirectories();

std::vector<std::wstring> GetDisabledMods();
void SaveDisabledMods(std::vector<Mod> disabledMods);

Mod GetModLoader();
std::vector<Mod> GetMods();
void LoadMods(std::vector<Mod> &mods);

void ApiSetup();

extern ModApi *api;
inline Mod *GetModByHandle(HMODULE handle) {
	for (auto &mod : mods) {
		if (mod.handle == handle) {
			return &mod;
		}
	}
	return nullptr;
}

inline Mod *GetMod() {
	HMODULE handle = GetCallingModule();
	return GetModByHandle(handle);
}
