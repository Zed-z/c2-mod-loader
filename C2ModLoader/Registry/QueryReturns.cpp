#include "QueryReturns.h"

#include <string>
#include <windows.h>

namespace RegistryQueryReturns {

LSTATUS ReturnString(const std::string &replacementValue, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
	if (lpcbData == nullptr) {
		return ERROR_INVALID_PARAMETER;
	}

	if (lpType != nullptr) {
		*lpType = REG_SZ;
	}

	const DWORD requiredBytes = (DWORD)(replacementValue.size() + 1);
	if (lpData == nullptr) {
		*lpcbData = requiredBytes;
		return ERROR_SUCCESS;
	}
	if (*lpcbData < requiredBytes) {
		*lpcbData = requiredBytes;
		return ERROR_MORE_DATA;
	}

	memcpy(lpData, replacementValue.c_str(), requiredBytes);
	*lpcbData = requiredBytes;
	return ERROR_SUCCESS;
}

LSTATUS ReturnDword(DWORD replacementValue, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
	if (lpcbData == nullptr) {
		return ERROR_INVALID_PARAMETER;
	}

	if (lpType != nullptr) {
		*lpType = REG_DWORD;
	}

	constexpr DWORD requiredBytes = sizeof(DWORD);
	if (lpData == nullptr) {
		*lpcbData = requiredBytes;
		return ERROR_SUCCESS;
	}
	if (*lpcbData < requiredBytes) {
		*lpcbData = requiredBytes;
		return ERROR_MORE_DATA;
	}

	*reinterpret_cast<DWORD *>(lpData) = replacementValue;
	*lpcbData = requiredBytes;
	return ERROR_SUCCESS;
}

} // namespace RegistryQueryReturns
