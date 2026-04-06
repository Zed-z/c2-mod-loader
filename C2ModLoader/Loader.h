#pragma once
#include "FileOverrides.h"
#include "ModApi.h"
#include "Utils.h"

#include <sstream>
#include <windows.h>

struct Mod {
	FileVersionInfo info;
	PathInfo path;
	bool enabled = true;
	HMODULE handle = nullptr;

	std::wstring overridePath;
	std::vector<FileOverrideEntry> fileOverrides;

	std::wstring getName();
};

void LoadConfig();

extern std::vector<Mod> mods;
extern int modsLoaded;

void SetupDirectories();

std::vector<std::wstring> GetDisabledMods();
void SaveDisabledMods(std::vector<Mod> disabledMods);

std::vector<Mod> GetMods();
void LoadMods(std::vector<Mod> &mods);

extern std::vector<void(__stdcall *)()> physicsCallbacks;
void __stdcall RunPhysicsHooks();

extern std::vector<void(__stdcall *)()> doorChangeCallbacks;
void __stdcall RunDoorChangeHooks();

extern std::vector<void(__stdcall *)()> mapChangeCallbacks;
void __stdcall RunMapChangeHooks();

extern std::vector<void(__stdcall *)()> playerDeathCallbacks;
void __stdcall RunPlayerDeathHooks();

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
