#include "IniConfig.h"

#include <fstream>

namespace LauncherIni {

std::vector<ConfigEntry> ParseIniFile(const std::wstring& filePath) {
    std::vector<ConfigEntry> entries;
    std::ifstream file(filePath);
    if (!file.is_open()) return entries;

    std::string currentSection;
    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        if (line[0] == '[' && line[line.length() - 1] == ']') {
            currentSection = line.substr(1, line.length() - 2);
        } else {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                entries.push_back({ currentSection, key, value });
            }
        }
    }
    return entries;
}

void WriteIniFile(const std::wstring& filePath, const std::vector<ConfigEntry>& entries) {
    std::ofstream file(filePath);
    if (!file.is_open()) return;

    std::string lastSection;
    for (const auto& entry : entries) {
        if (entry.section != lastSection) {
            if (!lastSection.empty()) {
                file << "\n";
            }
            file << "[" << entry.section << "]\n";
            lastSection = entry.section;
        }
        file << entry.key << "=" << entry.value << "\n";
    }
}

}
