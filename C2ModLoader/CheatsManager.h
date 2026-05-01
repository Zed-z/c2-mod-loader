#pragma once

#include <stdint.h>
#include <windows.h>

// https://www.speedrun.com/croc_2/forums/l2qjp

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
