#include "ModApi.h"
#include <iostream>
#include <sstream>
#include <windows.h>

ModApi *api = nullptr;
uint32_t base_fog_distance;
uint32_t base_render_distance;

bool is_active;
#define RENDER_MULTIPLIER_DEFAULT 3
#define RENDER_MULTIPLER_LIMIT 10
int render_multiplier = RENDER_MULTIPLIER_DEFAULT;

void RenderApply() {
	int multiplier = is_active ? render_multiplier : 1;
	uint32_t fogOffset = base_render_distance - base_fog_distance;
	*fogDistance = base_render_distance * multiplier - fogOffset;
	*renderDistance = base_render_distance * multiplier;
}

void __stdcall toggleExtender() {
	is_active = !is_active;
	api->WriteIniInt(L"Config", L"Enabled", is_active);
	RenderApply();

	if (is_active) {
		api->LogInfo("Render distance extended!");
		std::string toastMessage = "Render distance extended! (x" + std::to_string(render_multiplier) + ")";
		api->ShowInfoToast(toastMessage.c_str());
	} else {
		api->LogInfo("Render distance reverted!");
		api->ShowInfoToast("Render distance reverted!");
	}
}

void __stdcall decreaseExtender() {
	if (is_active && render_multiplier > 1) {
		render_multiplier--;
		RenderApply();
		api->WriteIniInt(L"Config", L"RenderMultiplier", render_multiplier);
		std::string logMessage = "Render distance decreased to: x" + std::to_string(render_multiplier);
		std::string toastMessage = "Render distance: x" + std::to_string(render_multiplier);
		api->LogInfo(logMessage.c_str());
		api->ShowInfoToast(toastMessage.c_str());
	}
}

void __stdcall increaseExtender() {
	if (is_active && render_multiplier < RENDER_MULTIPLER_LIMIT) {
		render_multiplier++;
		RenderApply();
		api->WriteIniInt(L"Config", L"RenderMultiplier", render_multiplier);
		std::string logMessage = "Render distance increased to: x" + std::to_string(render_multiplier);
		std::string toastMessage = "Render distance: x" + std::to_string(render_multiplier);
		api->LogInfo(logMessage.c_str());
		api->ShowInfoToast(toastMessage.c_str());
	}
}

static DWORD WINAPI HotkeyThread(LPVOID) {
	DWORD pid = GetCurrentProcessId();

	bool prevF1 = false;
	bool prevMinus = false;
	bool prevPlus = false;

	while (true) {
		DWORD foregroundPid = 0;
		GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
		bool isForeground = pid == foregroundPid;

		if (isForeground) {

			// Toggle extender
			bool currF1 = GetAsyncKeyState(VK_F1) & 0x8000;
			if (currF1 && !prevF1)
				toggleExtender();
			prevF1 = currF1;

			// Decrease distance
			bool currMinus = GetAsyncKeyState(VK_OEM_MINUS) & 0x8000;
			if (currMinus && !prevMinus)
				decreaseExtender();
			prevMinus = currMinus;

			// Increase distance
			bool currPlus = GetAsyncKeyState(VK_OEM_PLUS) & 0x8000;
			if (currPlus && !prevPlus)
				increaseExtender();
			prevPlus = currPlus;
		}

		Sleep(20);
	}
	return 0;
}

void __stdcall OnDistancesWritten() {
	base_fog_distance = *fogDistance;
	base_render_distance = *renderDistance;
	api->LogDebug(("Render distance changed to: " + std::to_string(base_render_distance)).c_str());
	RenderApply();
}

void __stdcall OnDoorChange() {
	api->LogDebug(("Door fog distance changed to: " + std::to_string((*doorStruct)->fogDistance)).c_str());
	api->LogDebug(("Door render distance changed to: " + std::to_string((*doorStruct)->renderDistance)).c_str());
	(*doorStruct)->fogDistance = INT_MAX;
	(*doorStruct)->renderDistance = INT_MAX;
}

void hookDistanceChanges() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+23D04 - 89 15 487B4B00        - mov [Croc2.exe+B7B48],edx
			Croc2.exe+23D0A - 8B 40 40              - mov eax,[eax+40]
			Croc2.exe+23D0D - A3 187B4B00           - mov [Croc2.exe+B7B18],eax
		*/
		api->HookFunction(0x423D0D, 5, &OnDistancesWritten, INJECT_AFTER);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+243A4 - 89 15 38ED4B00        - mov [Croc2.exe+BED38],edx
			Croc2.exe+243AA - 8B 40 40              - mov eax,[eax+40]
			Croc2.exe+243AD - A3 08ED4B00           - mov [Croc2.exe+BED08],eax
		*/
		api->HookFunction(0x4243AD, 5, &OnDistancesWritten, INJECT_AFTER);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+24134 - 89 15 486B4B00        - mov [Croc2.exe+B6B48],edx
			Croc2.exe+2413A - 8B 40 40              - mov eax,[eax+40]
			Croc2.exe+2413D - A3 186B4B00           - mov [Croc2.exe+B6B18],eax
		*/
		api->HookFunction(0x42413D, 5, &OnDistancesWritten, INJECT_AFTER);
		break;
	}
	}

	// Commented out due to issues
	// api->HookGame(GAME_HOOK_DOOR_CHANGE, OnDoorChange);
}

MenuActionRegistration __stdcall toggleExtenderRegistration() {
	return {is_active ? "Disable Extender" : "Enable Extender", is_active ? "Disable the extender." : "Enable the extender.", toggleExtender, true};
}

MenuActionRegistration __stdcall decreaseExtenderRegistration() {
	static std::string tooltip;
	tooltip = "Decrease render distance (currently: x" + std::to_string(render_multiplier) + ")";
	return {"Decrease Distance", tooltip.c_str(), decreaseExtender, is_active && render_multiplier > 1};
}

MenuActionRegistration __stdcall increaseExtenderRegistration() {
	static std::string tooltip;
	tooltip = "Increase render distance (currently: x" + std::to_string(render_multiplier) + ")";
	return {"Increase Distance", tooltip.c_str(), increaseExtender, is_active && render_multiplier < RENDER_MULTIPLER_LIMIT};
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		api = LoadModApi();
		if (!api)
			return FALSE;

		// Hook function
		hookDistanceChanges();

		is_active = api->SetupIniBool(L"Config", L"Enabled", true);
		render_multiplier = api->SetupIniInt(L"Config", L"RenderMultiplier", RENDER_MULTIPLIER_DEFAULT);

		// Register menu actions
		api->RegisterMenuAction(hModule, toggleExtenderRegistration);
		api->RegisterMenuAction(hModule, decreaseExtenderRegistration);
		api->RegisterMenuAction(hModule, increaseExtenderRegistration);

		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, HotkeyThread, nullptr, 0, nullptr);
	}
	return TRUE;
}
