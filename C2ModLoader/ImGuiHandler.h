#pragma once
#include <Windows.h>
#include "imgui.h"

extern ImGuiTextBuffer logBuffer;

DWORD WINAPI ImGuiInitThread(LPVOID);
