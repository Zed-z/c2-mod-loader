#include "Logs.h"

#include <mutex>
#include <string>
#include <vector>

std::vector<LogMessage> logMessages;
std::mutex logMutex;