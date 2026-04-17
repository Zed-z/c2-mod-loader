#pragma once

#include "Logs.h"

#include <string>

namespace ConsoleLogging {

void Initialize();
void LogToConsole(const std::wstring &message, LogSeverity severity);

} // namespace ConsoleLogging
