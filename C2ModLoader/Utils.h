#pragma once
#include <Windows.h>
#include <string>
#include <vector>


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


#pragma comment(lib, "Version.lib")
struct FileVersionInfo {
    std::wstring fileVersion;
    std::wstring productVersion;
    std::wstring companyName;
    std::wstring productName;
    std::wstring fileDescription;
};

inline FileVersionInfo GetFileVersionInfo(const std::wstring filename) {
    FileVersionInfo info;

    // Get size, return early if needed
    DWORD sizeHandle;
    DWORD size = GetFileVersionInfoSizeW(filename.c_str(), &sizeHandle);
    if (size == 0) {
        return info;
    }

    // Get data, return early if needed
    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(filename.c_str(), 0, size, data.data())) {
        return info;
    }

    // English + Unicode
    const wchar_t* langCode = L"040904B0";

    // Lambda to query values
    auto queryValue = [&](const wchar_t* key) -> std::wstring {
        wchar_t query[100];
        swprintf(query, 100, L"\\StringFileInfo\\%s\\%s", langCode, key);

        LPVOID value = nullptr;
        UINT size = 0;
        if (VerQueryValueW(data.data(), query, &value, &size) && value) {
            return std::wstring((wchar_t*)value);
        }

        return L"";
    };

    // Fill in fields and return the struct
    info.fileVersion = queryValue(L"FileVersion");
    info.productVersion = queryValue(L"ProductVersion");
    info.companyName = queryValue(L"CompanyName");
    info.productName = queryValue(L"ProductName");
    info.fileDescription = queryValue(L"FileDescription");
    return info;
}