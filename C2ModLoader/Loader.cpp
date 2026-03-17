#include "Loader.h"

#include <filesystem>
#include <regex>

namespace fs = std::filesystem;

extern ModApi *api;

std::vector<Mod> mods;
int modsLoaded = 0;

std::wstring Mod::getName() {
	return (this->info.name.length() > 0)
		? (this->info.name)
		: (this->path.name);
}

extern bool loaderEnabled;
extern bool skipLauncher;
extern bool freeMouse;
extern bool guiEnabled;
extern bool showGui;
extern bool showLog;
extern bool showInputs;
extern bool showObjectList;
extern bool showCoords;
extern bool showLevelInfo;
extern bool showSaveSlotList;
extern bool showLogInfo;
extern bool showLogDebug;
extern bool showLogWarning;
extern bool showLogError;
extern bool showToastInfo;
extern bool showToastDebug;
extern bool showToastWarning;
extern bool showToastError;

void LoadConfig() {
	loaderEnabled = api->SetupIniBool(L"Config", L"LoaderEnabled", true);
	skipLauncher = api->SetupIniBool(L"Config", L"SkipLauncher", false);
	freeMouse = api->SetupIniBool(L"Config", L"FreeMouse", true);

	guiEnabled = api->SetupIniBool(L"GUI", L"GuiEnabled", true);
	showGui = api->SetupIniBool(L"GUI", L"ShowGui", false);
	showLog = api->SetupIniBool(L"GUI", L"ShowLog", false);
	showInputs = api->SetupIniBool(L"GUI", L"ShowInputs", false);
	showObjectList = api->SetupIniBool(L"GUI", L"ShowObjectList", false);
	showCoords = api->SetupIniBool(L"GUI", L"ShowCoords", false);
	showLevelInfo = api->SetupIniBool(L"GUI", L"ShowLevelInfo", false);
	showSaveSlotList = api->SetupIniBool(L"GUI", L"ShowSaveSlotList", false);

	showLogInfo = api->SetupIniBool(L"Logging", L"Info", true);
	showLogDebug = api->SetupIniBool(L"Logging", L"Debug", false);
	showLogWarning = api->SetupIniBool(L"Logging", L"Warning", true);
	showLogError = api->SetupIniBool(L"Logging", L"Error", true);

	showToastInfo = api->SetupIniBool(L"Toasts", L"Info", true);
	showToastDebug = api->SetupIniBool(L"Toasts", L"Debug", false);
	showToastWarning = api->SetupIniBool(L"Toasts", L"Warning", true);
	showToastError = api->SetupIniBool(L"Toasts", L"Error", true);
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

			mods.push_back(mod);
		}
	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);
	return mods;
}

void LoadMods(std::vector<Mod> &mods) {

	std::vector<Mod> failedMods;

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
		fs::path overridePath = std::regex_replace(mod.path.path, std::wregex(L".asi$"), L"");

		try {
			if (fs::exists(overridePath) && fs::is_directory(overridePath)) {
				fs::copy(
					overridePath, FILE_OVERRIDE_DIRECTORY_L,
					fs::copy_options::recursive | fs::copy_options::update_existing | fs::copy_options::overwrite_existing);
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
	}

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

std::vector<void(__stdcall *)()> physicsCallbacks;

void __stdcall RunPhysicsHooks() {
	for (auto &callback : physicsCallbacks) {
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