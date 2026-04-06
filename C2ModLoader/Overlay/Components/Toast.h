#pragma once

#include "Logs.h"

#include <deque>
#include <string>

extern bool showToastInfo;
extern bool showToastDebug;
extern bool showToastWarning;
extern bool showToastError;

struct Toast {
	std::string message;
	LogSeverity severity;
	float timeRemaining;
};

extern std::deque<Toast> toastQueue;

void ImGuiShowToast(const std::string &message, const LogSeverity &severity, float duration = 2.0f);

void RenderToasts();
