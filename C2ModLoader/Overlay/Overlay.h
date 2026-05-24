#pragma once

#include <windows.h>

namespace Overlay {

extern bool guiEnabled;
extern bool showWindows;

void Setup(HMODULE hModule);

void Draw();

} // namespace Overlay
