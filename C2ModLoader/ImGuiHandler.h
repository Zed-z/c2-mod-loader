#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <deque>

#include "ModApi.h"
#include "imgui.h"
#include "Loader.h"

enum LogSeverity {
    Info,
    Debug,
    Warning,
    Error
};

struct LogMessage {
	std::string text;
	LogSeverity severity;
};

extern std::vector<LogMessage> logMessages;
extern ImFont* uiFont;

extern bool showGui;
extern bool showLog;
extern bool showLogInfo;
extern bool showLogDebug;
extern bool showLogWarning;
extern bool showLogError;
extern bool showToastInfo;
extern bool showToastDebug;
extern bool showToastWarning;
extern bool showToastError;
extern bool showInputs;
extern bool showObjectList;
extern bool showCoords;
extern bool showLevelInfo;
extern bool showSaveSlotList;

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
void RenderCoords();
void RenderMenuBar();
