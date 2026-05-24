#pragma once

namespace Overlay::Log {

extern bool showLog;
extern bool showLogInfo;
extern bool showLogDebug;
extern bool showLogWarning;
extern bool showLogError;

void Setup();

void RenderLog();

} // namespace Overlay::Log
