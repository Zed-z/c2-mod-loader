#pragma once

#include <string>
#include <unordered_map>
#include <windows.h>

namespace RegistryValueMapParser {

std::unordered_map<std::string, std::string> ParseKeyValueMap(const std::string &serialized);
std::string SerializeKeyValueMap(const std::unordered_map<std::string, std::string> &values);

} // namespace RegistryValueMapParser
