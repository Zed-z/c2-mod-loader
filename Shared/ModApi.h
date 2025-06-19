#pragma once
#include <string>
#include <Windows.h>
#include <bitset>

// Api definition --------------------------------------------------------

#define MAX_OFFSETS 256
typedef struct {
    uintptr_t base;
    uintptr_t offsets[MAX_OFFSETS];
} MemoryAddress;

#define ADDR_FOG_DISTANCE       { 0x4B7B48 }
#define ADDR_RENDER_DISTANCE    { 0x4B7B18 }

#define ADDR_CROC_POS_X         { 0x4A8C3C, { 0x14, 0x28, 0x2C } }
#define ADDR_CROC_POS_Y         { 0x4A8C3C, { 0x14, 0x28, 0x30 } }
#define ADDR_CROC_POS_Z         { 0x4A8C3C, { 0x14, 0x28, 0x34 } }
#define ADDR_CROC_POS_ANGLE     { 0x4A8C3C, { 0x14, 0x28, 0x24 } }

#define ADDR_CURRENT_SAVE_SLOT  { 0x6220FC }
#define ADDR_SAVE_SLOT_OFFSET   0x2000

#define ADDR_INPUTS 0x52A590

typedef void(*LogFunction)(const std::string& message);
typedef void(*LogDebugFunction)(const std::string& message);
typedef void(*LogWarningFunction)(const std::string& message);
typedef void(*LogErrorFunction)(const std::string& message);

typedef uintptr_t(*ResolveAddressFunction)(MemoryAddress address);
typedef uintptr_t(*FindPatternFunction)(const void* pattern, size_t pattern_size, int occurrence);

typedef void(*AddressSetIntFunction)(uintptr_t address, int value);
typedef int(*AddressGetIntFunction)(uintptr_t address);

typedef bool(*PatchBytesFunction)(uintptr_t address, const void* bytes, size_t size);
typedef bool(*ReadBytesFunction)(uintptr_t address, void* out_buffer, size_t size);

#define INJECT_REPLACE 0
#define INJECT_BEFORE 1
#define INJECT_AFTER 2
typedef bool(*InjectCodeFunction)(uintptr_t hook_address, size_t hook_length, BYTE* code, size_t code_length, int inject_type);
typedef bool(*HookFunctionFunction)(uintptr_t target, size_t length, void(__stdcall* func)());
typedef bool(*HookPhysicsFunction)(void(__stdcall* func)());

typedef int(*ReadIniIntFunction)(const std::wstring& section, const std::wstring& key, int default_value);
typedef bool(*WriteIniIntFunction)(const std::wstring& section, const std::wstring& key, int value);
typedef bool(*ReadIniBoolFunction)(const std::wstring& section, const std::wstring& key, bool default_value);
typedef bool(*WriteIniBoolFunction)(const std::wstring& section, const std::wstring& key, bool value);
typedef void(*ReadIniStringFunction)(const std::wstring& section, const std::wstring& key, const wchar_t* default_value, wchar_t* buffer, DWORD buffer_size);
typedef bool(*WriteIniStringFunction)(const std::wstring& section, const std::wstring& key, const wchar_t* value);


#define GAMEVER_UNKNOWN 0
#define GAMEVER_US 1
#define GAMEVER_EU 2
#define GAMEVER_DEMO 3
extern std::string GameVersions[]; // Use this to get game versions as strings for logging/display
typedef int(*GetGameVersionFunction)();


typedef void(*ShowToastFunction)(const std::string& message);


struct MenuActionRegistration {
    std::string label;
    void(__stdcall* callback)();
    bool enabled;
};
typedef MenuActionRegistration(__stdcall* MenuActionRegistrationFunction)();
typedef bool(*RegisterMenuActionFunction)(HMODULE handle, MenuActionRegistrationFunction registration);


struct Inputs {
    uint32_t raw;

    bool pause;

    bool up;
    bool down;
    bool left;
    bool right;

    bool effectiveUp;
    bool effectiveDown;
    bool effectiveLeft;
    bool effectiveRight;

    bool flip;
    bool stepLeft;
    bool stepRight;
    bool jump;
    bool attack;
    bool invLeft;
    bool invUse;
    bool invRight;
};
typedef Inputs(*GetInputsFunction)();


struct ModApi {
    LogFunction Log;
    LogDebugFunction LogDebug;
    LogWarningFunction LogWarning;
    LogErrorFunction LogError;

    ResolveAddressFunction ResolveAddress;
    FindPatternFunction FindPattern;

    AddressSetIntFunction AddressSetInt;
    AddressGetIntFunction AddressGetInt;

    PatchBytesFunction PatchBytes;
    ReadBytesFunction ReadBytes;

    InjectCodeFunction InjectCode;
    HookFunctionFunction HookFunction;
    HookPhysicsFunction HookPhysics;

    ReadIniIntFunction ReadIniInt;
    WriteIniIntFunction WriteIniInt;
    ReadIniBoolFunction ReadIniBool;
    WriteIniBoolFunction WriteIniBool;
    ReadIniStringFunction ReadIniString;
    WriteIniStringFunction WriteIniString;

    GetGameVersionFunction GetGameVersion;
    GetInputsFunction GetInputs;

    ShowToastFunction ShowToast;
    RegisterMenuActionFunction RegisterMenuAction;
};

extern "C" __declspec(dllexport) ModApi * GetModApi();


// Api client --------------------------------------------------------
inline ModApi* LoadSharedModApi(const char* moduleName = "C2ModLoader.asi") {
#ifdef IS_MOD_LOADER
    return nullptr;
#else
    HMODULE hLoader = GetModuleHandleA(moduleName);
    if (!hLoader) return nullptr;

    auto getApi = (ModApi * (*)())GetProcAddress(hLoader, "GetModApi");
    if (!getApi) return nullptr;

    return getApi();
#endif
}
