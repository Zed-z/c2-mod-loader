#include "DpadMovement.h"

#include "Input/Controls.h"
#include "Input/Input.h"
#include "Input/TypeSwitching.h"
#include "ModApi.h"

extern ModApi *api;

namespace Input::DpadMovement {

void __stdcall doDpadMovement() {
	// Without this, the inputs will not update
	ModernInput input = Input::GetState();

	bool up = Input::getButtonPressed(Input::Controls::config.moveUp) || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveUp);
	bool down = Input::getButtonPressed(Input::Controls::config.moveDown) || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveDown);
	bool left = Input::getButtonPressed(Input::Controls::config.moveLeft) || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveLeft);
	bool right = Input::getButtonPressed(Input::Controls::config.moveRight) || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveRight);

	if (up)
		*analogY = 8128;
	if (down)
		*analogY = -8128;
	if (left)
		*analogX = 8128;
	if (right)
		*analogX = -8128;

	if (up || down || left || right) {
		*analogStrength = 181;
		*inputDeviceType = 2;
		Input::TypeSwitching::setUsingKeyboard();
	}
}

void PatchDpadMovement() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+2F086 - 83 3D 4CA55200 2A     - cmp dword ptr [Analog Strength],2A
			Croc2.exe+2F08D - 0F83 D1010000         - jae Croc2.exe+2F264
			Croc2.exe+2F093 - F6 C3 10              - test bl,10
			Croc2.exe+2F096 - 74 0A                 - je Croc2.exe+2F0A2
			Croc2.exe+2F098 - C7 05 74A55200 C01F0000 - mov [Analog Y],00001FC0
			Croc2.exe+2F0A2 - F6 C3 40              - test bl,40
			Croc2.exe+2F0A5 - 74 0A                 - je Croc2.exe+2F0B1
			Croc2.exe+2F0A7 - C7 05 74A55200 00E0FFFF - mov [Analog Y],FFFFE000
			Croc2.exe+2F0B1 - F6 C3 80              - test bl,-80
			Croc2.exe+2F0B4 - 74 0B                 - je Croc2.exe+2F0C1
			Croc2.exe+2F0B6 - BD C01F0000           - mov ebp,00001FC0
			Croc2.exe+2F0BB - 89 2D 64A55200        - mov [Analog X],ebp
			Croc2.exe+2F0C1 - F6 C3 20              - test bl,20
			Croc2.exe+2F0C4 - 74 0B                 - je Croc2.exe+2F0D1
			Croc2.exe+2F0C6 - BD 00E0FFFF           - mov ebp,FFFFE000
			Croc2.exe+2F0CB - 89 2D 64A55200        - mov [Analog X],ebp
			Croc2.exe+2F0D1 - DB 05 64A55200        - fild dword ptr [Croc2.exe+12A564]
		*/
		api->HookFunction(0x42F093, 62, &doDpadMovement, INJECT_REPLACE);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+2F806 - 83 3D 3C175300 2A     - cmp dword ptr [Croc2.exe+13173C],2A
			Croc2.exe+2F80D - 0F83 D1010000         - jae Croc2.exe+2F9E4
			Croc2.exe+2F813 - F6 C3 10              - test bl,10
			Croc2.exe+2F816 - 74 0A                 - je Croc2.exe+2F822
			Croc2.exe+2F818 - C7 05 64175300 C01F0000 - mov [Croc2.exe+131764],00001FC0
			Croc2.exe+2F822 - F6 C3 40              - test bl,40
			Croc2.exe+2F825 - 74 0A                 - je Croc2.exe+2F831
			Croc2.exe+2F827 - C7 05 64175300 00E0FFFF - mov [Croc2.exe+131764],FFFFE000
			Croc2.exe+2F831 - F6 C3 80              - test bl,-80
			Croc2.exe+2F834 - 74 0B                 - je Croc2.exe+2F841
			Croc2.exe+2F836 - BD C01F0000           - mov ebp,00001FC0
			Croc2.exe+2F83B - 89 2D 54175300        - mov [Croc2.exe+131754],ebp
			Croc2.exe+2F841 - F6 C3 20              - test bl,20
			Croc2.exe+2F844 - 74 0B                 - je Croc2.exe+2F851
			Croc2.exe+2F846 - BD 00E0FFFF           - mov ebp,FFFFE000
			Croc2.exe+2F84B - 89 2D 54175300        - mov [Croc2.exe+131754],ebp
			Croc2.exe+2F851 - DB 05 54175300        - fild dword ptr [Croc2.exe+131754]
		*/
		api->HookFunction(0x42F813, 62, &doDpadMovement, INJECT_REPLACE);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+2F4B6 - 83 3D 44955200 2A     - cmp dword ptr [Croc2.exe+129544],2A
			Croc2.exe+2F4BD - 0F83 D1010000         - jae Croc2.exe+2F694
			Croc2.exe+2F4C3 - F6 C3 10              - test bl,10
			Croc2.exe+2F4C6 - 74 0A                 - je Croc2.exe+2F4D2
			Croc2.exe+2F4C8 - C7 05 6C955200 C01F0000 - mov [Croc2.exe+12956C],00001FC0
			Croc2.exe+2F4D2 - F6 C3 40              - test bl,40
			Croc2.exe+2F4D5 - 74 0A                 - je Croc2.exe+2F4E1
			Croc2.exe+2F4D7 - C7 05 6C955200 00E0FFFF - mov [Croc2.exe+12956C],FFFFE000
			Croc2.exe+2F4E1 - F6 C3 80              - test bl,-80
			Croc2.exe+2F4E4 - 74 0B                 - je Croc2.exe+2F4F1
			Croc2.exe+2F4E6 - BD C01F0000           - mov ebp,00001FC0
			Croc2.exe+2F4EB - 89 2D 5C955200        - mov [Croc2.exe+12955C],ebp
			Croc2.exe+2F4F1 - F6 C3 20              - test bl,20
			Croc2.exe+2F4F4 - 74 0B                 - je Croc2.exe+2F501
			Croc2.exe+2F4F6 - BD 00E0FFFF           - mov ebp,FFFFE000
			Croc2.exe+2F4FB - 89 2D 5C955200        - mov [Croc2.exe+12955C],ebp
			Croc2.exe+2F501 - DB 05 5C955200        - fild dword ptr [Croc2.exe+12955C]
		*/
		api->HookFunction(0x42F4C3, 62, &doDpadMovement, INJECT_REPLACE);
		break;
	}
	}
}

void Setup() {
	PatchDpadMovement();
}

} // namespace Input::DpadMovement
