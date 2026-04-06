#pragma once

#include <mutex>
#include <string>
#include <vector>

enum LogSeverity {
	Info,
	Debug,
	Warning,
	Error
};

struct LogMessage {
	std::string text;
	LogSeverity severity;
};

extern std::vector<LogMessage> logMessages;
extern std::mutex logMutex;
