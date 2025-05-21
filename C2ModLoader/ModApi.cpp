#define IS_MOD_LOADER
#include "ModApi.h"
#include "Config.h"
#include "ImGuiHandler.h"

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

inline HMODULE GetCallingModule() {
    HMODULE caller = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)_ReturnAddress(),
        &caller
    );
    return caller;
}


struct ModuleFilepath {
    std::wstring path;       // Full path
    std::wstring filename;   // Filename only
    std::wstring name;       // Filename without extension
    std::wstring directory;  // Directory only
};
inline ModuleFilepath GetModuleFilepath(HMODULE module) {
    wchar_t filePath[MAX_PATH];
    GetModuleFileNameW(module, filePath, MAX_PATH);
    std::wstring filePathStr(filePath);
    
    size_t lastBackslashPos = filePathStr.find_last_of(L"\\/");

    std::wstring filename = filePathStr.substr(lastBackslashPos + 1);
    std::wstring directory = filePathStr.substr(0, lastBackslashPos);

    size_t extensionPos = filename.find(L".asi");
    std::wstring name = filename;
    if (extensionPos != std::wstring::npos) {
        name = filename.substr(0, extensionPos);
    }

    return {
        filePath, filename, name, directory
    };
}

inline std::wstring ToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstrTo[0], size_needed);
    return wstrTo;
}
std::string WStringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, nullptr, nullptr);
    return strTo;
}

void Log(const std::string& message) {

    // Get module path
    HMODULE caller = GetCallingModule();
    ModuleFilepath moduleFilepath = GetModuleFilepath(caller);

    // Get log path - ignore directory override
    HMODULE executable = GetModuleHandleA(NULL);
    ModuleFilepath executableFilepath = GetModuleFilepath(executable);
    std::wstring logPath = executableFilepath.directory + L"\\" + ToWString(LOG_FILE);

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
    msg << L"[" << std::put_time(&timeInfo, L"%Y-%m-%d %H:%M:%S") << L"] [" << moduleFilepath.name << L"] " << ToWString(message) << std::endl;

    // Write the log to file and buffer
    log << msg.str();
    logBuffer.appendf("%s\n", WStringToUtf8(msg.str()).c_str());

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


#define JMP_SIZE 5 // 1 byte for the instruction, 4 for the address

bool InjectCode(uintptr_t hook_address, size_t hook_length, BYTE* code, size_t code_length, int inject_type) {

    // Allocate memory
    size_t totalSize = code_length + JMP_SIZE;
    if (inject_type != INJECT_REPLACE) {
        totalSize += hook_length;
    }

    uintptr_t newmem = (uintptr_t)VirtualAlloc(NULL, totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!newmem) {
        Log("Failed to allocate memory for code injection.");
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


int ReadIniInt(const std::wstring& section, const std::wstring& key, int default_value) {

    // Get module path
    HMODULE caller = GetCallingModule();
    ModuleFilepath moduleFilepath = GetModuleFilepath(caller);
    std::wstring iniPath = std::regex_replace(moduleFilepath.path, std::wregex(L".asi$"), L".ini");

    return GetPrivateProfileIntW(section.c_str(), key.c_str(), default_value, iniPath.c_str());
}

inline bool WriteIniInt(const std::wstring& section, const std::wstring& key, int value) {

    // Get module path
    HMODULE caller = GetCallingModule();
    ModuleFilepath moduleFilepath = GetModuleFilepath(caller);
    std::wstring iniPath = std::regex_replace(moduleFilepath.path, std::wregex(L".asi$"), L".ini");

    wchar_t valueStr[16];
    _itow_s(value, valueStr, 10);
    return WritePrivateProfileStringW(section.c_str(), key.c_str(), valueStr, iniPath.c_str()) != 0;
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


// API
ModApi g_ModApi = {
    Log, ResolveAddress, FindPattern, AddressSetInt, AddressGetInt, PatchBytes, ReadBytes, InjectCode, HookFunction, ReadIniInt, WriteIniInt, GetGameVersion
};

extern "C" __declspec(dllexport) ModApi * GetModApi() {
    return &g_ModApi;
}
