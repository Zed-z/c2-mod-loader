#pragma once
#include <string>
#include <Windows.h>

// Api definition --------------------------------------------------------

#define MAX_OFFSETS 256
typedef struct {
    uintptr_t base;
    uintptr_t offsets[MAX_OFFSETS];
} MemoryAddress;

typedef void(*LogFunction)(const std::string&);
typedef uintptr_t(*ResolveAddressFunction)(MemoryAddress);
typedef void(*AddressSetIntFunction)(uintptr_t, int);
typedef int(*AddressGetIntFunction)(uintptr_t);

struct ModApi {
    LogFunction Log;
    ResolveAddressFunction ResolveAddress;
    AddressSetIntFunction AddressSetInt;
    AddressGetIntFunction AddressGetInt;
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
