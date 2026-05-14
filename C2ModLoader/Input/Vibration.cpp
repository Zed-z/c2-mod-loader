#include "Vibration.h"

#include <algorithm>
#include <windows.h>
#include <xinput.h>
#pragma comment(lib, "xinput.lib")

#include "Input/Input.h"
#include "ModApi.h"

extern ModApi *api;

using std::min, std::max;

namespace {

struct VibrationParams {
	int strength;
	int durationMs;
};

static DWORD WINAPI vibrationThread(LPVOID param) {
	VibrationParams *vibParams = (VibrationParams *)param;
	int strength = vibParams->strength;
	int durationMs = vibParams->durationMs;
	delete vibParams;

	XINPUT_VIBRATION vibrationEnable{};
	vibrationEnable.wLeftMotorSpeed = strength;
	vibrationEnable.wRightMotorSpeed = strength;
	XInputSetState(Input::deviceIndex, &vibrationEnable);

	Sleep(durationMs);

	XINPUT_VIBRATION vibrationDisable{};
	vibrationDisable.wLeftMotorSpeed = 0;
	vibrationDisable.wRightMotorSpeed = 0;
	XInputSetState(Input::deviceIndex, &vibrationDisable);

	return 0;
}

void vibrate(int strength, int durationMs) {
	VibrationParams *params = new VibrationParams{strength, durationMs};
	HANDLE threadHandle = CreateThread(nullptr, 0, vibrationThread, params, 0, nullptr);
	if (threadHandle != nullptr) {
		CloseHandle(threadHandle);
	} else {
		delete params;
	}
}

} // namespace

namespace Input::Vibration {

bool vibrationEnabled;
float vibrationStrength;

void __stdcall PreDamage() {
	if (vibrationEnabled) {
		int strength = (int)(vibrationStrength * 65535);
		vibrate(strength, 200);
	}
}

void PatchPreDamage() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		// Croc2.exe+81B80 - 83 3D 3C794B00 0B     - cmp dword ptr [Croc2.exe+B793C],0B
		api->HookFunction(0x481B80, 7, &PreDamage, INJECT_BEFORE);
		break;
	}
	case GAMEVER_EU: {
		// Croc2.exe+826D0 - 83 3D 2CEB4B00 0B     - cmp dword ptr [Croc2.exe+BEB2C],0B
		api->HookFunction(0x4826D0, 7, &PreDamage, INJECT_BEFORE);
		break;
	}
	case GAMEVER_DEMO: {
		// Croc2.exe+81010 - 83 3D 3C694B00 0B     - cmp dword ptr [Croc2.exe+B693C],0B
		api->HookFunction(0x481010, 7, &PreDamage, INJECT_BEFORE);
		break;
	}
	}
}

void Setup() {
	vibrationEnabled = api->SetupIniBool(L"Input", L"VibrationEnabled", true);
	vibrationStrength = api->SetupIniInt(L"Input", L"VibrationStrength", 100) / 100.0f;
	vibrationStrength = min(max(vibrationStrength, 0.0f), 1.0f);

	PatchPreDamage();
}

} // namespace Input::Vibration
