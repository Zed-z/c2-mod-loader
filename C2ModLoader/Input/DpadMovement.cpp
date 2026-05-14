#include "DpadMovement.h"

#include "Input/Input.h"
#include "Input/TypeSwitching.h"
#include "ModApi.h"

extern ModApi *api;

namespace Input::DpadMovement {

bool dpadMovement;

void __stdcall doDpadMovement() {
	if (!dpadMovement)
		return;

	ModernInput input = Input::GetState();

	if (input.dpad.up)
		*analogY = 8128;
	if (input.dpad.down)
		*analogY = -8128;
	if (input.dpad.left)
		*analogX = 8128;
	if (input.dpad.right)
		*analogX = -8128;

	if (input.dpad.up || input.dpad.down || input.dpad.left || input.dpad.right) {
		*analogStrength = 181;
		*inputDeviceType = 2;
		Input::TypeSwitching::setUsingKeyboard();
	}
}

void PatchDpadMovement() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+2F0D1 - DB 05 64A55200        - fild dword ptr [Croc2.exe+12A564]
		*/
		api->HookFunction(0x42F0D1, 6, &doDpadMovement, INJECT_BEFORE);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+2F851 - DB 05 54175300        - fild dword ptr [Croc2.exe+131754]
		*/
		api->HookFunction(0x42F851, 6, &doDpadMovement, INJECT_BEFORE);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+2F501 - DB 05 5C955200        - fild dword ptr [Croc2.exe+12955C]
		*/
		api->HookFunction(0x42F501, 6, &doDpadMovement, INJECT_BEFORE);
		break;
	}
	}
}

void Setup() {
	dpadMovement = api->SetupIniBool(L"Input", L"DpadMovement", true);

	PatchDpadMovement();
}

} // namespace Input::DpadMovement
