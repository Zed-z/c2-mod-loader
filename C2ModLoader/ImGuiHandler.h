#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <deque>

#include "ModApi.h"
#include "imgui.h"
#include "Loader.h"

extern ImGuiTextBuffer logBuffer;
extern ImFont* toastFont;

extern bool incompatibleWarningShown;

extern bool showGui;
extern bool showLog;
extern bool logMessages;
extern bool logDebug;
extern bool logWarnings;
extern bool logErrors;
extern bool showInputs;

DWORD WINAPI ImGuiInitThread(LPVOID);
void ImGuiDraw();

// Toast notifications
struct Toast {
    std::string message;
    float timeRemaining;
};
extern std::deque<Toast> toastQueue;

void ImGuiShowToast(const std::string& message, float duration = 2.0f);

// Menu actions
struct MenuAction {
	HMODULE handle;
    MenuActionRegistrationFunction function;
};

extern std::vector<MenuAction> menuActionRegistrations;
bool ImGuiRegisterMenuAction(HMODULE handle, MenuActionRegistrationFunction registration);
