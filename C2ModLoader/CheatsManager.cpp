#include "CheatsManager.h"
#include "ModApi.h"

extern ModApi *api;

uint32_t cheatsValue = 0x00000000;

bool cheatsDebugMenu = false;
bool cheatsPositionBar = false;
bool cheatsInvulnerability = false;
bool cheatsBonusCrystals = false;
bool cheatsMusicSelect = false;

void SetupCheats() {
	cheatsValue = api->AddressGetInt(ADDR_CHEATS);

	cheatsDebugMenu = api->SetupIniBool(L"Cheats", L"DebugMenu", false);
	setDebugMenu(cheatsDebugMenu);

	cheatsPositionBar = api->SetupIniBool(L"Cheats", L"PositionBar", false);
	setPositionBar(cheatsPositionBar);

	cheatsInvulnerability = api->SetupIniBool(L"Cheats", L"Invulnerability", false);
	setInvulnerability(cheatsInvulnerability);

	cheatsBonusCrystals = api->SetupIniBool(L"Cheats", L"BonusCrystals", false);
	setBonusCrystals(cheatsBonusCrystals);

	cheatsMusicSelect = api->SetupIniBool(L"Cheats", L"MusicSelect", false);
	setMusicSelect(cheatsMusicSelect);
}

void ApplyCheats() {
	api->AddressSetInt(ADDR_CHEATS, cheatsValue);
}

void setDebugMenu(bool enable) {
	if (enable) {
		cheatsValue |= 1;
	} else {
		cheatsValue &= ~1;
	}
	api->WriteIniBool(L"Cheats", L"DebugMenu", cheatsDebugMenu);
	ApplyCheats();
}

void setPositionBar(bool enable) {
	if (enable) {
		cheatsValue |= 2;
	} else {
		cheatsValue &= ~2;
	}
	api->WriteIniBool(L"Cheats", L"PositionBar", cheatsPositionBar);
	ApplyCheats();
}

void setInvulnerability(bool enable) {
	if (enable) {
		cheatsValue |= 4;
	} else {
		cheatsValue &= ~4;
	}
	api->WriteIniBool(L"Cheats", L"Invulnerability", cheatsInvulnerability);
	ApplyCheats();
}

void setBonusCrystals(bool enable) {
	if (enable) {
		cheatsValue |= 16;
	} else {
		cheatsValue &= ~16;
	}
	api->WriteIniBool(L"Cheats", L"BonusCrystals", cheatsBonusCrystals);
	ApplyCheats();
}

void setMusicSelect(bool enable) {
	if (enable) {
		cheatsValue |= 32;
	} else {
		cheatsValue &= ~32;
	}
	api->WriteIniBool(L"Cheats", L"MusicSelect", cheatsMusicSelect);
	ApplyCheats();
}
