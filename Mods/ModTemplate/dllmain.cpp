#include "ModApi.h"
#include <iostream>
#include <windows.h>

ModApi *api = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		api = LoadModApi();
		if (!api)
			return FALSE;
		// TODO
	}
	return TRUE;
}
