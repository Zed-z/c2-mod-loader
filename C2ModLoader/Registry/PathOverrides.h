#pragma once

#include <string>
#include <windows.h>

namespace RegistryPathOverrides {

LSTATUS TryHandleReadOverride(
	const std::string &valueName,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData,
	const char *apiName,
	bool &handled);

} // namespace RegistryPathOverrides
