#pragma once
#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>

#include "Loader.h"
#include "Logs.h"
#include "ModApi.h"
#include "imgui.h"

extern ImFont *uiFont;

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
