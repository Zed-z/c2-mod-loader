#pragma once

#include <windows.h>
#include <stdint.h>

// https://www.speedrun.com/croc_2/forums/l2qjp

extern uint32_t cheatsValue;
#define ADDR_CHEATS 0x4B7964

void SetupCheats();
void ApplyCheats();

extern bool cheatsDebugMenu;
void setDebugMenu(bool enable);

extern bool cheatsPositionBar;
void setPositionBar(bool enable);

extern bool cheatsInvulnerability;
void setInvulnerability(bool enable);

extern bool cheatsBonusCrystals;
void setBonusCrystals(bool enable);

extern bool cheatsMusicSelect;
void setMusicSelect(bool enable);
