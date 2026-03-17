#include "ModApi.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <windows.h>

ModApi *api = nullptr;

// Keys 0-9
#define KEY_COUNT 10
int coordKeys[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};

struct SavedCoords {
	bool saved = false;
	RotPos3i coords;

	std::string toString(bool showAngle = false) const {
		std::ostringstream stream;

		stream << "x:" << coords.position.x << ",y:" << coords.position.y << ",z:" << coords.position.z;
		if (showAngle) {
			stream << ",angle:" << coords.rotation.y;
		}

		return stream.str();
	}
};

SavedCoords saved_coords[KEY_COUNT];

void __stdcall positionSave(int key) {
	StratEntity *croc = api->GetEntity(ADDR_CROC_OBJ);
	if (croc == nullptr) {
		api->ShowErrorToast("Croc entity not present!");
		api->LogError("Croc entity not present!");
		return;
	}

	saved_coords[key].coords = croc->newRotPos;
	saved_coords[key].saved = true;

	std::string toastMessage = "Saved position " + std::to_string(key) + ": " + saved_coords[key].toString();
	std::string logMessage = "Saved position " + std::to_string(key) + ": " + saved_coords[key].toString(true);
	api->ShowInfoToast(toastMessage.c_str());
	api->LogInfo(logMessage.c_str());
}

void __stdcall positionLoad(int key) {
	if (!saved_coords[key].saved)
		return;

	StratEntity *croc = api->GetEntity(ADDR_CROC_OBJ);
	if (croc == nullptr) {
		api->ShowErrorToast("Croc entity not present!");
		api->LogError("Croc entity not present!");
		return;
	}

	croc->newRotPos = saved_coords[key].coords;

	std::string toastMessage = "Recalled position " + std::to_string(key) + ": " + saved_coords[key].toString();
	std::string logMessage = "Recalled position " + std::to_string(key) + ": " + saved_coords[key].toString(true);
	api->ShowInfoToast(toastMessage.c_str());
	api->LogInfo(logMessage.c_str());
}

void __stdcall positionClear(int key) {
	if (!saved_coords[key].saved)
		return;

	saved_coords[key].saved = false;

	std::ostringstream stream;
	stream << "Cleared position!";
	std::string message = stream.str();
	api->LogInfo(message.c_str());
	api->ShowInfoToast(message.c_str());
}

void __stdcall positionClearAll() {
	for (int i = 0; i < KEY_COUNT; i++) {
		if (saved_coords[i].saved) {
			saved_coords[i].saved = false;
		}
	}

	std::ostringstream stream;
	stream << "Cleared all positions!";
	std::string message = stream.str();
	api->LogInfo(message.c_str());
	api->ShowInfoToast(message.c_str());
}

DWORD WINAPI PatchThread(LPVOID) {
	while (true) {

		bool save = GetAsyncKeyState(VK_F5);

		// Loop through keys 0-9
		for (int key = 0; key < KEY_COUNT; key++) {
			if (key == VK_F5)
				continue;

			if (GetAsyncKeyState(coordKeys[key]) & 1) {

				// Save position
				if (save) {
					positionSave(key);
				}

				// Recall position
				else {
					positionLoad(key);
				}
			}
		}

		Sleep(50);
	}
	return 0;
}

#define DEFINE_SLOT_FUNCS(N)                                                                              \
	void __stdcall slot##N##save() { positionSave(N); }                                                   \
	void __stdcall slot##N##load() { positionLoad(N); }                                                   \
	MenuActionRegistration __stdcall slot##N##registration() {                                            \
		static std::string label;                                                                         \
		if (saved_coords[N].saved) {                                                                      \
			label = "Recall Slot " #N " - " + saved_coords[N].toString();                                 \
			return {label.c_str(), "Set your position from this slot.", slot##N##load, true};             \
		} else {                                                                                          \
			return {"Save Slot " #N " - EMPTY", "Save your position to this slot.", slot##N##save, true}; \
		}                                                                                                 \
	}

DEFINE_SLOT_FUNCS(0)
DEFINE_SLOT_FUNCS(1)
DEFINE_SLOT_FUNCS(2)
DEFINE_SLOT_FUNCS(3)
DEFINE_SLOT_FUNCS(4)
DEFINE_SLOT_FUNCS(5)
DEFINE_SLOT_FUNCS(6)
DEFINE_SLOT_FUNCS(7)
DEFINE_SLOT_FUNCS(8)
DEFINE_SLOT_FUNCS(9)

MenuActionRegistration __stdcall slotClearAllRegistration() {
	return {"Clear All Slots", "Clear all saved positions.", positionClearAll, true};
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		api = LoadModApi();
		if (!api)
			return FALSE;

		// Register menu actions
		api->RegisterMenuAction(hModule, slot0registration);
		api->RegisterMenuAction(hModule, slot1registration);
		api->RegisterMenuAction(hModule, slot2registration);
		api->RegisterMenuAction(hModule, slot3registration);
		api->RegisterMenuAction(hModule, slot4registration);
		api->RegisterMenuAction(hModule, slot5registration);
		api->RegisterMenuAction(hModule, slot6registration);
		api->RegisterMenuAction(hModule, slot7registration);
		api->RegisterMenuAction(hModule, slot8registration);
		api->RegisterMenuAction(hModule, slot9registration);
		api->RegisterMenuAction(hModule, slotClearAllRegistration);

		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, PatchThread, nullptr, 0, nullptr);
	}
	return TRUE;
}
