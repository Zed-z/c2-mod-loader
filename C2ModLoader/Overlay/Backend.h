#pragma once

#include <windows.h>

namespace Overlay::Backend {

extern float dpiScale;

DWORD WINAPI OverlayInitThread(LPVOID lpParam);

} // namespace Overlay::Backend
