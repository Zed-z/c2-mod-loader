#pragma once
#include <Windows.h>

#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib")

LRESULT CALLBACK LauncherWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool ShowLauncherWindow(HINSTANCE hInstance);
