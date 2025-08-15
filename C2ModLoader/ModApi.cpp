#define IS_MOD_LOADER
#include "ModApi.h"
#include "Config.h"
#include "ImGuiHandler.h"
#include "Utils.h"
#include "Resource.h"
#include "Loader.h"

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
#include <regex>
#include <cstdint>

#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

#include <intrin.h>
#pragma intrinsic(_ReturnAddress)

void LogRaw(const std::string& message, const LogSeverity& severity, const std::string& prefix = "") {

    // Get module path
    HMODULE caller = GetCallingModule();
    PathInfo pathInfo = GetModuleFilepath(caller);

    // Get log path - ignore directory override
    HMODULE executable = GetModuleHandleA(NULL);
    PathInfo executableFilepath = GetModuleFilepath(executable);
    std::wstring logPath = executableFilepath.directory + L"\\" + StringToWString(LOG_FILE);

    // Open log
    std::wofstream log(logPath, std::ios::app);
    if (!log.is_open()) return;

    // Get current time
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo;
    localtime_s(&timeInfo, &now_c);

    // Format and write the timestamp and message
    std::wstringstream msg;
    msg << L"[" << std::put_time(&timeInfo, L"%Y-%m-%d %H:%M:%S") << L"] [" << StringToWString(prefix)  << pathInfo.name << L"] " << StringToWString(message) << std::endl;

    // Write the log to file and buffer
    log << msg.str();
    logMessages.push_back({ WStringToString(msg.str()), severity });

    // Close log
    log.close();
}

void LogInfo(const std::string& message) {
    if (!logInfo) return;
    LogRaw(message, LogSeverity::Info, "INFO    | ");
}


void LogDebug(const std::string& message) {
    if (!logDebug) return;
    LogRaw(message, LogSeverity::Debug, "DEBUG   | ");
}


void LogWarning(const std::string& message) {
    if (!logWarnings) return;
    LogRaw(message, LogSeverity::Warning, "WARNING | ");
}


void LogError(const std::string& message) {
    if (!logErrors) return;
    LogRaw(message, LogSeverity::Error, "ERROR   | ");
}


uintptr_t ResolveAddress(MemoryAddress address) {
    uintptr_t addr = address.base;

    for (size_t i = 0; i < MAX_OFFSETS; ++i) {

        // End of offsets
        if (address.offsets[i] == 0) break;

        if (IsBadReadPtr((void*)addr, sizeof(uintptr_t))) {
            //Log("Bad read at: " + std::to_string(addr));
            return 0;
        }

        addr = *(uintptr_t*)addr;

        if (addr == 0) {
            //Log("Null pointer during chain at offset index: " + std::to_string(i));
            return 0;
        }

        addr += address.offsets[i];
    }

    if (IsBadReadPtr((void*)addr, sizeof(uintptr_t))) {
        //Log("Bad address at: " + std::to_string(addr));
        return 0;
    }

    return addr;
}

void AddressSetInt(uintptr_t address, int value) {
    *(int*)address = value;

#ifdef DEBUG
    std::ostringstream stream;
    stream << "Memory at " << address << " set to " << value;
    LogInfo(stream.str());
#endif
}


int AddressGetInt(uintptr_t address) {

    int value = *(int*)address;

#ifdef DEBUG
    std::ostringstream stream;
    stream << "Memory at " << address << " is " << value;
    LogInfo(stream.str());
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


#define JMP_SIZE 5 // 1 byte for the instruction, 4 for the address

bool InjectCode(uintptr_t hook_address, size_t hook_length, BYTE* code, size_t code_length, int inject_type) {

    // Allocate memory
    size_t totalSize = code_length + JMP_SIZE;
    if (inject_type != INJECT_REPLACE) {
        totalSize += hook_length;
    }

    uintptr_t newmem = (uintptr_t)VirtualAlloc(NULL, totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!newmem) {
        LogError("Failed to allocate memory for code injection.");
        return false;
    }


    // Insert code
    switch (inject_type) {
    case INJECT_BEFORE: {
        memcpy((void*)newmem, code, code_length);
        memcpy((void*)(newmem + code_length), (void*)hook_address, hook_length);
        break;
    }
    case INJECT_REPLACE: {
        memcpy((void*)newmem, code, code_length);
        break;
    }
    case INJECT_AFTER: {
        memcpy((void*)newmem, (void*)hook_address, hook_length);
        memcpy((void*)(newmem + hook_length), code, code_length);
        break;
    }
    }


    // Add jumpback instruction
    uintptr_t returnAddress = hook_address + hook_length;
    uintptr_t jmpReturnOffset = returnAddress - (newmem + totalSize);

    BYTE jumpback[] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
    *(int32_t*)&jumpback[1] = (int32_t)jmpReturnOffset;
    memcpy((void*)(newmem + totalSize - JMP_SIZE), jumpback, JMP_SIZE);


    // Prepare the patch
    BYTE* patch = new BYTE[hook_length];
    memset(patch, 0x90, hook_length); // Fill with NOP

    // Calculate the position of the next instructions as an offset
    int32_t rel_jmp_to_newmem = (int32_t)((uintptr_t)newmem - hook_address - JMP_SIZE);

    BYTE jump[] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
    *(int32_t*)&jump[1] = (int32_t)rel_jmp_to_newmem;
    memcpy(patch, jump, JMP_SIZE);

    // Replace the instruction at the address with the hook
    DWORD protect;
    VirtualProtect((LPVOID)hook_address, hook_length, PAGE_EXECUTE_READWRITE, &protect);
    memcpy((void*)hook_address, patch, hook_length);
    VirtualProtect((LPVOID)hook_address, hook_length, protect, &protect);


    // Cleanup
    delete[] patch;
    return true;
}

// Allows you to put arbitrary C++ code
bool HookFunction(uintptr_t target, size_t length, void(__stdcall* func)()) {

    // Target needs to be able to fit a jump
    if (length < JMP_SIZE) return false;


    // Allocate memory for injection
    // Trampoline: an injected bit of code made just to call a function
    // Use pushad and pushfd to store CPU state, to prevent weird issues 
    // original instructions + pushad + pushfd + function call + popfd + popad + jmp back
    size_t trampoline_size = length + 1 + 1 + JMP_SIZE + 1 + 1 + JMP_SIZE;
    BYTE* trampoline = (BYTE*)VirtualAlloc(
        nullptr, trampoline_size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    if (!trampoline) return false;


    // Copy original instructions
    memcpy(trampoline, (void*)target, length);


    // Start proper trampoline
    BYTE* p = trampoline + length;


    // Save state
    *p++ = 0x60;  // pushad
    *p++ = 0x9C;  // pushfd


    // Append function call 
    p[0] = 0xE8; // CALL <4 byte function address - relative>

    int32_t relative_function = (int32_t)(
        (uintptr_t)func - ((uintptr_t)p + JMP_SIZE)
        );

    memcpy(p + 1, &relative_function, 4);
    p += JMP_SIZE;


    // Restore state
    *p++ = 0x9D;  // popfd
    *p++ = 0x61;  // popad


    // Append JMP back to target + length
    p[0] = 0xE9; // CALL <4 byte function address - relative>

    int32_t relative_jumpback = (int32_t)(
        (int64_t)(target + length) - ((uintptr_t)p + JMP_SIZE)
        );

    memcpy(p + 1, &relative_jumpback, 4);


    // Prepare patch
    BYTE patch[16];
    memset(patch, 0x90, length); // NOPs

    patch[0] = 0xE9;
    int32_t relative_trampoline = (int32_t)(
        (uintptr_t)trampoline - (target + JMP_SIZE)
        );
    memcpy(patch + 1, &relative_trampoline, 4);


    // Apply patch
    DWORD old;
    VirtualProtect((void*)target, length, PAGE_EXECUTE_READWRITE, &old);
    memcpy((void*)target, patch, length);
    VirtualProtect((void*)target, length, old, &old);

    return true;
}

bool HookPhysics(void(__stdcall* func)()) {
    physicsCallbacks.push_back(func);
    return true;
}


int ReadIniInt(const std::wstring& section, const std::wstring& key, int default_value) {

    // Get module path
    HMODULE caller = GetCallingModule();
    PathInfo PathInfo = GetModuleFilepath(caller);
    std::wstring iniPath = std::regex_replace(PathInfo.path, std::wregex(L".asi$"), L".ini");

    return GetPrivateProfileIntW(section.c_str(), key.c_str(), default_value, iniPath.c_str());
}

bool WriteIniInt(const std::wstring& section, const std::wstring& key, int value) {

    // Get module path
    HMODULE caller = GetCallingModule();
    PathInfo PathInfo = GetModuleFilepath(caller);
    std::wstring iniPath = std::regex_replace(PathInfo.path, std::wregex(L".asi$"), L".ini");

    wchar_t valueStr[16];
    _itow_s(value, valueStr, 10);
    return WritePrivateProfileStringW(section.c_str(), key.c_str(), valueStr, iniPath.c_str()) != 0;
}

bool ReadIniBool(const std::wstring& section, const std::wstring& key, bool default_value) {

    // Get module path
    HMODULE caller = GetCallingModule();
    PathInfo PathInfo = GetModuleFilepath(caller);
    std::wstring iniPath = std::regex_replace(PathInfo.path, std::wregex(L".asi$"), L".ini");

    return (bool)GetPrivateProfileIntW(section.c_str(), key.c_str(), (int)default_value, iniPath.c_str());
}

bool WriteIniBool(const std::wstring& section, const std::wstring& key, bool value) {

    // Get module path
    HMODULE caller = GetCallingModule();
    PathInfo PathInfo = GetModuleFilepath(caller);
    std::wstring iniPath = std::regex_replace(PathInfo.path, std::wregex(L".asi$"), L".ini");

    wchar_t valueStr[16];
    _itow_s((int)value, valueStr, 10);
    return WritePrivateProfileStringW(section.c_str(), key.c_str(), valueStr, iniPath.c_str()) != 0;
}

void ReadIniString(const std::wstring& section, const std::wstring& key, const wchar_t* default_value, wchar_t* buffer, DWORD buffer_size) {

    // Get module path
    HMODULE caller = GetCallingModule();
    PathInfo PathInfo = GetModuleFilepath(caller);
    std::wstring iniPath = std::regex_replace(PathInfo.path, std::wregex(L".asi$"), L".ini");

    GetPrivateProfileStringW(section.c_str(), key.c_str(), default_value, buffer, buffer_size, iniPath.c_str());
}

bool WriteIniString(const std::wstring& section, const std::wstring& key, const wchar_t* value) {

    // Get module path
    HMODULE caller = GetCallingModule();
    PathInfo PathInfo = GetModuleFilepath(caller);
    std::wstring iniPath = std::regex_replace(PathInfo.path, std::wregex(L".asi$"), L".ini");

    return WritePrivateProfileStringW(section.c_str(), key.c_str(), value, iniPath.c_str()) != 0;
}



std::string GameVersions[] = { "UNKNOWN", "US", "EU", "DEMO" };

int GetGameVersion() {

    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return GAMEVER_UNKNOWN;

    std::streamsize size = file.tellg();
    file.close();

    if (size == 0xB4000) return GAMEVER_US;
    if (size == 0xBD000) return GAMEVER_EU;
    if (size == 0xB3000) return GAMEVER_DEMO;
    return GAMEVER_UNKNOWN;
}


Inputs GetInputsRaw(int address) {

    // Get inputs
    int input = AddressGetInt(address);

    Inputs result;
    result.raw = input;

    result.pause = input & 8;

    result.up = input & 16;
    result.right = input & 32;
    result.down = input & 64;
    result.left = input & 128;

    result.invLeft = input & 256;
    result.invRight = input & 512;

    result.stepLeft = input & 1024;
    result.stepRight = input & 2048;

    result.invUse = input & 4096;
    result.flip = input & 8192;

    result.jump = input & 16384;
    result.attack = input & 32768;

    result.effectiveUp = input & 65536;
    result.effectiveDown = input & 131072;
    result.effectiveLeft = input & 262144;
    result.effectiveRight = input & 524288;

    return result;
}

Inputs GetInputs() {
    return GetInputsRaw(ADDR_INPUTS);
}

Inputs GetInputsPressed() {
    return GetInputsRaw(ADDR_INPUTS_PRESSED);
}

Inputs GetInputsReleased() {
    return GetInputsRaw(ADDR_INPUTS_RELEASED);
}


void ShowToast(const std::string& message) {
    ImGuiShowToast(message);
}

bool RegisterMenuAction(HMODULE handle, MenuActionRegistrationFunction registration) {
    return ImGuiRegisterMenuAction(handle, registration);
}


StratEntity* GetEntity(MemoryAddress address) {
    StratEntity* entity = nullptr;

    uintptr_t addr = api->ResolveAddress(address);

    if (addr == 0) return nullptr;
    if (IsBadReadPtr((void*)addr, sizeof(StratEntity))) return nullptr;

    entity = (StratEntity*)addr;

    if (entity->next == nullptr) return nullptr;
    if (IsBadReadPtr((void*)(entity->next), sizeof(StratEntity))) return nullptr;

    entity = entity->next;

    return entity;
}


// API
ModApi g_ModApi = {
    LogInfo,
    LogDebug,
    LogWarning,
    LogError,
    ResolveAddress,
    FindPattern,
    AddressSetInt,
    AddressGetInt,
    PatchBytes,
    ReadBytes,
    InjectCode,
    HookFunction,
    HookPhysics,
    ReadIniInt,
    WriteIniInt,
    ReadIniBool,
    WriteIniBool,
    ReadIniString,
    WriteIniString,
    GetGameVersion,
    GetInputs,
    GetInputsPressed,
    GetInputsReleased,
    ShowToast,
    RegisterMenuAction,
    GetEntity
};

extern "C" __declspec(dllexport) ModApi * GetModApi() {
    return &g_ModApi;
}
