#include "ConsoleLogging.h"

#include "Logs.h"
#include "Utils.h"

#include <cstdio>
#include <windows.h>

namespace ConsoleLogging {

void WriteWideLogToHandle(HANDLE handle, const std::wstring &text) {
	if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
		return;
	}

	DWORD fileType = GetFileType(handle);
	if (fileType == FILE_TYPE_CHAR) {
		DWORD written = 0;
		WriteConsoleW(handle, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
		return;
	}

	std::string utf8Text = WStringToString(text);
	if (utf8Text.empty()) {
		return;
	}

	DWORD written = 0;
	WriteFile(handle, utf8Text.data(), static_cast<DWORD>(utf8Text.size()), &written, nullptr);
}

void Initialize() {
	if (GetConsoleWindow() != nullptr) {
		return;
	}

	if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
		return;
	}

	FILE *unused = nullptr;
	freopen_s(&unused, "CONOUT$", "w", stdout);
	freopen_s(&unused, "CONOUT$", "w", stderr);
	SetConsoleOutputCP(CP_UTF8);
}

void LogToConsole(const std::wstring &message, LogSeverity severity) {
	HANDLE consoleHandle = GetStdHandle(severity == LogSeverity::Warning || severity == LogSeverity::Error ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
	WriteWideLogToHandle(consoleHandle, message);
}

} // namespace ConsoleLogging
