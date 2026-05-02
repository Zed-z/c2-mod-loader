#include "GameHooks.h"

#include "ModApi.h"
#include <vector>

extern ModApi *api;

namespace GameHooks {

std::vector<void(__stdcall *)()> preStepCallbacks;

void __stdcall RunPreStepHooks() {
	for (auto &callback : preStepCallbacks) {
		callback();
	}
}

std::vector<void(__stdcall *)()> postStepCallbacks;

void __stdcall RunPostStepHooks() {
	for (auto &callback : postStepCallbacks) {
		callback();
	}
}

std::vector<void(__stdcall *)()> doorChangeCallbacks;

void __stdcall RunDoorChangeHooks() {
	api->LogDebug("[GameHooks] Door change hook triggered.");
	for (auto &callback : doorChangeCallbacks) {
		callback();
	}
}

std::vector<void(__stdcall *)()> mapChangeCallbacks;

void __stdcall RunMapChangeHooks() {
	api->LogDebug("[GameHooks] Map change hook triggered.");
	for (auto &callback : mapChangeCallbacks) {
		callback();
	}
}

std::vector<void(__stdcall *)()> playerDeathCallbacks;

void __stdcall RunPlayerDeathHooks() {
	api->LogDebug("[GameHooks] Player death hook triggered.");
	for (auto &callback : playerDeathCallbacks) {
		callback();
	}
}

void RegisterHook(int hook_type, void(__stdcall *func)()) {
	switch (hook_type) {
	case GAME_HOOK_POST_STEP:
		postStepCallbacks.push_back(func);
		break;
	case GAME_HOOK_DOOR_CHANGE:
		doorChangeCallbacks.push_back(func);
		break;
	case GAME_HOOK_MAP_CHANGE:
		mapChangeCallbacks.push_back(func);
		break;
	case GAME_HOOK_PLAYER_DEATH:
		playerDeathCallbacks.push_back(func);
		break;
	}
}

void ApplyHooks() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+894B0 - 55                    - push ebp
			Croc2.exe+894B1 - 8B EC                 - mov ebp,esp
			Croc2.exe+894B3 - 81 EC 98000000        - sub esp,00000098
			Croc2.exe+894B9 - A1 3C8C4A00           - mov eax,[Croc2.exe+A8C3C]
			Croc2.exe+894BE - C7 05 582D6200 20000000 - mov [Croc2.exe+222D58],00000020
		*/
		api->HookFunction(0x4894B0, 9, RunPreStepHooks, INJECT_BEFORE);
		/*
			Croc2.exe+8A32A - 0F85 7EFCFFFF         - jne Croc2.exe+89FAE
			Croc2.exe+8A330 - E8 EBCEF7FF           - call Croc2.exe+7220
			Croc2.exe+8A335 - 5F                    - pop edi
			Croc2.exe+8A336 - 5E                    - pop esi
			Croc2.exe+8A337 - 5B                    - pop ebx
			Croc2.exe+8A338 - 8B E5                 - mov esp,ebp
			Croc2.exe+8A33A - 5D                    - pop ebp
			Croc2.exe+8A33B - C3                    - ret
		*/
		api->HookFunction(0x48A335, 6, RunPostStepHooks, INJECT_AFTER);
		// Croc2.exe + 7FBF0 - 89 3D 88784B00 - mov[Croc2.exe + B7888], edi
		api->HookFunction(0x47FBF0, 6, RunDoorChangeHooks, INJECT_AFTER);
		// Croc2.exe + 18DBB - 89 3D 4C8C4A00 - mov [Croc2.exe + A8C4C],edi
		api->HookFunction(0x418DBB, 6, RunMapChangeHooks, INJECT_AFTER);
		// Croc2.exe + 81DF9 - C7 05 50794B00 01000000 - mov [Croc2.exe + B7950],00000001
		api->HookFunction(0x481DF9, 10, RunPlayerDeathHooks, INJECT_BEFORE);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+8A030 - 55                    - push ebp
			Croc2.exe+8A031 - 8B EC                 - mov ebp,esp
			Croc2.exe+8A033 - 81 EC 98000000        - sub esp,00000098
			Croc2.exe+8A039 - A1 3C9C4A00           - mov eax,[Croc2.exe+A9C3C]
			Croc2.exe+8A03E - C7 05 489F6200 20000000 - mov [Croc2.exe+229F48],00000020
		*/
		api->HookFunction(0x48A030, 9, RunPreStepHooks, INJECT_BEFORE);
		/*
			Croc2.exe+8AEAA - 0F85 7EFCFFFF         - jne Croc2.exe+8AB2E
			Croc2.exe+8AEB0 - E8 6BC3F7FF           - call Croc2.exe+7220
			Croc2.exe+8AEB5 - 5F                    - pop edi
			Croc2.exe+8AEB6 - 5E                    - pop esi
			Croc2.exe+8AEB7 - 5B                    - pop ebx
			Croc2.exe+8AEB8 - 8B E5                 - mov esp,ebp
			Croc2.exe+8AEBA - 5D                    - pop ebp
			Croc2.exe+8AEBB - C3                    - ret
		*/
		api->HookFunction(0x48AEB5, 6, RunPostStepHooks, INJECT_AFTER);
		// Croc2.exe+80740 - 89 3D 78EA4B00        - mov [Croc2.exe+BEA78],edi
		api->HookFunction(0x480740, 6, RunDoorChangeHooks, INJECT_AFTER);
		// Croc2.exe+192A3 - 89 3D 4C9C4A00        - mov [Croc2.exe+A9C4C],edi
		api->HookFunction(0x4192A3, 6, RunMapChangeHooks, INJECT_AFTER);
		// Croc2.exe+82949 - C7 05 40EB4B00 01000000 - mov [Croc2.exe+BEB40],00000001
		api->HookFunction(0x482949, 10, RunPlayerDeathHooks, INJECT_BEFORE);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+88970 - 55                    - push ebp
			Croc2.exe+88971 - 8B EC                 - mov ebp,esp
			Croc2.exe+88973 - 81 EC 98000000        - sub esp,00000098
			Croc2.exe+88979 - A1 3C7C4A00           - mov eax,[Croc2.exe+A7C3C]
			Croc2.exe+8897E - C7 05 501D6200 20000000 - mov [Croc2.exe+221D50],00000020
		*/
		api->HookFunction(0x488970, 9, RunPreStepHooks, INJECT_BEFORE);
		/*
			Croc2.exe+897EA - 0F85 7EFCFFFF         - jne Croc2.exe+8946E
			Croc2.exe+897F0 - E8 2BDAF7FF           - call Croc2.exe+7220
			Croc2.exe+897F5 - 5F                    - pop edi
			Croc2.exe+897F6 - 5E                    - pop esi
			Croc2.exe+897F7 - 5B                    - pop ebx
			Croc2.exe+897F8 - 8B E5                 - mov esp,ebp
			Croc2.exe+897FA - 5D                    - pop ebp
			Croc2.exe+897FB - C3                    - ret
		*/
		api->HookFunction(0x4897F5, 6, RunPostStepHooks, INJECT_AFTER);
		// Croc2.exe+7F060 - 89 3D 88684B00        - mov [Croc2.exe+B6888],edi
		api->HookFunction(0x47F060, 6, RunDoorChangeHooks, INJECT_AFTER);
		// Croc2.exe+18E44 - 89 3D 4C7C4A00        - mov [Croc2.exe+A7C4C],edi
		api->HookFunction(0x418E44, 6, RunMapChangeHooks, INJECT_AFTER);
		// Croc2.exe+81289 - C7 05 50694B00 01000000 - mov [Croc2.exe+B6950],00000001
		api->HookFunction(0x481289, 10, RunPlayerDeathHooks, INJECT_BEFORE);
		break;
	}
	}
}

} // namespace GameHooks
