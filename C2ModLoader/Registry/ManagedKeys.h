#pragma once

#include <string>
#include <unordered_map>
#include <windows.h>

namespace RegistryManagedKeys {

void LoadManagedKeys(
	const std::unordered_map<std::string, std::string> &rawOverrides,
	const char *apiName);

LSTATUS TryHandleManagedRead(
	const std::string &valueName,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData,
	const char *apiName,
	bool &handled);

LSTATUS HandleManagedWrite(
	const std::string &valueName,
	DWORD dwType,
	const BYTE *lpData,
	DWORD cbData);

} // namespace RegistryManagedKeys
