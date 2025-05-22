#pragma once
#include <Windows.h>


inline std::wstring StringToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstrTo[0], size_needed);
    return wstrTo;
}

inline std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, nullptr, nullptr);
    return strTo;
}


struct PathInfo {
    std::wstring path;       // Full path
    std::wstring filename;   // Filename only
    std::wstring name;       // Filename without extension
    std::wstring directory;  // Directory only
};

inline PathInfo GetPathInfo(std::wstring path) {

    size_t lastBackslashPos = path.find_last_of(L"\\/");

    std::wstring filename = path.substr(lastBackslashPos + 1);
    std::wstring directory = path.substr(0, lastBackslashPos);

    size_t extensionPos = filename.find(L".asi");
    std::wstring name = filename;
    if (extensionPos != std::wstring::npos) {
        name = filename.substr(0, extensionPos);
    }

    return {
        path, filename, name, directory
    };
}

inline PathInfo GetModuleFilepath(HMODULE module) {
    wchar_t filePath[MAX_PATH];
    GetModuleFileNameW(module, filePath, MAX_PATH);
    std::wstring filePathStr(filePath);
    return GetPathInfo(filePathStr);
}
