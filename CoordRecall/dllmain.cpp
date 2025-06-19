#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>

ModApi* api = nullptr;

// Keys 0-9
#define KEY_COUNT 10
int coordKeys[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };

struct SavedCoords {
    bool saved = false;

    int x;
    int y;
    int z;
    int angle;
};

SavedCoords saved_coords[KEY_COUNT];


void __stdcall positionSave(int key) {
    saved_coords[key].x = api->AddressGetInt(api->ResolveAddress(ADDR_CROC_POS_X));
    saved_coords[key].y = api->AddressGetInt(api->ResolveAddress(ADDR_CROC_POS_Y));
    saved_coords[key].z = api->AddressGetInt(api->ResolveAddress(ADDR_CROC_POS_Z));
    saved_coords[key].angle = api->AddressGetInt(api->ResolveAddress(ADDR_CROC_POS_ANGLE));
    saved_coords[key].saved = true;

    std::ostringstream stream;
    stream << "Saved position x: " << saved_coords[key].x
        << " y: " << saved_coords[key].y
        << " z: " << saved_coords[key].z
        << " angle: " << saved_coords[key].angle;
    api->Log(stream.str());
    api->ShowToast(stream.str());
}

void __stdcall positionLoad(int key) {
    if (!saved_coords[key].saved) return;

    api->AddressSetInt(api->ResolveAddress(ADDR_CROC_POS_X), saved_coords[key].x);
    api->AddressSetInt(api->ResolveAddress(ADDR_CROC_POS_Y), saved_coords[key].y);
    api->AddressSetInt(api->ResolveAddress(ADDR_CROC_POS_Z), saved_coords[key].z);
    api->AddressSetInt(api->ResolveAddress(ADDR_CROC_POS_ANGLE), saved_coords[key].angle);

    std::ostringstream stream;
    stream << "Recalled position x: " << saved_coords[key].x
        << " y: " << saved_coords[key].y
        << " z: " << saved_coords[key].z
        << " angle: " << saved_coords[key].angle;
    api->Log(stream.str());
    api->ShowToast(stream.str());
}

void __stdcall positionClear(int key) {
    if (!saved_coords[key].saved) return;

    saved_coords[key].saved = false;

    std::ostringstream stream;
    stream << "Cleared position!";
    api->Log(stream.str());
    api->ShowToast(stream.str());
}


DWORD WINAPI PatchThread(LPVOID) {
    while (true) {

        bool save = GetAsyncKeyState(VK_F5);

		// Loop through keys 0-9
        for (int key = 0; key < KEY_COUNT; key++) {
            if (key == VK_F5) continue;

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


void __stdcall slot0save() { positionSave(0); }
void __stdcall slot0load() { positionLoad(0); }
MenuActionRegistration __stdcall slot0registration() {
    if (saved_coords[0].saved) {
        return { "Recall Slot 0", slot0load, true };
    } else {
        return { "Save Slot 0", slot0save, true };
	}
}

void __stdcall slot1save() { positionSave(1); }
void __stdcall slot1load() { positionLoad(1); }
MenuActionRegistration __stdcall slot1registration() {
    if (saved_coords[1].saved) {
        return { "Recall Slot 1", slot1load, true };
    }
    else {
        return { "Save Slot 1", slot1save, true };
    }
}

void __stdcall slot2save() { positionSave(2); }
void __stdcall slot2load() { positionLoad(2); }
MenuActionRegistration __stdcall slot2registration() {
    if (saved_coords[2].saved) {
        return { "Recall Slot 2", slot2load, true };
    }
    else {
        return { "Save Slot 2", slot2save, true };
    }
}

void __stdcall slot3save() { positionSave(3); }
void __stdcall slot3load() { positionLoad(3); }
MenuActionRegistration __stdcall slot3registration() {
    if (saved_coords[3].saved) {
        return { "Recall Slot 3", slot3load, true };
    }
    else {
        return { "Save Slot 3", slot3save, true };
    }
}

void __stdcall slot4save() { positionSave(4); }
void __stdcall slot4load() { positionLoad(4); }
MenuActionRegistration __stdcall slot4registration() {
    if (saved_coords[4].saved) {
        return { "Recall Slot 4", slot4load, true };
    }
    else {
        return { "Save Slot 4", slot4save, true };
    }
}

void __stdcall slot5save() { positionSave(5); }
void __stdcall slot5load() { positionLoad(5); }
MenuActionRegistration __stdcall slot5registration() {
    if (saved_coords[5].saved) {
        return { "Recall Slot 5", slot5load, true };
    }
    else {
        return { "Save Slot 5", slot5save, true };
    }
}

void __stdcall slot6save() { positionSave(6); }
void __stdcall slot6load() { positionLoad(6); }
MenuActionRegistration __stdcall slot6registration() {
    if (saved_coords[6].saved) {
        return { "Recall Slot 6", slot6load, true };
    }
    else {
        return { "Save Slot 6", slot6save, true };
    }
}

void __stdcall slot7save() { positionSave(7); }
void __stdcall slot7load() { positionLoad(7); }
MenuActionRegistration __stdcall slot7registration() {
    if (saved_coords[7].saved) {
        return { "Recall Slot 7", slot7load, true };
    }
    else {
        return { "Save Slot 7", slot7save, true };
    }
}

void __stdcall slot8save() { positionSave(8); }
void __stdcall slot8load() { positionLoad(8); }
MenuActionRegistration __stdcall slot8registration() {
    if (saved_coords[8].saved) {
        return { "Recall Slot 8", slot8load, true };
    }
    else {
        return { "Save Slot 8", slot8save, true };
    }
}

void __stdcall slot9save() { positionSave(9); }
void __stdcall slot9load() { positionLoad(9); }
MenuActionRegistration __stdcall slot9registration() {
    if (saved_coords[9].saved) {
        return { "Recall Slot 9", slot9load, true };
    }
    else {
        return { "Save Slot 9", slot9save, true };
    }
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;

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

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, PatchThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
