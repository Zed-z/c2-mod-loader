#include "RegistryManager.h"

#include "MinHook.h"
#include "ModApi.h"
#include "Registry/ManagedKeys.h"
#include "Registry/PathOverrides.h"
#include "Registry/ValueMapParser.h"
#include "Utils.h"

#include <string>
#include <unordered_map>

extern ModApi *api;

namespace {

bool g_enabled = true;

LSTATUS(WINAPI *g_originalRegQueryValueExA)(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) = nullptr;
LSTATUS(WINAPI *g_originalRegSetValueExA)(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData) = nullptr;

void LogRegistryRequest(const std::string &apiName, const std::string &valueName) {
	std::string message = "[" + apiName + "] Registry request: " + valueName;
	api->LogDebug(message.c_str());
}

LSTATUS WINAPI HookedRegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
	(void)hKey;
	(void)lpReserved;

	const std::string valueName = lpValueName != nullptr ? std::string(lpValueName) : std::string();
	LogRegistryRequest("RegQueryValueExA", valueName);

	bool handled = false;

	LSTATUS handledStatus = RegistryPathOverrides::TryHandleReadOverride(valueName, lpType, lpData, lpcbData, "RegQueryValueExA", handled);
	if (handled)
		return handledStatus;

	handledStatus = RegistryManagedKeys::TryHandleManagedRead(valueName, lpType, lpData, lpcbData, "RegQueryValueExA", handled);
	if (handled)
		return handledStatus;

	LSTATUS status = g_originalRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	return status;
}

LSTATUS WINAPI HookedRegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData) {
	const std::string valueName = lpValueName != nullptr ? std::string(lpValueName) : std::string();
	LogRegistryRequest("RegSetValueExA", valueName);

	const LSTATUS handledStatus = RegistryManagedKeys::HandleManagedWrite(valueName, dwType, lpData, cbData);
	if (handledStatus != ERROR_CALL_NOT_IMPLEMENTED)
		return handledStatus;

	return g_originalRegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
}

} // namespace

namespace RegistryManager {

void SetEnabled(bool enabled) {
	g_enabled = enabled;
}

void SetManagedKeys(const std::string &managedKeys) {
	const std::unordered_map<std::string, std::string> rawKeys = RegistryValueMapParser::ParseKeyValueMap(managedKeys);
	RegistryManagedKeys::LoadManagedKeys(rawKeys, "LoadConfig");
}

void InstallHooks() {
	if (!g_enabled) {
		api->LogInfo("Registry path remapper disabled by config.");
		return;
	}

	MH_STATUS initStatus = MH_Initialize();
	if (initStatus != MH_OK && initStatus != MH_ERROR_ALREADY_INITIALIZED) {
		api->LogWarning("[RegistryPathRemapper] Failed to initialize MinHook");
		return;
	}

	if (MH_CreateHookApi(L"advapi32", "RegQueryValueExA", (void *)&HookedRegQueryValueExA, reinterpret_cast<void **>(&g_originalRegQueryValueExA)) != MH_OK || MH_EnableHook((void *)&RegQueryValueExA) != MH_OK) {
		api->LogWarning("[RegistryPathRemapper] Failed to hook RegQueryValueExA");
	}

	if (MH_CreateHookApi(L"advapi32", "RegSetValueExA", (void *)&HookedRegSetValueExA, reinterpret_cast<void **>(&g_originalRegSetValueExA)) != MH_OK || MH_EnableHook((void *)&RegSetValueExA) != MH_OK) {
		api->LogWarning("[RegistryPathRemapper] Failed to hook RegSetValueExA");
	}
}

} // namespace RegistryManager
