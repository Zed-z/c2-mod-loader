#include "Toasts.h"

namespace Toasts {

std::deque<Toast> toastQueue;

void ShowToast(const std::string &message, const LogSeverity &severity, float duration) {
	toastQueue.push_front({message, severity, duration});
}

} // namespace Toasts
