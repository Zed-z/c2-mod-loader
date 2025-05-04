#pragma once
#include <string>
#include <Windows.h>

// Api definition --------------------------------------------------------
typedef void(*LogFunction)(const std::string&);
typedef int(*GetAddressFunction)(int);
typedef void(*SetAddressFunction)(int, int);

struct ModApi {
    LogFunction Log;
    GetAddressFunction GetAddress;
    SetAddressFunction SetAddress;
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
