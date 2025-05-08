#define IS_MOD_LOADER
#include "ModApi.h"
#include "Config.h"

#include <Windows.h>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>

#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")


void Log(const std::string& message) {

    // Get log path - ignore directory override
    // Get module name
    char modulePath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(NULL), modulePath, MAX_PATH);
    std::string path(modulePath);
    size_t pos = path.find_last_of("\\/");

    std::string logPath = path.substr(0, pos) + "\\" + LOG_FILE;
    std::string moduleName = path.substr(pos + 1);

    // Open log
    std::ofstream log(logPath, std::ios::app);
    if (!log.is_open()) return;

    // Get current time
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo;
    localtime_s(&timeInfo, &now_c);

    // Format and write the timestamp and message
    log << "[" << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S") << "] [" << moduleName << "]\t" << message << std::endl;

    // Close log
    log.close();
}


uintptr_t ResolveAddress(MemoryAddress address) {
    uintptr_t addr = address.base;

    for (size_t i = 0; i < MAX_OFFSETS; ++i) {

        // End of offsets
        if (address.offsets[i] == 0) break;

        if (IsBadReadPtr((void*)addr, sizeof(uintptr_t))) {
            Log("Bad read at: " + std::to_string(addr));
            return 0;
        }

        addr = *(uintptr_t*)addr;

        if (addr == 0) {
            Log("Null pointer during chain at offset index: " + std::to_string(i));
            return 0;
        }

        addr += address.offsets[i];
    }

    if (IsBadReadPtr((void*)addr, sizeof(uintptr_t))) {
        Log("Bad address at: " + std::to_string(addr));
        return 0;
    }

    return addr;
}

void AddressSetInt(uintptr_t address, int value) {
    *(int*)address = value;

#ifdef DEBUG
    std::ostringstream stream;
    stream << "Memory at " << address << " set to " << value;
    Log(stream.str());
#endif
}

int AddressGetInt(uintptr_t address) {

    int value = *(int*)address;

#ifdef DEBUG
    std::ostringstream stream;
    stream << "Memory at " << address << " is " << value;
    Log(stream.str());
#endif

    return value;
}

bool PatchBytes(uintptr_t address, const void* bytes, size_t size) {
    DWORD protect;
    if (!VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &protect)) {
        return false;
    }

    memcpy((LPVOID)address, bytes, size);

    DWORD temp;
    VirtualProtect((LPVOID)address, size, protect, &temp);
    return true;
}

bool ReadBytes(uintptr_t address, void* out_buffer, size_t size) {
    DWORD protect;
    if (!VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &protect)) {
        return false;
    }

    memcpy(out_buffer, (LPVOID)address, size);

    DWORD temp;
    VirtualProtect((LPVOID)address, size, protect, &temp);
    return true;
}

uintptr_t FindPattern(const void* pattern, size_t pattern_size, int occurrence) {
    HMODULE hModule = GetModuleHandle(nullptr);
    if (!hModule) return 0;

    MODULEINFO modInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
        return 0;

    uintptr_t start = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    uintptr_t end = start + modInfo.SizeOfImage;

    int count = 0;

    for (uintptr_t addr = start; addr <= end - pattern_size; ++addr) {
        if (memcmp(reinterpret_cast<void*>(addr), pattern, pattern_size) == 0) {
            if (++count == occurrence)
                return addr;
        }
    }

    return 0;
}


// API
ModApi g_ModApi = {
    Log, ResolveAddress, FindPattern, AddressSetInt, AddressGetInt, PatchBytes, ReadBytes
};

extern "C" __declspec(dllexport) ModApi * GetModApi() {
    return &g_ModApi;
}
