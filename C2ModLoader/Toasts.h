#pragma once

#include "Logs.h"
#include "ModApi.h"

#include <deque>
#include <string>

namespace Toasts {

struct Toast {
	std::string message;
	LogSeverity severity;
	float timeRemaining;
};

extern std::deque<Toast> toastQueue;

void ShowToast(const std::string &message, const LogSeverity &severity, float duration = 2.0f);

} // namespace Toasts
