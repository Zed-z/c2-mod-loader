#define IS_MOD_LOADER
#include "CheatsManager.h"
#include "Config.h"
#include "ConsoleLogging.h"
#include "Launcher/Launcher.h"
#include "Loader.h"
#include "ModApi.h"
#include "MouseCaptureRemover.h"
#include "Overlay/Backend.h"
#include "Overlay/Overlay.h"
#include "Registry/RegistryManager.h"
#include "Resource.h"
#include "Utils.h"

#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

using std::sin;

ModApi *api;

bool loaderEnabled = true;
bool skipLauncher = false;
bool guiEnabled = true;

static HANDLE g_mainThreadHandle = nullptr;

namespace {

struct FindCtx {
	DWORD pid;
	HWND result;
};

static BOOL CALLBACK FindGameWindowCallback(HWND h, LPARAM lp) {
	auto &c = *reinterpret_cast<FindCtx *>(lp);
	DWORD wpid = 0;
	GetWindowThreadProcessId(h, &wpid);
	if (wpid != c.pid) {
		return TRUE;
	}
	if (!IsWindowVisible(h) || GetWindow(h, GW_OWNER)) {
		return TRUE;
	}
	c.result = h;
	return FALSE;
}

// Keyboard input thread
static DWORD WINAPI HotkeyThread(LPVOID) {
	DWORD pid = GetCurrentProcessId();

	bool prevTab = false;

	while (true) {
		DWORD foregroundPid = 0;
		GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
		bool isForeground = pid == foregroundPid;

		if (isForeground) {

			// Toggle GUI
			bool currTab = GetAsyncKeyState(VK_TAB) & 0x8000;
			if (currTab && !prevTab) {
				showGui = !showGui;
				api->WriteIniInt(L"GUI", L"ShowGui", (int)showGui);
			}
			prevTab = currTab;
		}

		Sleep(20);
	}
	return 0;
}

static DWORD WINAPI WindowTitleCallback(LPVOID param) {
	Sleep(1000);
	std::wstring gameName = L"Croc 2";
	HWND hwnd = FindWindow(NULL, gameName.c_str());
	if (hwnd) {
		std::wstring newTitle = gameName + L" (" + LOADER_NAME_L + L")";
		SetWindowText(hwnd, newTitle.c_str());
	}
	return 0;
}

static DWORD WINAPI CameraPanCallback(LPVOID param) {
	double timer = 0;

	while (true) {
		Sleep(100);

		LevelInfo levelInfo = api->GetLevelInfo();
		if (levelInfo.tribe == 0 && levelInfo.level == 0 && levelInfo.map == 0) {
			StratEntity *camera = api->GetEntity(ADDR_ROOT_OBJ);
			if (camera != nullptr) {
				double camMod = sin(timer);
				camera->newRotPos = {0, (int)(8388608 + camMod * 100000), 0, 54394, -1024, 30854};
			}
		}

		timer += 0.1;
	}
}

static DWORD WINAPI ModLoaderMainThread(LPVOID param) {
	HMODULE hModule = (HMODULE)param;

	if (g_mainThreadHandle) {
		SuspendThread(g_mainThreadHandle);
	}

	ConsoleLogging::Initialize();

	std::string gameVersionMessage = std::string("Game version: ") + GameVersions[api->GetGameVersion()];
	api->LogInfo(gameVersionMessage.c_str());

	SetupDirectories();

	mods = GetMods();

	if (!skipLauncher) {
		bool result = Launcher::ShowLauncherWindow(hModule);
		if (!result) {
			api->LogInfo("Exiting modloader.");
			ExitProcess(0);
			return 0;
		}
	}

	// Reload config after launcher in case settings were changed
	LoadConfig();

	if (loaderEnabled) {
		LoadMods(mods);
	}

	// Window title
	CreateThread(nullptr, 0, WindowTitleCallback, nullptr, 0, nullptr);

	ApiSetup();
	SetupCheats();
	RegistryManager::InstallHooks();

	if (guiEnabled) {
		CreateThread(nullptr, 0, OverlayInitThread, hModule, 0, nullptr);
	}

	if (freeMouse) {
		CreateThread(nullptr, 0, MouseInitThread, nullptr, 0, nullptr);
	}

	CreateThread(nullptr, 0, HotkeyThread, nullptr, 0, nullptr);

	// Camera tilt on main menu
	CreateThread(nullptr, 0, CameraPanCallback, nullptr, 0, nullptr);

	if (g_mainThreadHandle) {
		ResumeThread(g_mainThreadHandle);
		CloseHandle(g_mainThreadHandle);
		g_mainThreadHandle = nullptr;

		// Wait for game window and focus it
		FindCtx ctx{GetCurrentProcessId(), nullptr};
		for (int i = 0; i < 200 && !ctx.result; i++) {
			Sleep(50); // Try for 10 seconds
			EnumWindows(FindGameWindowCallback, reinterpret_cast<LPARAM>(&ctx));
		}

		if (ctx.result) {
			api->LogDebug("Focusing game window.");
			for (int i = 0; i < 20; i++) {
				HWND fg = GetForegroundWindow();
				if (fg == ctx.result) {
					break;
				}

				DWORD gameThread = GetWindowThreadProcessId(ctx.result, nullptr);
				DWORD fgThread = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
				bool attached = false;

				if (fgThread && fgThread != gameThread) {
					attached = AttachThreadInput(gameThread, fgThread, TRUE) != FALSE;
				}

				ShowWindow(ctx.result, SW_SHOW);
				SetWindowPos(ctx.result, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				SetWindowPos(ctx.result, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				BringWindowToTop(ctx.result);
				SetForegroundWindow(ctx.result);
				SetActiveWindow(ctx.result);
				SetFocus(ctx.result);

				if (attached) {
					AttachThreadInput(gameThread, fgThread, FALSE);
				}

				Sleep(25);
			}
		}
	}

	return 0;
}

} // namespace

// Entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: {
		api = GetModApi();
		ClearLog();

		LoadConfig();

		HANDLE duplicatedMainThread = nullptr;
		DuplicateHandle(
			GetCurrentProcess(),
			GetCurrentThread(),
			GetCurrentProcess(),
			&duplicatedMainThread,
			THREAD_SUSPEND_RESUME,
			FALSE,
			0);
		g_mainThreadHandle = duplicatedMainThread;

		DisableThreadLibraryCalls(hModule);

		HANDLE hModLoaderThread = CreateThread(nullptr, 0, ModLoaderMainThread, hModule, 0, nullptr);
		if (!hModLoaderThread && g_mainThreadHandle) {
			CloseHandle(g_mainThreadHandle);
			g_mainThreadHandle = nullptr;
		}
	}
	}
	return TRUE;
}
