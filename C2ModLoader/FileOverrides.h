#pragma once

#include <string>
#include <vector>

struct FileOverrideEntry {
	std::wstring relativePath;
	bool isDirectory;
	size_t fileSize;
};

std::wstring GetFileOverridePath(const std::wstring &modPath);
bool GetFileOverrides(const std::wstring &overridePath, std::vector<FileOverrideEntry> &entries, std::string *errorMessage = nullptr);
