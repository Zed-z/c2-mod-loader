#pragma once
#include <Windows.h>
#include "imgui.h"

extern ImGuiTextBuffer logBuffer;
extern bool showLog;

DWORD WINAPI ImGuiInitThread(LPVOID);
