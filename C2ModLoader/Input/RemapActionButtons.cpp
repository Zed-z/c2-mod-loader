#include "RemapActionButtons.h"

#include "Input/Controls.h"
#include "Input/Input.h"
#include "ModApi.h"

extern ModApi *api;

#define DIRECTIONAL_ANALOG_THRESHOLD 64

namespace {

uint32_t input_builder = 0;
void __stdcall remapDecodePad() {
	input_builder = 0;

	if (Input::getButtonPressed(Input::Controls::config.moveUp) || Input::getAnalogStickY(Input::Controls::config.movement) > DIRECTIONAL_ANALOG_THRESHOLD || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveUp)) {
		*realPad |= INPUT_UP;
		*playerPad |= INPUT_UP;
		input_builder |= INPUT_UP;
	}
	if (Input::getButtonPressed(Input::Controls::config.moveRight) || Input::getAnalogStickX(Input::Controls::config.movement) < -DIRECTIONAL_ANALOG_THRESHOLD || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveRight)) {
		*realPad |= INPUT_RIGHT;
		*playerPad |= INPUT_RIGHT;
		input_builder |= INPUT_RIGHT;
	}
	if (Input::getButtonPressed(Input::Controls::config.moveDown) || Input::getAnalogStickY(Input::Controls::config.movement) < -DIRECTIONAL_ANALOG_THRESHOLD || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveDown)) {
		*realPad |= INPUT_DOWN;
		*playerPad |= INPUT_DOWN;
		input_builder |= INPUT_DOWN;
	}
	if (Input::getButtonPressed(Input::Controls::config.moveLeft) || Input::getAnalogStickX(Input::Controls::config.movement) > DIRECTIONAL_ANALOG_THRESHOLD || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveLeft)) {
		*realPad |= INPUT_LEFT;
		*playerPad |= INPUT_LEFT;
		input_builder |= INPUT_LEFT;
	}

	if (Input::getButtonPressed(Input::Controls::config.pause) || Input::getKeyboardKeyPressed(Input::Controls::config.keyPause))
		input_builder |= INPUT_PAUSE;

	if (Input::getButtonPressed(Input::Controls::config.jump) || Input::getKeyboardKeyPressed(Input::Controls::config.keyJump))
		input_builder |= INPUT_JUMP;
	if (Input::getButtonPressed(Input::Controls::config.attack) || Input::getKeyboardKeyPressed(Input::Controls::config.keyAttack))
		input_builder |= INPUT_ATTACK;

	if (Input::getButtonPressed(Input::Controls::config.stepLeft) || Input::getKeyboardKeyPressed(Input::Controls::config.keyStepLeft))
		input_builder |= INPUT_STEP_LEFT;
	if (Input::getButtonPressed(Input::Controls::config.stepRight) || Input::getKeyboardKeyPressed(Input::Controls::config.keyStepRight))
		input_builder |= INPUT_STEP_RIGHT;
	if (Input::getButtonPressed(Input::Controls::config.cameraFlip) || Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraFlip))
		input_builder |= INPUT_FLIP;

	if (Input::getButtonPressed(Input::Controls::config.itemPrev) || Input::getKeyboardKeyPressed(Input::Controls::config.keyItemPrev))
		input_builder |= INPUT_INV_LEFT;
	if (Input::getButtonPressed(Input::Controls::config.itemNext) || Input::getKeyboardKeyPressed(Input::Controls::config.keyItemNext))
		input_builder |= INPUT_INV_RIGHT;
	if (Input::getButtonPressed(Input::Controls::config.itemUse) || Input::getKeyboardKeyPressed(Input::Controls::config.keyItemUse))
		input_builder |= INPUT_INV_USE;
}

void patchDecodePad() {
	uintptr_t inputPtr = (uintptr_t)&(input_builder);
	uint8_t hookCode[16];

	// mov EDI, [inputs_raw_builder]
	int p = 0;
	hookCode[p++] = 0x8B;
	hookCode[p++] = 0x3D;
	*(uintptr_t *)(hookCode + p) = inputPtr;
	p += 4;

	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+2F26B - F6 C1 02              - test cl,02
			Croc2.exe+2F26E - 74 0A                 - je Croc2.exe+2F27A
			Croc2.exe+2F270 - 8B D3                 - mov edx,ebx
			Croc2.exe+2F272 - 81 E2 00800000        - and edx,00008000
			Croc2.exe+2F278 - 0B FA                 - or edi,edx
			Croc2.exe+2F27A - F6 C1 04              - test cl,04
			Croc2.exe+2F27D - 74 0A                 - je Croc2.exe+2F289
			Croc2.exe+2F27F - 8B D3                 - mov edx,ebx
			Croc2.exe+2F281 - 81 E2 00400000        - and edx,00004000
			Croc2.exe+2F287 - 0B FA                 - or edi,edx
			Croc2.exe+2F289 - F6 C1 08              - test cl,08
			Croc2.exe+2F28C - 74 0A                 - je Croc2.exe+2F298
			Croc2.exe+2F28E - 8B D3                 - mov edx,ebx
			Croc2.exe+2F290 - 81 E2 000C0000        - and edx,00000C00
			Croc2.exe+2F296 - 0B FA                 - or edi,edx
			Croc2.exe+2F298 - F6 C1 10              - test cl,10
			Croc2.exe+2F29B - 74 0A                 - je Croc2.exe+2F2A7
			Croc2.exe+2F29D - 8B D3                 - mov edx,ebx
			Croc2.exe+2F29F - 81 E2 00200000        - and edx,00002000
			Croc2.exe+2F2A5 - 0B FA                 - or edi,edx
			Croc2.exe+2F2A7 - F6 C1 20              - test cl,20
			Croc2.exe+2F2AA - 74 08                 - je Croc2.exe+2F2B4
			Croc2.exe+2F2AC - 81 E3 00130000        - and ebx,00001300
			Croc2.exe+2F2B2 - 0B FB                 - or edi,ebx
		*/
		api->HookFunction(0x42f26b, 60, &remapDecodePad, INJECT_REPLACE);
		api->InjectCode(0x42f2a7, 13, hookCode, p, INJECT_REPLACE);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+2F9EB - F6 C1 02              - test cl,02
			Croc2.exe+2F9EE - 74 0A                 - je Croc2.exe+2F9FA
			Croc2.exe+2F9F0 - 8B D3                 - mov edx,ebx
			Croc2.exe+2F9F2 - 81 E2 00800000        - and edx,00008000
			Croc2.exe+2F9F8 - 0B FA                 - or edi,edx
			Croc2.exe+2F9FA - F6 C1 04              - test cl,04
			Croc2.exe+2F9FD - 74 0A                 - je Croc2.exe+2FA09
			Croc2.exe+2F9FF - 8B D3                 - mov edx,ebx
			Croc2.exe+2FA01 - 81 E2 00400000        - and edx,00004000
			Croc2.exe+2FA07 - 0B FA                 - or edi,edx
			Croc2.exe+2FA09 - F6 C1 08              - test cl,08
			Croc2.exe+2FA0C - 74 0A                 - je Croc2.exe+2FA18
			Croc2.exe+2FA0E - 8B D3                 - mov edx,ebx
			Croc2.exe+2FA10 - 81 E2 000C0000        - and edx,00000C00
			Croc2.exe+2FA16 - 0B FA                 - or edi,edx
			Croc2.exe+2FA18 - F6 C1 10              - test cl,10
			Croc2.exe+2FA1B - 74 0A                 - je Croc2.exe+2FA27
			Croc2.exe+2FA1D - 8B D3                 - mov edx,ebx
			Croc2.exe+2FA1F - 81 E2 00200000        - and edx,00002000
			Croc2.exe+2FA25 - 0B FA                 - or edi,edx
			Croc2.exe+2FA27 - F6 C1 20              - test cl,20
			Croc2.exe+2FA2A - 74 08                 - je Croc2.exe+2FA34
			Croc2.exe+2FA2C - 81 E3 00130000        - and ebx,00001300
			Croc2.exe+2FA32 - 0B FB                 - or edi,ebx
		*/
		api->HookFunction(0x42F9EB, 60, &remapDecodePad, INJECT_REPLACE);
		api->InjectCode(0x42FA27, 13, hookCode, p, INJECT_REPLACE);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+2F69B - F6 C1 02              - test cl,02
			Croc2.exe+2F69E - 74 0A                 - je Croc2.exe+2F6AA
			Croc2.exe+2F6A0 - 8B D3                 - mov edx,ebx
			Croc2.exe+2F6A2 - 81 E2 00800000        - and edx,00008000
			Croc2.exe+2F6A8 - 0B FA                 - or edi,edx
			Croc2.exe+2F6AA - F6 C1 04              - test cl,04
			Croc2.exe+2F6AD - 74 0A                 - je Croc2.exe+2F6B9
			Croc2.exe+2F6AF - 8B D3                 - mov edx,ebx
			Croc2.exe+2F6B1 - 81 E2 00400000        - and edx,00004000
			Croc2.exe+2F6B7 - 0B FA                 - or edi,edx
			Croc2.exe+2F6B9 - F6 C1 08              - test cl,08
			Croc2.exe+2F6BC - 74 0A                 - je Croc2.exe+2F6C8
			Croc2.exe+2F6BE - 8B D3                 - mov edx,ebx
			Croc2.exe+2F6C0 - 81 E2 000C0000        - and edx,00000C00
			Croc2.exe+2F6C6 - 0B FA                 - or edi,edx
			Croc2.exe+2F6C8 - F6 C1 10              - test cl,10
			Croc2.exe+2F6CB - 74 0A                 - je Croc2.exe+2F6D7
			Croc2.exe+2F6CD - 8B D3                 - mov edx,ebx
			Croc2.exe+2F6CF - 81 E2 00200000        - and edx,00002000
			Croc2.exe+2F6D5 - 0B FA                 - or edi,edx
			Croc2.exe+2F6D7 - F6 C1 20              - test cl,20
			Croc2.exe+2F6DA - 74 08                 - je Croc2.exe+2F6E4
			Croc2.exe+2F6DC - 81 E3 00130000        - and ebx,00001300
			Croc2.exe+2F6E2 - 0B FB                 - or edi,ebx
		*/
		api->HookFunction(0x42F69B, 60, &remapDecodePad, INJECT_REPLACE);
		api->InjectCode(0x42F6D7, 13, hookCode, p, INJECT_REPLACE);
		break;
	}
	}
}

void __stdcall remapUpdatePadForMenu() {
	Input::PollInput();
	ModernInput input = Input::GetState();

	uint32_t current_pad = 0;
	if (Input::getButtonPressed(Input::Controls::config.moveUp) || Input::getAnalogStickY(Input::Controls::config.movement) > DIRECTIONAL_ANALOG_THRESHOLD || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveUp))
		current_pad |= INPUT_UP;
	if (Input::getButtonPressed(Input::Controls::config.moveRight) || Input::getAnalogStickX(Input::Controls::config.movement) < -DIRECTIONAL_ANALOG_THRESHOLD || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveRight))
		current_pad |= INPUT_RIGHT;
	if (Input::getButtonPressed(Input::Controls::config.moveDown) || Input::getAnalogStickY(Input::Controls::config.movement) < -DIRECTIONAL_ANALOG_THRESHOLD || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveDown))
		current_pad |= INPUT_DOWN;
	if (Input::getButtonPressed(Input::Controls::config.moveLeft) || Input::getAnalogStickX(Input::Controls::config.movement) > DIRECTIONAL_ANALOG_THRESHOLD || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveLeft))
		current_pad |= INPUT_LEFT;
	if (Input::getButtonPressed(Input::Controls::config.jump) || Input::getKeyboardKeyPressed(Input::Controls::config.keyJump))
		current_pad |= INPUT_JUMP;
	if (Input::getButtonPressed(Input::Controls::config.attack) || Input::getKeyboardKeyPressed(Input::Controls::config.keyAttack))
		current_pad |= INPUT_ATTACK;
	if (Input::getButtonPressed(Input::Controls::config.cameraFlip) || Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraFlip))
		current_pad |= INPUT_FLIP;
	if (Input::getButtonPressed(Input::Controls::config.itemUse) || Input::getKeyboardKeyPressed(Input::Controls::config.keyItemUse))
		current_pad |= INPUT_INV_USE;
	if (Input::getButtonPressed(Input::Controls::config.stepLeft) || Input::getKeyboardKeyPressed(Input::Controls::config.keyStepLeft))
		current_pad |= INPUT_STEP_LEFT;
	if (Input::getButtonPressed(Input::Controls::config.stepRight) || Input::getKeyboardKeyPressed(Input::Controls::config.keyStepRight))
		current_pad |= INPUT_STEP_RIGHT;
	if (Input::getButtonPressed(Input::Controls::config.itemPrev) || Input::getKeyboardKeyPressed(Input::Controls::config.keyItemPrev))
		current_pad |= INPUT_INV_LEFT;
	if (Input::getButtonPressed(Input::Controls::config.itemNext) || Input::getKeyboardKeyPressed(Input::Controls::config.keyItemNext))
		current_pad |= INPUT_INV_RIGHT;
	if (Input::getButtonPressed(Input::Controls::config.pause) || Input::getKeyboardKeyPressed(Input::Controls::config.keyPause))
		current_pad |= INPUT_PAUSE;

	*playerPadPush = ~*_prevPlayerPadState & current_pad;
	*realPadPush = ~*_prevRealPadState & current_pad;

	*_prevPlayerPadState = current_pad;
	*_prevRealPadState = current_pad;

	// Menu specific logic
	if (*gameState == GS_MAIN_MENU) {
		if ((current_pad & INPUT_INV_LEFT) != 0) {
			*realPadPush &= ~INPUT_INV_LEFT;
			if (*realPadPush != 0) {
				_InputMenuCallback(0);
			}
		}
		if ((current_pad & INPUT_INV_RIGHT) != 0) {
			*realPadPush &= ~INPUT_INV_RIGHT;
			if (*realPadPush != 0) {
				_InputMenuCallback(1);
			}
		}
	}

	// Force use inventory
	uint32_t final_pad = current_pad & ~INPUT_INV_USE;
	if (*_invUseOverride > 0) {
		final_pad |= INPUT_INV_USE;
	}

	uint32_t old_pad = *playerPadOld;
	*playerPadOld = final_pad;
	*realPad = final_pad;
	*playerPadPush = ~old_pad & final_pad;
	*playerPadPull = ~final_pad & old_pad;
	*realPadPush = ~*realPadOld & final_pad;
	*realPadOld = final_pad;
	*playerPad = final_pad;
}

void patchUpdatePadForMenu() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+1ACF0 - 8B 0D 0C664B00        - mov ecx,[Croc2.exe+B660C]
			Croc2.exe+1ACF6 - 33 C0                 - xor eax,eax
			Croc2.exe+1ACF8 - 85 C9                 - test ecx,ecx
			Croc2.exe+1ACFA - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1ACFF - 7E 0A                 - jle Croc2.exe+1AD0B
			Croc2.exe+1AD01 - B8 10000000           - mov eax,00000010
			Croc2.exe+1AD06 - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AD0B - 8B 0D 2C664B00        - mov ecx,[Croc2.exe+B662C]
			Croc2.exe+1AD11 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD13 - 7E 07                 - jle Croc2.exe+1AD1C
			Croc2.exe+1AD15 - 0C 40                 - or al,40
			Croc2.exe+1AD17 - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AD1C - 8B 0D 18664B00        - mov ecx,[Croc2.exe+B6618]
			Croc2.exe+1AD22 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD24 - 7E 07                 - jle Croc2.exe+1AD2D
			Croc2.exe+1AD26 - 0C 80                 - or al,-80
			Croc2.exe+1AD28 - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AD2D - 8B 0D 20664B00        - mov ecx,[Croc2.exe+B6620]
			Croc2.exe+1AD33 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD35 - 7E 07                 - jle Croc2.exe+1AD3E
			Croc2.exe+1AD37 - 0C 20                 - or al,20
			Croc2.exe+1AD39 - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AD3E - 8B 0D 5C634B00        - mov ecx,[Croc2.exe+B635C]
			Croc2.exe+1AD44 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD46 - 7E 08                 - jle Croc2.exe+1AD50
			Croc2.exe+1AD48 - 80 CC 40              - or ah,40
			Croc2.exe+1AD4B - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AD50 - 8B 0D D0634B00        - mov ecx,[Croc2.exe+B63D0]
			Croc2.exe+1AD56 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD58 - 7E 08                 - jle Croc2.exe+1AD62
			Croc2.exe+1AD5A - 80 CC 80              - or ah,-80
			Croc2.exe+1AD5D - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AD62 - 8B 0D A4634B00        - mov ecx,[Croc2.exe+B63A4]
			Croc2.exe+1AD68 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD6A - 7E 08                 - jle Croc2.exe+1AD74
			Croc2.exe+1AD6C - 80 CC 20              - or ah,20
			Croc2.exe+1AD6F - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AD74 - 8B 0D 28634B00        - mov ecx,[Croc2.exe+B6328]
			Croc2.exe+1AD7A - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD7C - 7E 07                 - jle Croc2.exe+1AD85
			Croc2.exe+1AD7E - 0C 08                 - or al,08
			Croc2.exe+1AD80 - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AD85 - 8B 0D 98634B00        - mov ecx,[Croc2.exe+B6398]
			Croc2.exe+1AD8B - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD8D - 7E 07                 - jle Croc2.exe+1AD96
			Croc2.exe+1AD8F - 0C 01                 - or al,01
			Croc2.exe+1AD91 - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AD96 - 8B 0D 64634B00        - mov ecx,[Croc2.exe+B6364]
			Croc2.exe+1AD9C - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD9E - 7E 08                 - jle Croc2.exe+1ADA8
			Croc2.exe+1ADA0 - 80 CC 04              - or ah,04
			Croc2.exe+1ADA3 - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1ADA8 - 8B 0D 68634B00        - mov ecx,[Croc2.exe+B6368]
			Croc2.exe+1ADAE - 85 C9                 - test ecx,ecx
			Croc2.exe+1ADB0 - 7E 08                 - jle Croc2.exe+1ADBA
			Croc2.exe+1ADB2 - 80 CC 08              - or ah,08
			Croc2.exe+1ADB5 - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1ADBA - 8B 0D 3C634B00        - mov ecx,[Croc2.exe+B633C]
			Croc2.exe+1ADC0 - 85 C9                 - test ecx,ecx
			Croc2.exe+1ADC2 - 7E 08                 - jle Croc2.exe+1ADCC
			Croc2.exe+1ADC4 - 80 CC 10              - or ah,10
			Croc2.exe+1ADC7 - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1ADCC - 8B 0D B47A4B00        - mov ecx,[Croc2.exe+B7AB4]
			Croc2.exe+1ADD2 - 8B 15 3C794B00        - mov edx,[Croc2.exe+B793C]
			Croc2.exe+1ADD8 - F7 D1                 - not ecx
			Croc2.exe+1ADDA - 23 C8                 - and ecx,eax
			Croc2.exe+1ADDC - A3 B47A4B00           - mov [Croc2.exe+B7AB4],eax
			Croc2.exe+1ADE1 - 89 0D 78A55200        - mov [Croc2.exe+12A578],ecx
			Croc2.exe+1ADE7 - 8B 0D B87A4B00        - mov ecx,[Croc2.exe+B7AB8]
			Croc2.exe+1ADED - F7 D1                 - not ecx
			Croc2.exe+1ADEF - 23 C8                 - and ecx,eax
			Croc2.exe+1ADF1 - 83 FA 10              - cmp edx,10
			Croc2.exe+1ADF4 - A3 88A55200           - mov [Croc2.exe+12A588],eax
			Croc2.exe+1ADF9 - A3 B87A4B00           - mov [Croc2.exe+B7AB8],eax
			Croc2.exe+1ADFE - 75 50                 - jne Croc2.exe+1AE50
			Croc2.exe+1AE00 - F6 C4 04              - test ah,04
			Croc2.exe+1AE03 - 74 23                 - je Croc2.exe+1AE28
			Croc2.exe+1AE05 - 81 E1 FFFBFFFF        - and ecx,FFFFFBFF
			Croc2.exe+1AE0B - 89 0D 54A55200        - mov [Croc2.exe+12A554],ecx
			Croc2.exe+1AE11 - 74 15                 - je Croc2.exe+1AE28
			Croc2.exe+1AE13 - 6A 00                 - push 00
			Croc2.exe+1AE15 - E8 56FDFFFF           - call Croc2.exe+1AB70
			Croc2.exe+1AE1A - A1 68A55200           - mov eax,[Croc2.exe+12A568]
			Croc2.exe+1AE1F - 8B 0D 54A55200        - mov ecx,[Croc2.exe+12A554]
			Croc2.exe+1AE25 - 83 C4 04              - add esp,04
			Croc2.exe+1AE28 - 8B 15 88A55200        - mov edx,[Croc2.exe+12A588]
			Croc2.exe+1AE2E - F6 C6 08              - test dh,08
			Croc2.exe+1AE31 - 74 1D                 - je Croc2.exe+1AE50
			Croc2.exe+1AE33 - 81 E1 FFF7FFFF        - and ecx,FFFFF7FF
			Croc2.exe+1AE39 - 89 0D 54A55200        - mov [Croc2.exe+12A554],ecx
			Croc2.exe+1AE3F - 74 0F                 - je Croc2.exe+1AE50
			Croc2.exe+1AE41 - 6A 01                 - push 01
			Croc2.exe+1AE43 - E8 28FDFFFF           - call Croc2.exe+1AB70
			Croc2.exe+1AE48 - A1 68A55200           - mov eax,[Croc2.exe+12A568]
			Croc2.exe+1AE4D - 83 C4 04              - add esp,04
			Croc2.exe+1AE50 - 8B 0D F0624B00        - mov ecx,[Croc2.exe+B62F0]
			Croc2.exe+1AE56 - 80 E4 EF              - and ah,-11
			Croc2.exe+1AE59 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AE5B - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AE60 - 7E 08                 - jle Croc2.exe+1AE6A
			Croc2.exe+1AE62 - 80 CC 10              - or ah,10
			Croc2.exe+1AE65 - A3 68A55200           - mov [Croc2.exe+12A568],eax
			Croc2.exe+1AE6A - 8B 0D 80A55200        - mov ecx,[Croc2.exe+12A580]
			Croc2.exe+1AE70 - A3 80A55200           - mov [Croc2.exe+12A580],eax
			Croc2.exe+1AE75 - 8B D1                 - mov edx,ecx
			Croc2.exe+1AE77 - A3 88A55200           - mov [Croc2.exe+12A588],eax
			Croc2.exe+1AE7C - F7 D2                 - not edx
			Croc2.exe+1AE7E - 23 D0                 - and edx,eax
			Croc2.exe+1AE80 - 89 15 78A55200        - mov [Croc2.exe+12A578],edx
			Croc2.exe+1AE86 - 8B D0                 - mov edx,eax
			Croc2.exe+1AE88 - F7 D2                 - not edx
			Croc2.exe+1AE8A - 23 D1                 - and edx,ecx
			Croc2.exe+1AE8C - 8B 0D 90A55200        - mov ecx,[Croc2.exe+12A590]
			Croc2.exe+1AE92 - F7 D1                 - not ecx
			Croc2.exe+1AE94 - 23 C8                 - and ecx,eax
			Croc2.exe+1AE96 - 89 15 7CA55200        - mov [Croc2.exe+12A57C],edx
			Croc2.exe+1AE9C - 89 0D 54A55200        - mov [Croc2.exe+12A554],ecx
			Croc2.exe+1AEA2 - A3 90A55200           - mov [Croc2.exe+12A590],eax
			Croc2.exe+1AEA7 - C3                    - ret
		*/
		api->HookFunction(0x41acf0, 440, &remapUpdatePadForMenu, INJECT_REPLACE);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+1B1EC - E7 B0                 - out -50,eax
			Croc2.exe+1B1EE - 41                    - inc ecx
			Croc2.exe+1B1EF - 00 EE                 - add dh,ch
			Croc2.exe+1B1F1 - B0 41                 - mov al,41
			Croc2.exe+1B1F3 - 00 F5                 - add ch,dh
			Croc2.exe+1B1F5 - B0 41                 - mov al,41
			Croc2.exe+1B1F7 - 00 FC                 - add ah,bh
			Croc2.exe+1B1F9 - B0 41                 - mov al,41
			Croc2.exe+1B1FB - 00 03                 - add [ebx],al
			Croc2.exe+1B1FD - B1 41                 - mov cl,41
			Croc2.exe+1B1FF - 00 0A                 - add [edx],cl
			Croc2.exe+1B201 - B1 41                 - mov cl,41
			Croc2.exe+1B203 - 00 11                 - add [ecx],dl
			Croc2.exe+1B205 - B1 41                 - mov cl,41
			Croc2.exe+1B207 - 00 18                 - add [eax],bl
			Croc2.exe+1B209 - B1 41                 - mov cl,41
			Croc2.exe+1B20B - 00 90 9090908B        - add [eax-746F6F70],dl
			Croc2.exe+1B211 - 0D FCD74B00           - or eax,Croc2.exe+BD7FC
			Croc2.exe+1B216 - 33 C0                 - xor eax,eax
			Croc2.exe+1B218 - 85 C9                 - test ecx,ecx
			Croc2.exe+1B21A - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B21F - 7E 0A                 - jle Croc2.exe+1B22B
			Croc2.exe+1B221 - B8 10000000           - mov eax,00000010
			Croc2.exe+1B226 - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B22B - 8B 0D 1CD84B00        - mov ecx,[Croc2.exe+BD81C]
			Croc2.exe+1B231 - 85 C9                 - test ecx,ecx
			Croc2.exe+1B233 - 7E 07                 - jle Croc2.exe+1B23C
			Croc2.exe+1B235 - 0C 40                 - or al,40
			Croc2.exe+1B237 - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B23C - 8B 0D 08D84B00        - mov ecx,[Croc2.exe+BD808]
			Croc2.exe+1B242 - 85 C9                 - test ecx,ecx
			Croc2.exe+1B244 - 7E 07                 - jle Croc2.exe+1B24D
			Croc2.exe+1B246 - 0C 80                 - or al,-80
			Croc2.exe+1B248 - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B24D - 8B 0D 10D84B00        - mov ecx,[Croc2.exe+BD810]
			Croc2.exe+1B253 - 85 C9                 - test ecx,ecx
			Croc2.exe+1B255 - 7E 07                 - jle Croc2.exe+1B25E
			Croc2.exe+1B257 - 0C 20                 - or al,20
			Croc2.exe+1B259 - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B25E - 8B 0D 4CD54B00        - mov ecx,[Croc2.exe+BD54C]
			Croc2.exe+1B264 - 85 C9                 - test ecx,ecx
			Croc2.exe+1B266 - 7E 08                 - jle Croc2.exe+1B270
			Croc2.exe+1B268 - 80 CC 40              - or ah,40
			Croc2.exe+1B26B - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B270 - 8B 0D C0D54B00        - mov ecx,[Croc2.exe+BD5C0]
			Croc2.exe+1B276 - 85 C9                 - test ecx,ecx
			Croc2.exe+1B278 - 7E 08                 - jle Croc2.exe+1B282
			Croc2.exe+1B27A - 80 CC 80              - or ah,-80
			Croc2.exe+1B27D - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B282 - 8B 0D 94D54B00        - mov ecx,[Croc2.exe+BD594]
			Croc2.exe+1B288 - 85 C9                 - test ecx,ecx
			Croc2.exe+1B28A - 7E 08                 - jle Croc2.exe+1B294
			Croc2.exe+1B28C - 80 CC 20              - or ah,20
			Croc2.exe+1B28F - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B294 - 8B 0D 18D54B00        - mov ecx,[Croc2.exe+BD518]
			Croc2.exe+1B29A - 85 C9                 - test ecx,ecx
			Croc2.exe+1B29C - 7E 07                 - jle Croc2.exe+1B2A5
			Croc2.exe+1B29E - 0C 08                 - or al,08
			Croc2.exe+1B2A0 - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B2A5 - 8B 0D 88D54B00        - mov ecx,[Croc2.exe+BD588]
			Croc2.exe+1B2AB - 85 C9                 - test ecx,ecx
			Croc2.exe+1B2AD - 7E 07                 - jle Croc2.exe+1B2B6
			Croc2.exe+1B2AF - 0C 01                 - or al,01
			Croc2.exe+1B2B1 - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B2B6 - 8B 0D 54D54B00        - mov ecx,[Croc2.exe+BD554]
			Croc2.exe+1B2BC - 85 C9                 - test ecx,ecx
			Croc2.exe+1B2BE - 7E 08                 - jle Croc2.exe+1B2C8
			Croc2.exe+1B2C0 - 80 CC 04              - or ah,04
			Croc2.exe+1B2C3 - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B2C8 - 8B 0D 58D54B00        - mov ecx,[Croc2.exe+BD558]
			Croc2.exe+1B2CE - 85 C9                 - test ecx,ecx
			Croc2.exe+1B2D0 - 7E 08                 - jle Croc2.exe+1B2DA
			Croc2.exe+1B2D2 - 80 CC 08              - or ah,08
			Croc2.exe+1B2D5 - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B2DA - 8B 0D 2CD54B00        - mov ecx,[Croc2.exe+BD52C]
			Croc2.exe+1B2E0 - 85 C9                 - test ecx,ecx
			Croc2.exe+1B2E2 - 7E 08                 - jle Croc2.exe+1B2EC
			Croc2.exe+1B2E4 - 80 CC 10              - or ah,10
			Croc2.exe+1B2E7 - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B2EC - 8B 0D A4EC4B00        - mov ecx,[Croc2.exe+BECA4]
			Croc2.exe+1B2F2 - 8B 15 2CEB4B00        - mov edx,[Croc2.exe+BEB2C]
			Croc2.exe+1B2F8 - F7 D1                 - not ecx
			Croc2.exe+1B2FA - 23 C8                 - and ecx,eax
			Croc2.exe+1B2FC - A3 A4EC4B00           - mov [Croc2.exe+BECA4],eax
			Croc2.exe+1B301 - 89 0D 68175300        - mov [Croc2.exe+131768],ecx
			Croc2.exe+1B307 - 8B 0D A8EC4B00        - mov ecx,[Croc2.exe+BECA8]
			Croc2.exe+1B30D - F7 D1                 - not ecx
			Croc2.exe+1B30F - 23 C8                 - and ecx,eax
			Croc2.exe+1B311 - 83 FA 10              - cmp edx,10
			Croc2.exe+1B314 - A3 78175300           - mov [Croc2.exe+131778],eax
			Croc2.exe+1B319 - A3 A8EC4B00           - mov [Croc2.exe+BECA8],eax
			Croc2.exe+1B31E - 75 50                 - jne Croc2.exe+1B370
			Croc2.exe+1B320 - F6 C4 04              - test ah,04
			Croc2.exe+1B323 - 74 23                 - je Croc2.exe+1B348
			Croc2.exe+1B325 - 81 E1 FFFBFFFF        - and ecx,FFFFFBFF
			Croc2.exe+1B32B - 89 0D 44175300        - mov [Croc2.exe+131744],ecx
			Croc2.exe+1B331 - 74 15                 - je Croc2.exe+1B348
			Croc2.exe+1B333 - 6A 00                 - push 00
			Croc2.exe+1B335 - E8 56FDFFFF           - call Croc2.exe+1B090
			Croc2.exe+1B33A - A1 58175300           - mov eax,[Croc2.exe+131758]
			Croc2.exe+1B33F - 8B 0D 44175300        - mov ecx,[Croc2.exe+131744]
			Croc2.exe+1B345 - 83 C4 04              - add esp,04
			Croc2.exe+1B348 - 8B 15 78175300        - mov edx,[Croc2.exe+131778]
			Croc2.exe+1B34E - F6 C6 08              - test dh,08
			Croc2.exe+1B351 - 74 1D                 - je Croc2.exe+1B370
			Croc2.exe+1B353 - 81 E1 FFF7FFFF        - and ecx,FFFFF7FF
			Croc2.exe+1B359 - 89 0D 44175300        - mov [Croc2.exe+131744],ecx
			Croc2.exe+1B35F - 74 0F                 - je Croc2.exe+1B370
			Croc2.exe+1B361 - 6A 01                 - push 01
			Croc2.exe+1B363 - E8 28FDFFFF           - call Croc2.exe+1B090
			Croc2.exe+1B368 - A1 58175300           - mov eax,[Croc2.exe+131758]
			Croc2.exe+1B36D - 83 C4 04              - add esp,04
			Croc2.exe+1B370 - 8B 0D E0D44B00        - mov ecx,[Croc2.exe+BD4E0]
			Croc2.exe+1B376 - 80 E4 EF              - and ah,-11
			Croc2.exe+1B379 - 85 C9                 - test ecx,ecx
			Croc2.exe+1B37B - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B380 - 7E 08                 - jle Croc2.exe+1B38A
			Croc2.exe+1B382 - 80 CC 10              - or ah,10
			Croc2.exe+1B385 - A3 58175300           - mov [Croc2.exe+131758],eax
			Croc2.exe+1B38A - 8B 0D 70175300        - mov ecx,[Croc2.exe+131770]
			Croc2.exe+1B390 - A3 70175300           - mov [Croc2.exe+131770],eax
			Croc2.exe+1B395 - 8B D1                 - mov edx,ecx
			Croc2.exe+1B397 - A3 78175300           - mov [Croc2.exe+131778],eax
			Croc2.exe+1B39C - F7 D2                 - not edx
			Croc2.exe+1B39E - 23 D0                 - and edx,eax
			Croc2.exe+1B3A0 - 89 15 68175300        - mov [Croc2.exe+131768],edx
			Croc2.exe+1B3A6 - 8B D0                 - mov edx,eax
			Croc2.exe+1B3A8 - F7 D2                 - not edx
			Croc2.exe+1B3AA - 23 D1                 - and edx,ecx
			Croc2.exe+1B3AC - 8B 0D 80175300        - mov ecx,[Croc2.exe+131780]
			Croc2.exe+1B3B2 - F7 D1                 - not ecx
			Croc2.exe+1B3B4 - 23 C8                 - and ecx,eax
			Croc2.exe+1B3B6 - 89 15 6C175300        - mov [Croc2.exe+13176C],edx
			Croc2.exe+1B3BC - 89 0D 44175300        - mov [Croc2.exe+131744],ecx
			Croc2.exe+1B3C2 - A3 80175300           - mov [Croc2.exe+131780],eax
			Croc2.exe+1B3C7 - C3                    - ret
		*/
		api->HookFunction(0x41B1EC, 475, &remapUpdatePadForMenu, INJECT_REPLACE);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+1AC50 - 8B 0D 0C564B00        - mov ecx,[Croc2.exe+B560C]
			Croc2.exe+1AC56 - 33 C0                 - xor eax,eax
			Croc2.exe+1AC58 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AC5A - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1AC5F - 7E 0A                 - jle Croc2.exe+1AC6B
			Croc2.exe+1AC61 - B8 10000000           - mov eax,00000010
			Croc2.exe+1AC66 - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1AC6B - 8B 0D 2C564B00        - mov ecx,[Croc2.exe+B562C]
			Croc2.exe+1AC71 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AC73 - 7E 07                 - jle Croc2.exe+1AC7C
			Croc2.exe+1AC75 - 0C 40                 - or al,40
			Croc2.exe+1AC77 - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1AC7C - 8B 0D 18564B00        - mov ecx,[Croc2.exe+B5618]
			Croc2.exe+1AC82 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AC84 - 7E 07                 - jle Croc2.exe+1AC8D
			Croc2.exe+1AC86 - 0C 80                 - or al,-80
			Croc2.exe+1AC88 - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1AC8D - 8B 0D 20564B00        - mov ecx,[Croc2.exe+B5620]
			Croc2.exe+1AC93 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AC95 - 7E 07                 - jle Croc2.exe+1AC9E
			Croc2.exe+1AC97 - 0C 20                 - or al,20
			Croc2.exe+1AC99 - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1AC9E - 8B 0D 5C534B00        - mov ecx,[Croc2.exe+B535C]
			Croc2.exe+1ACA4 - 85 C9                 - test ecx,ecx
			Croc2.exe+1ACA6 - 7E 08                 - jle Croc2.exe+1ACB0
			Croc2.exe+1ACA8 - 80 CC 40              - or ah,40
			Croc2.exe+1ACAB - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1ACB0 - 8B 0D D0534B00        - mov ecx,[Croc2.exe+B53D0]
			Croc2.exe+1ACB6 - 85 C9                 - test ecx,ecx
			Croc2.exe+1ACB8 - 7E 08                 - jle Croc2.exe+1ACC2
			Croc2.exe+1ACBA - 80 CC 80              - or ah,-80
			Croc2.exe+1ACBD - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1ACC2 - 8B 0D A4534B00        - mov ecx,[Croc2.exe+B53A4]
			Croc2.exe+1ACC8 - 85 C9                 - test ecx,ecx
			Croc2.exe+1ACCA - 7E 08                 - jle Croc2.exe+1ACD4
			Croc2.exe+1ACCC - 80 CC 20              - or ah,20
			Croc2.exe+1ACCF - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1ACD4 - 8B 0D 28534B00        - mov ecx,[Croc2.exe+B5328]
			Croc2.exe+1ACDA - 85 C9                 - test ecx,ecx
			Croc2.exe+1ACDC - 7E 07                 - jle Croc2.exe+1ACE5
			Croc2.exe+1ACDE - 0C 08                 - or al,08
			Croc2.exe+1ACE0 - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1ACE5 - 8B 0D 98534B00        - mov ecx,[Croc2.exe+B5398]
			Croc2.exe+1ACEB - 85 C9                 - test ecx,ecx
			Croc2.exe+1ACED - 7E 07                 - jle Croc2.exe+1ACF6
			Croc2.exe+1ACEF - 0C 01                 - or al,01
			Croc2.exe+1ACF1 - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1ACF6 - 8B 0D 64534B00        - mov ecx,[Croc2.exe+B5364]
			Croc2.exe+1ACFC - 85 C9                 - test ecx,ecx
			Croc2.exe+1ACFE - 7E 08                 - jle Croc2.exe+1AD08
			Croc2.exe+1AD00 - 80 CC 04              - or ah,04
			Croc2.exe+1AD03 - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1AD08 - 8B 0D 68534B00        - mov ecx,[Croc2.exe+B5368]
			Croc2.exe+1AD0E - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD10 - 7E 08                 - jle Croc2.exe+1AD1A
			Croc2.exe+1AD12 - 80 CC 08              - or ah,08
			Croc2.exe+1AD15 - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1AD1A - 8B 0D F0524B00        - mov ecx,[Croc2.exe+B52F0]
			Croc2.exe+1AD20 - 85 C9                 - test ecx,ecx
			Croc2.exe+1AD22 - 7E 08                 - jle Croc2.exe+1AD2C
			Croc2.exe+1AD24 - 80 CC 10              - or ah,10
			Croc2.exe+1AD27 - A3 60955200           - mov [Croc2.exe+129560],eax
			Croc2.exe+1AD2C - 8B 0D 78955200        - mov ecx,[Croc2.exe+129578]
			Croc2.exe+1AD32 - A3 78955200           - mov [Croc2.exe+129578],eax
			Croc2.exe+1AD37 - 8B D1                 - mov edx,ecx
			Croc2.exe+1AD39 - A3 80955200           - mov [Croc2.exe+129580],eax
			Croc2.exe+1AD3E - F7 D2                 - not edx
			Croc2.exe+1AD40 - 23 D0                 - and edx,eax
			Croc2.exe+1AD42 - 89 15 70955200        - mov [Croc2.exe+129570],edx
			Croc2.exe+1AD48 - 8B D0                 - mov edx,eax
			Croc2.exe+1AD4A - F7 D2                 - not edx
			Croc2.exe+1AD4C - 23 D1                 - and edx,ecx
			Croc2.exe+1AD4E - 8B 0D 88955200        - mov ecx,[Croc2.exe+129588]
			Croc2.exe+1AD54 - F7 D1                 - not ecx
			Croc2.exe+1AD56 - 23 C8                 - and ecx,eax
			Croc2.exe+1AD58 - 89 15 74955200        - mov [Croc2.exe+129574],edx
			Croc2.exe+1AD5E - 89 0D 4C955200        - mov [Croc2.exe+12954C],ecx
			Croc2.exe+1AD64 - A3 88955200           - mov [Croc2.exe+129588],eax
			Croc2.exe+1AD69 - C3                    - ret
		*/
		// TODO: api->HookFunction(0x41AC5F, 281, &remapUpdatePadForMenu, INJECT_REPLACE);
		break;
	}
	}
}

void __stdcall remapUpdateMenuControls() {
}

void patchUpdateMenuControls() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		break;
	}
	case GAMEVER_EU: {
		break;
	}
	case GAMEVER_DEMO: {
		break;
	}
	}
}

} // namespace

namespace Input::RemapActionButtons {

void Setup() {
	patchDecodePad();
	patchUpdatePadForMenu();
	// patchUpdateMenuControls();
}

} // namespace Input::RemapActionButtons
