#pragma once
#include "ModApi.h"
#include "Utils.h"

#include <sstream>
#include <windows.h>

#ifdef _MSC_VER
#define return_address(...) _ReturnAddress()
#elif defined(__MINGW32__)
#define return_address(...) __builtin_return_address(0)
#endif

struct Mod {
	FileVersionInfo info;
	PathInfo path;
	bool enabled = true;
	HMODULE handle = nullptr;

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

void ApiSetup();

namespace {

HMODULE getModuleFromAddress(const void *address) {
	HMODULE module = nullptr;
	GetModuleHandleExA(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		static_cast<LPCSTR>(address),
		&module);
	return module;
}

bool isAsiModule(HMODULE module) {
	if (!module)
		return false;

	wchar_t path[MAX_PATH] = {};
	if (GetModuleFileNameW(module, path, MAX_PATH) == 0)
		return false;

	std::wstring modulePath(path);
	if (modulePath.length() < 4)
		return false;

	std::wstring extension = modulePath.substr(modulePath.length() - 4);
	return extension == L".asi" || extension == L".ASI";
};

} // namespace

inline HMODULE GetCallingModule() {
	HMODULE self = getModuleFromAddress((const void *)&GetCallingModule);

	void *frames[32] = {};
	USHORT capturedFrames = CaptureStackBackTrace(0, (ULONG)(sizeof(frames) / sizeof(frames[0])), frames, nullptr);

	for (USHORT i = 0; i < capturedFrames; ++i) {
		HMODULE frameModule = getModuleFromAddress(frames[i]);
		if (!frameModule || frameModule == self)
			continue;

		// Found asi module in call stack
		if (isAsiModule(frameModule)) {
			return frameModule;
		}
	}

	// Only loader module found in call stack
	if (self) {
		return self;
	}

	// Fallback to return address
	return getModuleFromAddress((const void *)return_address());
}

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
