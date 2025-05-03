#pragma once
#include <string>
#include <Windows.h>

// Api definition --------------------------------------------------------
typedef void(*LogFunction)(const std::string&);

struct ModApi {
    LogFunction Log;
};

extern "C" __declspec(dllexport) ModApi * GetModApi();


// Api client --------------------------------------------------------
inline ModApi* LoadSharedModApi(const char* moduleName = "C2ModLoader.asi") {
#ifdef IS_MOD_LOADER
    return &g_ModApi;
#else
    HMODULE hLoader = GetModuleHandleA(moduleName);
    if (!hLoader) return nullptr;

    auto getApi = (ModApi * (*)())GetProcAddress(hLoader, "GetModApi");
    if (!getApi) return nullptr;

    return getApi();
#endif
}