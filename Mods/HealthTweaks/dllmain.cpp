#include "ModApi.h"
#include <iostream>
#include <windows.h>

ModApi *api = nullptr;

bool fullHealthOnRetry = true;
bool fullHealthOnLevelEntry = true;
bool infiniteHazardBounce = false;

void __stdcall restoreHealth() {
	SaveSlot *slot = api->GetCurrentSaveSlot();
	slot->health = slot->heartPots;
	api->ShowInfoToast("Health restored!");
}

MenuActionRegistration __stdcall restoreHealthRegistration() {
	return {"Restore Health", "Set your health to the maximum value.", restoreHealth, true};
}

void __stdcall toggleFullHealthOnRetryFunction() {
	fullHealthOnRetry = !fullHealthOnRetry;
	api->WriteIniBool(L"Config", L"FullHealthOnRetry", fullHealthOnRetry);
}

MenuActionRegistration __stdcall toggleFullHealthOnRetryRegistration() {
	static std::string label;
	label = std::string("Full Health On Retry: ") + (fullHealthOnRetry ? "Enabled" : "Disabled");
	return {
		label.c_str(),
		"Fully replenish health when retrying after a game over.",
		toggleFullHealthOnRetryFunction,
		true};
}

void __stdcall toggleFullHealthOnLevelEntryFunction() {
	fullHealthOnLevelEntry = !fullHealthOnLevelEntry;
	api->WriteIniBool(L"Config", L"FullHealthOnLevelEntry", fullHealthOnLevelEntry);
}

MenuActionRegistration __stdcall toggleFullHealthOnLevelEntryRegistration() {
	static std::string label;
	label = std::string("Full Health On Level Entry: ") + (fullHealthOnLevelEntry ? "Enabled" : "Disabled");
	return {
		label.c_str(),
		"Fully replenish health when entering a level.",
		toggleFullHealthOnLevelEntryFunction,
		true};
}

void __stdcall toggleInfiniteHazardBounceFunction() {
	infiniteHazardBounce = !infiniteHazardBounce;
	api->WriteIniBool(L"Config", L"InfiniteHazardBounce", infiniteHazardBounce);
}

MenuActionRegistration __stdcall toggleInfiniteHazardBounceRegistration() {
	static std::string label;
	label = std::string("Infinite Hazard Bounce: ") + (infiniteHazardBounce ? "Enabled" : "Disabled");
	return {
		label.c_str(),
		"Bounce on hazards indefinitely without respawning.",
		toggleInfiniteHazardBounceFunction,
		true};
}

void __stdcall onRetry() {
	if (!fullHealthOnRetry)
		return;
	SaveSlot *slot = api->GetCurrentSaveSlot();
	slot->health = slot->heartPots;
}

void __stdcall onLevelEntry() {
	if (!fullHealthOnLevelEntry)
		return;
	SaveSlot *slot = api->GetCurrentSaveSlot();
	slot->health = slot->heartPots;
}

void __stdcall postStep() {
	if (infiniteHazardBounce) {
		StratEntity *croc = api->GetEntity(crocObjRef);
		if (croc != nullptr && strcmp((char *)api->GetEntity(rootObjRef)->name, "WalkingCroc") == 0) {
			croc->localVars[CROC_VAR_HAZARD_BOUNCE_COUNT] = 0;
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		api = LoadModApi();
		if (!api)
			return FALSE;

		fullHealthOnRetry = api->SetupIniInt(L"Config", L"FullHealthOnRetry", true);
		fullHealthOnLevelEntry = api->SetupIniInt(L"Config", L"FullHealthOnLevelEntry", true);
		infiniteHazardBounce = api->SetupIniInt(L"Config", L"InfiniteHazardBounce", false);

		switch (api->GetGameVersion()) {
		case GAMEVER_US: {
			// Croc2.exe+7EF7 - C7 81 D0426000 05000000 - mov [ecx+Croc2.exe+2042D0],00000005
			api->HookFunction(0x407EF7, 10, &onRetry, INJECT_AFTER);
			// Croc2.exe+18DB5 - 89 1D 488C4A00 - mov [Croc2.exe+A8C48],ebx
			api->HookFunction(0x418DB5, 6, &onLevelEntry, INJECT_AFTER);
			break;
		}
		case GAMEVER_EU: {
			// Croc2.exe+7EFC - 89 99 C0B46000 - mov [ecx+Croc2.exe+20B4C0],ebx
			api->HookFunction(0x407EFC, 6, &onRetry, INJECT_AFTER);
			// Croc2.exe+1929D - 89 2D 489C4A00 - mov [Croc2.exe+A9C48],ebp
			api->HookFunction(0x41929D, 6, &onLevelEntry, INJECT_AFTER);
			break;
		}
		}

		api->RegisterMenuAction(hModule, restoreHealthRegistration);
		api->RegisterMenuAction(hModule, toggleFullHealthOnRetryRegistration);
		api->RegisterMenuAction(hModule, toggleFullHealthOnLevelEntryRegistration);
		api->RegisterMenuAction(hModule, toggleInfiniteHazardBounceRegistration);

		api->HookGame(GAME_HOOK_POST_STEP, &postStep);
	}
	return TRUE;
}
