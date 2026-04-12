#include "PathOverrides.h"

#include "ModApi.h"
#include "Registry/QueryReturns.h"
#include "Utils.h"

#include <string>

extern ModApi *api;

namespace {

void LogRegistryOverride(const std::string &apiName, const std::string &valueName, const std::string &rewrittenValue) {
	std::string message = "[" + apiName + "] Registry path override: " + valueName + " -> " + rewrittenValue;
	api->LogDebug(message.c_str());
}

std::string GetCurrentGameDirectory() {
	static const PathInfo mainModulePath = GetModuleFilepath(GetModuleHandleA(nullptr));
	return WStringToString(mainModulePath.directory);
}

bool IsPathValueName(const std::string &valueName) {
	if (valueName.empty()) {
		return false;
	}
	return valueName == "Path" || valueName == "InstallPath" || valueName == "CDPath";
}

} // namespace

namespace RegistryPathOverrides {

LSTATUS TryHandleReadOverride(const std::string &valueName, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData, const char *apiName, bool &handled) {
	if (!IsPathValueName(valueName)) {
		handled = false;
		return ERROR_SUCCESS;
	}

	const std::string stringValue = GetCurrentGameDirectory();
	handled = true;

	LogRegistryOverride(apiName != nullptr ? apiName : "RegQueryValueExA", valueName, stringValue);
	return RegistryQueryReturns::ReturnString(stringValue, lpType, lpData, lpcbData);
}

} // namespace RegistryPathOverrides
