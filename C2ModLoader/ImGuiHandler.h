#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <deque>

#include "ModApi.h"
#include "imgui.h"
#include "Loader.h"

extern std::vector<LogMessage> logMessages;
extern ImFont* toastFont;

extern bool incompatibleWarningShown;

extern bool showGui;
extern bool showLog;
extern bool logInfo;
extern bool logDebug;
extern bool logWarnings;
extern bool logErrors;
extern bool showInputs;
extern bool showObjectList;

DWORD WINAPI ImGuiInitThread(LPVOID);
void ImGuiDraw();

// Toast notifications
struct Toast {
    std::string message;
    LogSeverity severity;
    float timeRemaining;
};
extern std::deque<Toast> toastQueue;

void ImGuiShowToast(const std::string& message, const LogSeverity& severity, float duration = 2.0f);

// Menu actions
struct MenuAction {
	HMODULE handle;
    MenuActionRegistrationFunction function;
};

extern std::vector<MenuAction> menuActionRegistrations;
bool ImGuiRegisterMenuAction(HMODULE handle, MenuActionRegistrationFunction registration);

void RenderToasts();
void RenderInputs();
void RenderLog();
void RenderObjectList();
void RenderMenuBar();
