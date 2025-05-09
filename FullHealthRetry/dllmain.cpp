#include "ModApi.h"
#include <Windows.h>
#include <iostream>

ModApi* api = nullptr;

uintptr_t hookAddress = 0x407EF7;
#define overriddenInstructionSize 10
#define jumpInstructionSize 5
uintptr_t returnAddress = hookAddress + overriddenInstructionSize;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        api = LoadSharedModApi();
        if (!api) return FALSE;
        
        uintptr_t newmem = (uintptr_t)VirtualAlloc(NULL, 2048, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!newmem) {
            api->Log("Failed to allocate memory for code injection.");
            return FALSE;
        }

        uintptr_t jmpReturnOffset = returnAddress - (newmem + 13 + jumpInstructionSize); // 1+5+6+1 = 13 = total bytes before jmp

        BYTE injectedCode[] = {
            0x50,                                     // push eax
            0xA1, 0xCC, 0x62, 0x60, 0x00,             // mov eax, [Croc2.exe+2062CC] (absolute addr)
            0x89, 0x81, 0xD0, 0x42, 0x60, 0x00,       // mov [ecx+Croc2.exe+2042D0], eax
            0x58,                                     // pop eax
            0xE9, 0x00, 0x00, 0x00, 0x00              // jmp returnAddress (to be filled)
        };


        // Patch in the relative jump offset
        *(int32_t*)&injectedCode[14] = (int32_t)jmpReturnOffset;


        api->PatchBytes(newmem, injectedCode, sizeof(injectedCode));


        // Hook: jmp to our code
        // JMP <4 BYTE ADDRESS> NOP NOP
        BYTE hook[overriddenInstructionSize] = { 0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90, 0x90 };
        int32_t relAddr = (int32_t)(newmem - hookAddress - jumpInstructionSize);
        memcpy(&hook[1], &relAddr, 4);

        api->PatchBytes(hookAddress, hook, sizeof(hook));

        api->Log("Code injection installed.");

    }
    return TRUE;
}
