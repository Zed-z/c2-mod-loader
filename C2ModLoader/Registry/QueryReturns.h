#pragma once

#include <string>
#include <windows.h>

namespace RegistryQueryReturns {

LSTATUS ReturnString(const std::string &replacementValue, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
LSTATUS ReturnDword(DWORD replacementValue, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);

} // namespace RegistryQueryReturns
