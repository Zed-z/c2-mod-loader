#include "ManagedKeys.h"

#include "ModApi.h"
#include "Registry/QueryReturns.h"
#include "Registry/ValueMapParser.h"
#include "Utils.h"

#include <array>
#include <cctype>
#include <cstdlib>
#include <string>

extern ModApi *api;

namespace {

std::unordered_map<std::string, std::string> g_managedOverrides;

enum class ManagedValueType {
	String,
	Dword,
};

struct ManagedKeySpec {
	const char *pattern;
	ManagedValueType type;
};

bool MatchExactOrPrefix(const std::string &pattern, const std::string &value) {
	if (!pattern.empty() && pattern.back() == '*') {
		const std::string prefix = pattern.substr(0, pattern.size() - 1);
		return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
	}
	return pattern == value;
}

const std::array<ManagedKeySpec, 29> &GetManagedKeySpecs() {
	static const std::array<ManagedKeySpec, 29> specs = {{
		{"Brightness", ManagedValueType::String},
		{"Contrast", ManagedValueType::String},
		{"D3DDevice", ManagedValueType::String},
		{"DisplayDevice", ManagedValueType::String},
		{"DrawDistanceScale", ManagedValueType::String},
		{"Gamma", ManagedValueType::String},
		{"InputDevice*", ManagedValueType::String},
		{"Alpha", ManagedValueType::Dword},
		{"AlphaColourKey", ManagedValueType::Dword},
		{"AlphaDepthCue", ManagedValueType::Dword},
		{"AmbientSounds", ManagedValueType::Dword},
		{"Bilinear", ManagedValueType::Dword},
		{"ColouredLighting", ManagedValueType::Dword},
		{"Dither", ManagedValueType::Dword},
		{"DrawDistanceMode", ManagedValueType::Dword},
		{"Fades", ManagedValueType::Dword},
		{"Indexed", ManagedValueType::Dword},
		{"Language", ManagedValueType::Dword},
		{"Lighting", ManagedValueType::Dword},
		{"MusicVolume", ManagedValueType::Dword},
		{"Perspective", ManagedValueType::Dword},
		{"Quality", ManagedValueType::Dword},
		{"ScreenBPP", ManagedValueType::Dword},
		{"ScreenH", ManagedValueType::Dword},
		{"ScreenW", ManagedValueType::Dword},
		{"SortColourKey", ManagedValueType::Dword},
		{"SoundVolume", ManagedValueType::Dword},
		{"SpeechVolume", ManagedValueType::Dword},
		{"TextureAlpha", ManagedValueType::Dword},
	}};
	return specs;
}

bool TryGetManagedKeyType(const std::string &valueName, ManagedValueType &outType) {
	for (const ManagedKeySpec &spec : GetManagedKeySpecs()) {
		if (MatchExactOrPrefix(spec.pattern, valueName)) {
			outType = spec.type;
			return true;
		}
	}

	return false;
}

bool TryReadAnsiString(const BYTE *lpData, DWORD cbData, std::string &outValue) {
	if (lpData == nullptr || cbData == 0) {
		outValue.clear();
		return true;
	}

	const char *charData = reinterpret_cast<const char *>(lpData);

	size_t actualLength = 0;
	for (size_t i = 0; i < cbData; ++i) {
		if (charData[i] == '\0') {
			actualLength = i;
			break;
		}
		actualLength = i + 1;
	}

	outValue = std::string(charData, actualLength);
	return true;
}

void SaveManagedKeys(
	const std::unordered_map<std::string, std::string> &managedOverrides) {
	const std::wstring managedValues = StringToWString(RegistryValueMapParser::SerializeKeyValueMap(managedOverrides));
	api->WriteIniString(L"RegistryBypass", L"ManagedValues", managedValues.c_str());
}

void LogIniManaged(const std::string &apiName, const std::string &valueName, const std::string &details) {
	std::string message = "[" + apiName + "] [ManagedKeys] " + valueName + " - " + details;
	api->LogDebug(message.c_str());
}

void LogIniManagedWarning(const std::string &apiName, const std::string &valueName, const std::string &details) {
	std::string message = "[" + apiName + "] [ManagedKeys] " + valueName + " - " + details;
	api->LogWarning(message.c_str());
}

} // namespace

namespace RegistryManagedKeys {

void LoadManagedKeys(const std::unordered_map<std::string, std::string> &rawOverrides, const char *apiName) {
	const std::string sourceApi = apiName != nullptr ? apiName : "LoadConfig";
	g_managedOverrides.clear();

	for (const auto &pair : rawOverrides) {
		ManagedValueType managedType = ManagedValueType::String;
		if (!TryGetManagedKeyType(pair.first, managedType)) {
			LogIniManagedWarning(sourceApi, pair.first, "key type mismatch when setting up");
			continue;
		}

		if (managedType == ManagedValueType::Dword) {
			char *end = nullptr;
			const unsigned long parsed = std::strtoul(pair.second.c_str(), &end, 0);
			if (end == pair.second.c_str() || *end != '\0') {
				LogIniManagedWarning(sourceApi, pair.first, "key type mismatch when setting up");
				continue;
			}
			(void)parsed;
		}

		g_managedOverrides[pair.first] = pair.second;
	}
}

LSTATUS TryHandleManagedRead(const std::string &valueName, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData, const char *apiName, bool &handled) {
	const std::string sourceApi = apiName != nullptr ? apiName : "RegQueryValueExA";
	ManagedValueType managedType = ManagedValueType::String;
	if (!TryGetManagedKeyType(valueName, managedType)) {
		handled = false;
		return ERROR_SUCCESS;
	}

	const auto valueIt = g_managedOverrides.find(valueName);
	if (valueIt == g_managedOverrides.end()) {
		handled = true;
		LogIniManaged(sourceApi, valueName, "key missing in INI file");
		return ERROR_FILE_NOT_FOUND;
	}

	if (managedType == ManagedValueType::Dword) {
		char *end = nullptr;
		const unsigned long parsed = std::strtoul(valueIt->second.c_str(), &end, 0);
		if (end == valueIt->second.c_str() || *end != '\0') {
			handled = true;
			LogIniManagedWarning(sourceApi, valueName, "key type mismatch when reading from INI");
			return ERROR_FILE_NOT_FOUND;
		}

		const DWORD dwordValue = static_cast<DWORD>(parsed);
		handled = true;
		LogIniManaged(sourceApi, valueName, "read REG_DWORD integer from INI: " + std::to_string(dwordValue));
		return RegistryQueryReturns::ReturnDword(dwordValue, lpType, lpData, lpcbData);
	}

	handled = true;
	LogIniManaged(sourceApi, valueName, "read REG_SZ string from INI: " + valueIt->second);
	return RegistryQueryReturns::ReturnString(valueIt->second, lpType, lpData, lpcbData);
}

LSTATUS HandleManagedWrite(
	const std::string &valueName,
	DWORD dwType,
	const BYTE *lpData,
	DWORD cbData) {
	ManagedValueType managedType = ManagedValueType::String;
	if (!TryGetManagedKeyType(valueName, managedType)) {
		return ERROR_CALL_NOT_IMPLEMENTED;
	}

	if (managedType == ManagedValueType::Dword && dwType == REG_DWORD) {
		if (lpData == nullptr || cbData < sizeof(DWORD)) {
			return ERROR_INVALID_PARAMETER;
		}
		const DWORD value = *reinterpret_cast<const DWORD *>(lpData);
		g_managedOverrides[valueName] = std::to_string(value);
		SaveManagedKeys(g_managedOverrides);
		LogIniManaged("RegSetValueExA", valueName, "wrote REG_DWORD integer to INI: " + std::to_string(value));
		return ERROR_SUCCESS;
	}

	if (managedType == ManagedValueType::String && (dwType == REG_SZ || dwType == REG_EXPAND_SZ)) {
		std::string value;
		if (!TryReadAnsiString(lpData, cbData, value)) {
			return ERROR_INVALID_PARAMETER;
		}
		g_managedOverrides[valueName] = value;
		SaveManagedKeys(g_managedOverrides);
		LogIniManaged("RegSetValueExA", valueName, "wrote REG_SZ string to INI: " + value);
		return ERROR_SUCCESS;
	}

	LogIniManagedWarning("RegSetValueExA", valueName, "key type mismatch when writing to INI");
	return ERROR_SUCCESS;
}

} // namespace RegistryManagedKeys
