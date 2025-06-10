#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>

ModApi* api = nullptr;

struct SavedCoords {
    int x;
    int y;
    int z;
    int angle;
};

SavedCoords saved_coords[0xFF];
bool coords_saved[0xFF] = { false };

DWORD WINAPI PatchThread(LPVOID) {
    while (true) {

        bool save = GetAsyncKeyState(VK_F5);

        for (int key = 0x00; key < 0xFF; key++) {
            if (key == VK_F5) continue;

            if (GetAsyncKeyState(key) & 1) {

                // Save position
                if (save) {

                    saved_coords[key].x = api->AddressGetInt(api->ResolveAddress(ADDR_CROC_POS_X));
                    saved_coords[key].y = api->AddressGetInt(api->ResolveAddress(ADDR_CROC_POS_Y));
                    saved_coords[key].z = api->AddressGetInt(api->ResolveAddress(ADDR_CROC_POS_Z));
                    saved_coords[key].angle = api->AddressGetInt(api->ResolveAddress(ADDR_CROC_POS_ANGLE));

                    coords_saved[key] = true;

                    std::ostringstream stream;
                    stream << "Saved position x: " << saved_coords[key].x
                        << " y: " << saved_coords[key].y
                        << " z: " << saved_coords[key].z
                        << " angle: " << saved_coords[key].angle;
                    api->Log(stream.str());
                    api->ShowToast(stream.str());
                }

                // Recall position
                else {
                    if (!coords_saved[key]) continue;

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

            }

        }

        Sleep(50);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;

        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, PatchThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
