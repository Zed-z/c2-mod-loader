#include "TypeSwitching.h"

#include <string>

#include "Input/Input.h"
#include "ModApi.h"

extern ModApi *api;

namespace {

int GetControlScheme() {
	SaveSlot *currentSaveSlot = api->GetCurrentSaveSlot();
	return currentSaveSlot->controlMethod;
}

void SetControlScheme(int scheme) {
	SaveSlot *currentSaveSlot = api->GetCurrentSaveSlot();
	currentSaveSlot->controlMethod = scheme;
}

} // namespace

namespace Input::TypeSwitching {

TypeSwitchMode typeSwitchMode;

void __stdcall resetUsingKeyboard() {
	if (typeSwitchMode == TypeSwitchMode::Automatic)
		SetControlScheme(CTRL_TYPE_1);
}

void __stdcall setUsingKeyboard() {
	ModernInput input = Input::GetState();
	if (input.leftStick.x == 0 && input.leftStick.y == 0) {
		if (typeSwitchMode == TypeSwitchMode::Automatic)
			SetControlScheme(CTRL_TYPE_2);
	}
}

void __stdcall AnalogAutoModeSwitch() {
	ModernInput input = Input::GetState();
	if (input.leftStick.x != 0 || input.leftStick.y != 0) {
		resetUsingKeyboard();
	}
}

void PatchKeyboardPressed() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+2F0F8 - 85 F6                 - test esi,esi
			Croc2.exe+2F0FA - 66 A3 8EA55200        - mov [Croc2.exe+12A58E],ax
			Croc2.exe+2F100 - 74 14                 - je Croc2.exe+2F116
			Croc2.exe+2F102 - C7 05 4CA55200 B5000000 - mov [Croc2.exe+12A54C],000000B5
			Croc2.exe+2F10C - C7 05 60A55200 02000000 - mov [Croc2.exe+12A560],00000002
		*/
		api->HookFunction(0x42f102, 10, &setUsingKeyboard, INJECT_AFTER);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+2F878 - 85 F6                 - test esi,esi
			Croc2.exe+2F87A - 66 A3 7E175300        - mov [Croc2.exe+13177E],ax
			Croc2.exe+2F880 - 74 14                 - je Croc2.exe+2F896
			Croc2.exe+2F882 - C7 05 3C175300 B5000000 - mov [Croc2.exe+13173C],000000B5
			Croc2.exe+2F88C - C7 05 50175300 02000000 - mov [Croc2.exe+131750],00000002
		*/
		api->HookFunction(0x42F882, 10, &setUsingKeyboard, INJECT_AFTER);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+2F528 - 85 F6                 - test esi,esi
			Croc2.exe+2F52A - 66 A3 86955200        - mov [Croc2.exe+129586],ax
			Croc2.exe+2F530 - 74 14                 - je Croc2.exe+2F546
			Croc2.exe+2F532 - C7 05 44955200 B5000000 - mov [Croc2.exe+129544],000000B5
			Croc2.exe+2F53C - C7 05 58955200 02000000 - mov [Croc2.exe+129558],00000002
		*/
		api->HookFunction(0x42F532, 10, &setUsingKeyboard, INJECT_AFTER);
		break;
	}
	}
}

void __stdcall ManualModeSwitch() {
	if (typeSwitchMode == TypeSwitchMode::Manual) {
		bool changeControls = GetAsyncKeyState(VK_CAPITAL) & 1;
		if (changeControls) {
			int controlScheme = GetControlScheme();
			SetControlScheme(!controlScheme);
			api->ShowInfoToast(("Control scheme: " + std::string(ControlSchemeNames[!controlScheme])).c_str());
		}
	}
}

void Setup() {
	typeSwitchMode = (TypeSwitchMode)api->SetupIniInt(L"Input", L"TypeSwitchMode", TypeSwitchMode::Automatic);

	PatchKeyboardPressed();
	api->HookGame(GAME_HOOK_PRE_INPUT, &ManualModeSwitch);
	api->HookGame(GAME_HOOK_PRE_INPUT, &AnalogAutoModeSwitch);
}

} // namespace Input::TypeSwitching
