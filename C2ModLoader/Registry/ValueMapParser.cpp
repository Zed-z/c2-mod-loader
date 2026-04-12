#include "ValueMapParser.h"

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <windows.h>

namespace {

const char keySeparator = '|';
const char keyValueSeparator = '=';

} // namespace

namespace RegistryValueMapParser {

std::unordered_map<std::string, std::string> ParseKeyValueMap(const std::string &serialized) {
	std::unordered_map<std::string, std::string> out;
	size_t start = 0;

	while (start <= serialized.size()) {
		size_t end = serialized.find(keySeparator, start);
		if (end == std::string::npos) {
			end = serialized.size();
		}

		const std::string item = serialized.substr(start, end - start);
		const size_t sep = item.find(keyValueSeparator);
		if (sep != std::string::npos) {
			std::string key = item.substr(0, sep);
			std::string value = item.substr(sep + 1);
			if (!key.empty()) {
				out[key] = value;
			}
		}

		if (end == serialized.size()) {
			break;
		}
		start = end + 1;
	}

	return out;
}

std::string SerializeKeyValueMap(const std::unordered_map<std::string, std::string> &values) {
	std::vector<std::string> keys;
	keys.reserve(values.size());
	for (const auto &pair : values) {
		keys.push_back(pair.first);
	}
	std::sort(keys.begin(), keys.end());

	std::string out;
	for (size_t i = 0; i < keys.size(); ++i) {
		if (i > 0) {
			out.push_back(keySeparator);
		}
		out += keys[i];
		out.push_back(keyValueSeparator);
		out += values.at(keys[i]);
	}

	return out;
}

} // namespace RegistryValueMapParser
