#include "ModApi.h"
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>

ModApi* api = nullptr;

uintptr_t addr_x, addr_y, addr_z, addr_angle;

struct SavedCoords {
    int x;
    int y;
    int z;
    int angle;
};
SavedCoords saved_coords[3];
bool coords_saved[3] = { false,false,false };

uintptr_t ReadPointer(uintptr_t base, const std::vector<uintptr_t>& offsets) {
    uintptr_t addr = base;

    for (size_t i = 0; i < offsets.size(); ++i) {
        if (IsBadReadPtr((void*)addr, sizeof(uintptr_t))) {
            api->Log("Bad read at: " + std::to_string(addr));
            return 0;
        }

        addr = *(uintptr_t*)addr;

        if (addr == 0) {
            api->Log("Null pointer during chain at offset index: " + std::to_string(i));
            return 0;
        }

        addr += offsets[i];
    }

    return addr;
}


DWORD WINAPI PatchThread(LPVOID) {
    while (true) {

        int keymap[] = {VK_F6, VK_F7, VK_F8};
        bool save = GetAsyncKeyState(VK_F5);

        for (int i = 0; i < 3; i++) {

            if (GetAsyncKeyState(keymap[i]) & 1) {

                // Save position
                if (save) {

                    addr_x = ReadPointer(0x4A8C3C, { 0x14, 0x28, 0x2C });
                    addr_y = ReadPointer(0x4A8C3C, { 0x14, 0x28, 0x30 });
                    addr_z = ReadPointer(0x4A8C3C, { 0x14, 0x28, 0x34 });
                    addr_angle = ReadPointer(0x4A8C3C, { 0x14, 0x28, 0x24 });

                    saved_coords[i].x = *(int*)addr_x;
                    saved_coords[i].y = *(int*)addr_y;
                    saved_coords[i].z = *(int*)addr_z;
                    saved_coords[i].angle = *(int*)addr_angle;

                    coords_saved[i] = true;

                    std::ostringstream stream;
                    stream << "Saved position x: " << saved_coords[i].x
                        << " y: " << saved_coords[i].y
                        << " z: " << saved_coords[i].z
                        << " angle: " << saved_coords[i].angle;
                    api->Log(stream.str());
                }

                // Recall positin
                else {
                    if (!coords_saved[i]) continue;

                    addr_x = ReadPointer(0x4A8C3C, { 0x14, 0x28, 0x2C });
                    addr_y = ReadPointer(0x4A8C3C, { 0x14, 0x28, 0x30 });
                    addr_z = ReadPointer(0x4A8C3C, { 0x14, 0x28, 0x34 });
                    addr_angle = ReadPointer(0x4A8C3C, { 0x14, 0x28, 0x24 });

                    *(int*)addr_x = saved_coords[i].x;
                    *(int*)addr_y = saved_coords[i].y;
                    *(int*)addr_z = saved_coords[i].z;
                    *(int*)addr_angle = saved_coords[i].angle;

                    std::ostringstream stream;
                    stream << "Recalled position x: " << saved_coords[i].x
                        << " y: " << saved_coords[i].y
                        << " z: " << saved_coords[i].z
                        << " angle: " << saved_coords[i].angle;
                    api->Log(stream.str());
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
