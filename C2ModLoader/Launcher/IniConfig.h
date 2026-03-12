#pragma once

#include <string>
#include <vector>

namespace LauncherIni {

struct ConfigEntry {
    std::string section;
    std::string key;
    std::string value;
};

std::vector<ConfigEntry> ParseIniFile(const std::wstring& filePath);
void WriteIniFile(const std::wstring& filePath, const std::vector<ConfigEntry>& entries);

}
