#pragma once
#include <Windows.h>

extern bool freeMouse;
extern bool logHooks;

DWORD WINAPI MouseInitThread(LPVOID lpParam);
