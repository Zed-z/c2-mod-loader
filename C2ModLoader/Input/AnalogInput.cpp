#include "AnalogInput.h"

#include "Input/Controls.h"
#include "Input/Input.h"
#include "ModApi.h"

extern ModApi *api;

namespace {

void PatchAnalogInput() {
	uintptr_t ptrX, ptrY;
	switch (Input::Controls::config.movement) {
	case Input::Controls::ControlAnalog::LeftStick:
		ptrX = (uintptr_t)&(Input::controllerBackend->input.leftStick.x);
		ptrY = (uintptr_t)&(Input::controllerBackend->input.leftStick.y);
		break;
	case Input::Controls::ControlAnalog::RightStick:
		ptrX = (uintptr_t)&(Input::controllerBackend->input.rightStick.x);
		ptrY = (uintptr_t)&(Input::controllerBackend->input.rightStick.y);
		break;
	}

	uint8_t hookCode[12];
	int p = 0;

	// mov ebp, [ptrX]
	hookCode[p++] = 0x8B;
	hookCode[p++] = 0x2D;
	*(uintptr_t *)(hookCode + p) = ptrX;
	p += 4;

	// mov edx, [ptrY]
	hookCode[p++] = 0x8B;
	hookCode[p++] = 0x15;
	*(uintptr_t *)(hookCode + p) = ptrY;
	p += 4;

	/*
		Inject before these lines, to replace analog input after
		it's polled from directinput, but before further processing
	*/
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+2F1CA - 83 FD 7F              - cmp ebp,7F
			Croc2.exe+2F1CD - 89 15 74A55200        - mov [Croc2.exe+12A574],edx
		*/
		api->InjectCode(0x42F1CA, 9, hookCode, p, INJECT_BEFORE);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+2F94A - 83 FD 7F              - cmp ebp,7F
			Croc2.exe+2F94D - 89 15 64175300        - mov [Croc2.exe+131764],edx
		*/
		api->InjectCode(0x42F94A, 9, hookCode, p, INJECT_BEFORE);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+2F5FA - 83 FD 7F              - cmp ebp,7F
			Croc2.exe+2F5FD - 89 15 6C955200        - mov [Croc2.exe+12956C],edx
		*/
		api->InjectCode(0x42F5FA, 9, hookCode, p, INJECT_BEFORE);
		break;
	}
	}
}

} // namespace

namespace Input::AnalogInput {

void Setup() {
	PatchAnalogInput();
}

} // namespace Input::AnalogInput
