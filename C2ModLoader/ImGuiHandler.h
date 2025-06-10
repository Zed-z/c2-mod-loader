#pragma once
#include <Windows.h>
#include <vector>
#include <string>

#include "imgui.h"

extern ImGuiTextBuffer logBuffer;
extern ImFont* toastFont;
extern bool showGui;
extern bool showLog;

DWORD WINAPI ImGuiInitThread(LPVOID);
void ImGuiDraw();

// Toast notifications
struct Toast {
    std::string message;
    float timeRemaining;
};
extern std::vector<Toast> g_ToastQueue;

void ImGuiShowToast(const std::string& message, float duration = 2.0f);
