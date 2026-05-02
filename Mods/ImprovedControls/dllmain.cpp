#include "ModApi.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <windows.h>

ModApi *api = nullptr;

bool type1Flip = false;
bool type1FlipIsFlipping = false;
int type1FlipTimer = 0;

enum TypeSwitchMode {
	None = 0,
	Manual = 1,
	Automatic = 2
};
std::string typeSwitchModeNames[] = {"None", "Manual", "Automatic"};
TypeSwitchMode typeSwitchMode;

int GetControlScheme() {
	SaveSlot *currentSaveSlot = api->GetCurrentSaveSlot();
	return currentSaveSlot->controlMethod;
}

void SetControlScheme(int scheme) {
	SaveSlot *currentSaveSlot = api->GetCurrentSaveSlot();
	currentSaveSlot->controlMethod = scheme;
}

void __stdcall PhysicsStep() {
	Inputs inputs = api->GetInputs();
	Inputs inputsPressed = api->GetInputsPressed();

	bool controlScheme = (bool)GetControlScheme();

	// Auto control mode
	if (typeSwitchMode == TypeSwitchMode::Manual) {
		bool changeControls = GetAsyncKeyState(VK_CAPITAL) & 1;
		if (changeControls) {
			SetControlScheme(!controlScheme);
			std::string controlSchemeMessage = "Control scheme: " + std::string(ControlSchemeNames[!controlScheme]);
			api->LogInfo(controlSchemeMessage.c_str());
			api->ShowInfoToast(controlSchemeMessage.c_str());
		}
	}

	// Flip pressed
	if (type1Flip) {

		if (!type1FlipIsFlipping) {
			if (inputsPressed.flip) {
				if (controlScheme == CTRL_TYPE_1) {
					SetControlScheme(CTRL_TYPE_2);
					type1FlipIsFlipping = true;
					type1FlipTimer = PHYSICS_FPS;
				}
			}
		} else {
			if (--type1FlipTimer <= 0) {
				SetControlScheme(CTRL_TYPE_1);
				type1FlipIsFlipping = false;
			}
		}
	}
}

void __stdcall toggleType1Flip() {
	type1Flip = !type1Flip;
	api->WriteIniBool(L"Config", L"Type1Flip", type1Flip);
}

MenuActionRegistration __stdcall toggleType1FlipRegistration() {
	static std::string label;
	label = std::string("Type 1 Flip: ") + (type1Flip ? "Enabled" : "Disabled");
	return {label.c_str(), "Do a 180 with Type 1 controls by double pressing Camera / 180.", toggleType1Flip, true};
}

void __stdcall toggleTypeSwitchMode() {
	switch (typeSwitchMode) {
	case TypeSwitchMode::None:
		typeSwitchMode = TypeSwitchMode::Manual;
		break;
	case TypeSwitchMode::Manual:
		typeSwitchMode = TypeSwitchMode::Automatic;
		break;
	case TypeSwitchMode::Automatic:
		typeSwitchMode = TypeSwitchMode::None;
		break;
	}
	api->WriteIniInt(L"Config", L"TypeSwitchMode", (int)typeSwitchMode);

	std::string typeSwitchMessage = "Type Switch Mode: " + typeSwitchModeNames[typeSwitchMode];
	api->LogInfo(typeSwitchMessage.c_str());
	api->ShowInfoToast(typeSwitchMessage.c_str());
}

MenuActionRegistration __stdcall toggleTypeSwitchRegistration() {
	static std::string label;
	label = std::string("Type Switch: ") + typeSwitchModeNames[typeSwitchMode];
	return {label.c_str(), "Enable manual [CAPSLOCK] or automatic control type switching.", toggleTypeSwitchMode, true};
}

void __stdcall resetUsingKeyboard() {
	if (typeSwitchMode == TypeSwitchMode::Automatic)
		SetControlScheme(CTRL_TYPE_1);
}

void __stdcall setUsingKeyboard() {
	if (typeSwitchMode == TypeSwitchMode::Automatic)
		SetControlScheme(CTRL_TYPE_2);
}

void PatchAutoModeSwitch() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			0042f00a 89 3d 4c        MOV        dword ptr [analog_strength_0052a54c],EDI
					a5 52 00
		*/
		api->HookFunction(0x42f00a, 6, &resetUsingKeyboard, INJECT_AFTER);
		/*
			0042f0f8 85 f6           TEST       ESI,ESI
			0042f0fa 66 a3 8e        MOV        [DAT_0052a58e],AX
					a5 52 00
			0042f100 74 14           JZ         LAB_0042f116
			0042f102 c7 05 4c        MOV        dword ptr [analog_strength_0052a54c],0xb5
					a5 52 00
					b5 00 00 00
			0042f10c c7 05 60        MOV        dword ptr [DAT_0052a560],0x2
					a5 52 00
					02 00 00 00
		*/
		api->HookFunction(0x42f102, 10, &setUsingKeyboard, INJECT_AFTER);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+2F78A - 89 3D 3C175300        - mov [Croc2.exe+13173C],edi
		*/
		api->HookFunction(0x42F78A, 6, &resetUsingKeyboard, INJECT_AFTER);
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
			Croc2.exe+2F43A - 89 3D 44955200        - mov [Croc2.exe+129544],edi
		*/
		api->HookFunction(0x42F43A, 6, &resetUsingKeyboard, INJECT_AFTER);
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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		api = LoadModApi();
		if (!api)
			return FALSE;

		type1Flip = api->SetupIniBool(L"Config", L"Type1Flip", true);
		typeSwitchMode = (TypeSwitchMode)api->SetupIniInt(L"Config", L"TypeSwitchMode", TypeSwitchMode::Automatic);

		api->RegisterMenuAction(hModule, toggleType1FlipRegistration);
		api->RegisterMenuAction(hModule, toggleTypeSwitchRegistration);

		api->HookGame(GAME_HOOK_PHYSICS, PhysicsStep);

		DisableThreadLibraryCalls(hModule);

		PatchAutoModeSwitch();
	}
	return TRUE;
}
