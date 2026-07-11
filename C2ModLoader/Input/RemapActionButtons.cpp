#include "RemapActionButtons.h"

#include "Input/Controls.h"
#include "Input/Input.h"
#include "ModApi.h"

extern ModApi *api;

namespace {

void __stdcall remapMenuButtons() {
	ModernInput input = Input::GetState();

	if (input.dpad.up || Input::getAnalogStickY(Input::Controls::config.movement) < 0 || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveUp)) {
		*realPad |= INPUT_UP;
		*playerPad |= INPUT_UP;
	}
	if (input.dpad.right || Input::getAnalogStickX(Input::Controls::config.movement) > 0 || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveRight)) {
		*realPad |= INPUT_RIGHT;
		*playerPad |= INPUT_RIGHT;
	}
	if (input.dpad.down || Input::getAnalogStickY(Input::Controls::config.movement) > 0 || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveDown)) {
		*realPad |= INPUT_DOWN;
		*playerPad |= INPUT_DOWN;
	}
	if (input.dpad.left || Input::getAnalogStickX(Input::Controls::config.movement) < 0 || Input::getKeyboardKeyPressed(Input::Controls::config.keyMoveLeft)) {
		*realPad |= INPUT_LEFT;
		*playerPad |= INPUT_LEFT;
	}

	if (Input::getButtonPressed(Input::Controls::config.jump) || Input::getKeyboardKeyPressed(Input::Controls::config.keyJump)) {
		*realPad |= INPUT_JUMP;
		*playerPad |= INPUT_JUMP;
	}
	if (Input::getButtonPressed(Input::Controls::config.attack) || Input::getKeyboardKeyPressed(Input::Controls::config.keyAttack)) {
		*realPad |= INPUT_ATTACK;
		*playerPad |= INPUT_ATTACK;
	}
	if (Input::getButtonPressed(Input::Controls::config.cameraFlip) || Input::getKeyboardKeyPressed(Input::Controls::config.keyCameraFlip)) {
		*realPad |= INPUT_FLIP;
		*playerPad |= INPUT_FLIP;
	}
	if (Input::getButtonPressed(Input::Controls::config.itemUse) || Input::getKeyboardKeyPressed(Input::Controls::config.keyItemUse)) {
		*realPad |= INPUT_INV_USE;
		*playerPad |= INPUT_INV_USE;
	}
	if (Input::getButtonPressed(Input::Controls::config.stepLeft) || Input::getKeyboardKeyPressed(Input::Controls::config.keyStepLeft)) {
		*realPad |= INPUT_STEP_LEFT;
		*playerPad |= INPUT_STEP_LEFT;
	}
	if (Input::getButtonPressed(Input::Controls::config.stepRight) || Input::getKeyboardKeyPressed(Input::Controls::config.keyStepRight)) {
		*realPad |= INPUT_STEP_RIGHT;
		*playerPad |= INPUT_STEP_RIGHT;
	}
	if (Input::getButtonPressed(Input::Controls::config.itemPrev) || Input::getKeyboardKeyPressed(Input::Controls::config.keyItemPrev)) {
		*realPad |= INPUT_INV_LEFT;
		*playerPad |= INPUT_INV_LEFT;
	}
	if (Input::getButtonPressed(Input::Controls::config.itemNext) || Input::getKeyboardKeyPressed(Input::Controls::config.keyItemNext)) {
		*realPad |= INPUT_INV_RIGHT;
		*playerPad |= INPUT_INV_RIGHT;
	}
	if (Input::getButtonPressed(Input::Controls::config.pause) || Input::getKeyboardKeyPressed(Input::Controls::config.keyPause)) {
		*realPad |= INPUT_PAUSE;
		*playerPad |= INPUT_PAUSE;
	}

	*realPadPush = *realPad & ~*realPadOld;
	*realPadOld = *realPad;

	*playerPadPush = *playerPad & ~*playerPadOld;
	*playerPadOld = *playerPad;
}

uint32_t input_builder = 0;
void __stdcall remapGameplayButtons() {
	input_builder = 0;

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

void Patch() {
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
		*/
		api->HookFunction(0x41acff, 205, &remapMenuButtons, INJECT_REPLACE);
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
		api->HookFunction(0x42f26b, 60, &remapGameplayButtons, INJECT_REPLACE);
		api->InjectCode(0x42f2a7, 13, hookCode, p, INJECT_REPLACE);
		break;
	}
	case GAMEVER_EU: {
		/*
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
		*/
		api->HookFunction(0x41B21F, 205, &remapMenuButtons, INJECT_REPLACE);
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
		api->HookFunction(0x42F9EB, 60, &remapGameplayButtons, INJECT_REPLACE);
		api->InjectCode(0x42FA27, 13, hookCode, p, INJECT_REPLACE);
		break;
	}
	case GAMEVER_DEMO: {
		/*
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
		*/
		api->HookFunction(0x41AC5F, 205, &remapMenuButtons, INJECT_REPLACE);
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
		api->HookFunction(0x42F69B, 60, &remapGameplayButtons, INJECT_REPLACE);
		api->InjectCode(0x42F6D7, 13, hookCode, p, INJECT_REPLACE);
		break;
	}
	}
}

} // namespace

namespace Input::RemapActionButtons {

void Setup() {
	Patch();
}

} // namespace Input::RemapActionButtons
