#include "Loader.h"

#include "Camera/Camera.h"
#include "GameHooks.h"
#include "Input/Input.h"
#include "Overlay/Overlay.h"
#include "Registry/RegistryManager.h"
#include "Utils.h"
#include "Utils/Sha256.h"

#include <algorithm>
#include <filesystem>
#include <windows.h>

namespace fs = std::filesystem;

extern ModApi *api;

Mod modLoader;
std::vector<Mod> mods;
int modsLoaded = 0;
std::string loadedModsHash = "";

std::wstring Mod::getName() {
	return (this->info.name.length() > 0)
		? (this->info.name)
		: (this->path.name);
}

extern bool loaderEnabled;
extern bool skipLauncher;
extern bool freeMouse;

void LoadConfig() {
	loaderEnabled = api->SetupIniBool(L"Config", L"LoaderEnabled", true);
	skipLauncher = api->SetupIniBool(L"Config", L"SkipLauncher", false);
	freeMouse = api->SetupIniBool(L"Config", L"FreeMouse", true);

	RegistryManager::SetEnabled(api->SetupIniBool(L"RegistryBypass", L"Enabled", true));
	constexpr DWORD registryOverridesLength = 4096;
	wchar_t managedOverrides[registryOverridesLength] = {};
	api->SetupIniString(L"RegistryBypass", L"ManagedValues", L"", managedOverrides, registryOverridesLength);
	RegistryManager::SetManagedKeys(WStringToString(managedOverrides));
}

void SetupDirectories() {

	// Mod directory
	try {
		fs::create_directories(MOD_DIRECTORY_L);
	} catch (const fs::filesystem_error &e) {
		std::string message = "Error creating mod directory: " + std::string(e.what());
		api->LogError(message.c_str());
		return;
	}

	// File overrides
	try {
		if (fs::exists(FILE_OVERRIDE_DIRECTORY_L)) {
			fs::remove_all(FILE_OVERRIDE_DIRECTORY_L);
		}
		fs::create_directories(FILE_OVERRIDE_DIRECTORY_L);
	} catch (const fs::filesystem_error &e) {
		std::string message = "Error creating file override directory: " + std::string(e.what());
		api->LogError(message.c_str());
		return;
	}
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
	for (const auto &mod : mods) {
		if (mod.enabled)
			continue;

		if (!first)
			disabledModsStr += L"|";
		disabledModsStr += mod.path.path;
		first = false;
	}

	api->WriteIniString(L"Config", L"DisabledMods", disabledModsStr.c_str());
}

Mod GetModLoader() {
	Mod loader;

	std::wstring modPath = L"C2ModLoader.asi";
	loader.info = GetFileVersionInfo(modPath);
	loader.path = GetPathInfo(modPath);
	loader.enabled = true;

	loader.fileHash = Sha256::ComputeFileHash(WStringToString(modPath).c_str());

	loader.overridePath = MOD_DIRECTORY_L L"\\C2ModLoader";
	GetFileOverrides(loader.overridePath, loader.fileOverrides);

	return loader;
}

std::vector<Mod> GetMods() {

	std::vector<Mod> mods;

	std::wstring modDirectoryWstr(MOD_DIRECTORY_L);

	// Get disabled mods
	std::vector<std::wstring> disabledMods = GetDisabledMods();

	// Get mod files
	std::wstring modDirectory = modDirectoryWstr + L"\\*.asi";
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = FindFirstFile(modDirectory.c_str(), &findFileData);

	// No files found
	if (hFind == INVALID_HANDLE_VALUE)
		return mods;

	// Add mod files
	do {
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

			std::wstring wideModName(findFileData.cFileName);
			std::wstring modPath = modDirectoryWstr + L"\\" + wideModName;

			Mod mod;
			mod.info = GetFileVersionInfo(modPath);
			mod.path = GetPathInfo(modPath);
			mod.enabled = std::find(disabledMods.begin(), disabledMods.end(), modPath) == disabledMods.end();

			mod.fileHash = Sha256::ComputeFileHash(WStringToString(modPath).c_str());

			mod.overridePath = GetFileOverridePath(modPath);
			GetFileOverrides(mod.overridePath, mod.fileOverrides);

			mods.push_back(mod);
		}
	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);
	return mods;
}

void LoadMods(std::vector<Mod> &mods) {

	std::vector<Mod> failedMods;
	std::string modsHashConcat = modLoader.fileHash;

	// Load file overrides
	try {
		const fs::path loaderOverridePath = fs::path(MOD_DIRECTORY_L) / L"C2ModLoader";
		if (fs::exists(loaderOverridePath) && fs::is_directory(loaderOverridePath)) {
			fs::copy(
				loaderOverridePath, FILE_OVERRIDE_DIRECTORY_L,
				fs::copy_options::recursive | fs::copy_options::update_existing | fs::copy_options::overwrite_existing);
			api->LogInfo("Loaded file overrides: C2ModLoader");
		}
	} catch (const fs::filesystem_error &e) {
		std::string message = "Failed loading file overrides: C2ModLoader (" + std::string(e.what()) + ")";
		api->LogError(message.c_str());
	}

	// Load mods
	for (auto &mod : mods) {

		const std::string modName = WStringToString(mod.getName()) + " (" + WStringToString(mod.path.path) + ")";

		// Skip due to settings
		if (!mod.enabled) {
			std::string message = "Skipping mod: " + modName;
			api->LogInfo(message.c_str());
			continue;
		}

		// API version check
		if (mod.info.apiVersion == -1) {
			std::string message = "Mod: " + modName + " does not have a defined API version, issues may arise!";
			api->LogWarning(message.c_str());
		} else if (mod.info.apiVersion != API_VERSION) {
			std::string message = "Failed to load mod: " + modName + " due to a non-matching API version: v" + std::to_string(mod.info.apiVersion);
			api->LogError(message.c_str());
			failedMods.push_back(mod);
			continue;
		}

		// Load file overrides
		const std::wstring &overridePath = mod.overridePath;
		try {
			if (!overridePath.empty() && fs::exists(overridePath) && fs::is_directory(overridePath)) {
				fs::copy(
					fs::path(overridePath), FILE_OVERRIDE_DIRECTORY_L,
					fs::copy_options::recursive | fs::copy_options::update_existing | fs::copy_options::overwrite_existing);
				std::string message = "Loaded file overrides: " + modName;
				api->LogInfo(message.c_str());
			}
		} catch (const fs::filesystem_error &e) {
			std::string message = "Failed loading file overrides: " + modName + " (" + std::string(e.what()) + ")";
			api->LogError(message.c_str());
			failedMods.push_back(mod);
			continue;
		}

		// Load the mod file
		HMODULE loaded = LoadLibraryW(mod.path.path.c_str());
		if (!loaded) {
			DWORD err = GetLastError();
			std::string message = "Failed to load mod: " + modName + " with error code: " + std::to_string(err);
			api->LogError(message.c_str());
			failedMods.push_back(mod);
			continue;
		}

		mod.handle = loaded;
		std::string message = "Loaded mod: " + modName + " (API v" + std::to_string(mod.info.apiVersion) + ") - handle: " + std::to_string((int)mod.handle);
		api->LogInfo(message.c_str());
		modsLoaded++;

		modsHashConcat += mod.fileHash;
	}

	loadedModsHash = Sha256::ComputeHash(modsHashConcat.c_str());

	// Show message box when mods failed to load
	if (failedMods.size() > 0) {

		std::wstring message;

		if (failedMods.size() > 0) {
			message += L"\n\nFailed to load:";
			for (auto &mod : failedMods) {
				message += L"\n- " + mod.path.path;
			}
			message += L"\nCheck the log for details.";
		}

		MessageBoxW(NULL, message.c_str(), LOADER_NAME_L, MB_OK | MB_ICONERROR);
	}
}

void ApiSetup(HMODULE hModule) {
	GameHooks::ApplyHooks();
	Input::Setup();
	Camera::Setup();
	Overlay::Setup(hModule);
}
