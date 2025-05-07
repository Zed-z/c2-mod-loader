#pragma once
#include <string>
#include <Windows.h>

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

typedef void(*LogFunction)(const std::string& message);

typedef uintptr_t(*ResolveAddressFunction)(MemoryAddress address);

typedef void(*AddressSetIntFunction)(uintptr_t address, int value);
typedef int(*AddressGetIntFunction)(uintptr_t address);

typedef bool(*PatchBytesFunction)(uintptr_t address, const void* bytes, size_t size);
typedef bool(*ReadBytesFunction)(uintptr_t address, void* outBuffer, size_t size);

struct ModApi {
    LogFunction Log;

    ResolveAddressFunction ResolveAddress;

    AddressSetIntFunction AddressSetInt;
    AddressGetIntFunction AddressGetInt;

    PatchBytesFunction PatchBytes;
    ReadBytesFunction ReadBytes;
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
