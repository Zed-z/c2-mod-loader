#include "FileOverrides.h"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

std::wstring GetFileOverridePath(const std::wstring &modPath) {
	fs::path overridePath(modPath);
	overridePath.replace_extension();
	return overridePath.wstring();
}

bool GetFileOverrides(const std::wstring &overridePath, std::vector<FileOverrideEntry> &entries, std::string *errorMessage) {
	entries.clear();

	if (overridePath.empty() || !fs::exists(overridePath) || !fs::is_directory(overridePath)) {
		return true;
	}

	try {
		for (const auto &entry : fs::recursive_directory_iterator(overridePath)) {
			FileOverrideEntry overrideEntry;
			overrideEntry.relativePath = fs::relative(entry.path(), overridePath).wstring();
			overrideEntry.isDirectory = fs::is_directory(entry);
			overrideEntry.fileSize = overrideEntry.isDirectory ? 0 : fs::file_size(entry);
			entries.push_back(overrideEntry);
		}

		std::sort(entries.begin(), entries.end(), [](const FileOverrideEntry &a, const FileOverrideEntry &b) {
			if (a.isDirectory != b.isDirectory)
				return a.isDirectory > b.isDirectory;
			return a.relativePath < b.relativePath;
		});
	} catch (const fs::filesystem_error &e) {
		if (errorMessage != nullptr) {
			*errorMessage = e.what();
		}
		entries.clear();
		return false;
	}

	return true;
}