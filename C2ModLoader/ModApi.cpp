#define IS_MOD_LOADER
#include "ModApi.h"
#include "Config.h"
#include "ConsoleLogging.h"
#include "GameHooks.h"
#include "Input/Input.h"
#include "Loader.h"
#include "Overlay/Components/MenuBar.h"
#include "Overlay/Components/Toast.h"
#include "Overlay/Overlay.h"
#include "Resource.h"
#include "Utils.h"
#include "Utils/Sha256.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#include <mutex>

namespace fs = std::filesystem;

void LogRaw(const std::string &message, const LogSeverity &severity, const HMODULE &modHandle, const std::string &prefix = "") {

	// Get module path
	PathInfo pathInfo = GetModuleFilepath(modHandle);

	// Get log path - ignore directory override
	HMODULE executable = GetModuleHandleA(NULL);
	PathInfo executableFilepath = GetModuleFilepath(executable);
	fs::path p(executableFilepath.directory + L"\\" + StringToWString(LOG_FILE));

	// Open log
	std::wofstream log{p, std::ios::app};
	if (!log.is_open())
		return;

	// Get current time
	auto now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);
	std::tm timeInfo;
	localtime_s(&timeInfo, &now_c);

	// Format and write the timestamp and message
	std::wstringstream msg;
	msg << L"[" << std::put_time(&timeInfo, L"%Y-%m-%d %H:%M:%S") << L"] [" << StringToWString(prefix) << pathInfo.name << L"] " << StringToWString(message) << std::endl;

	// Write the log to file, console, and buffer
	{
		std::lock_guard<std::mutex> lock(logMutex);
		log << msg.str();
		ConsoleLogging::LogToConsole(msg.str(), severity);
		logMessages.push_back({WStringToString(msg.str()), severity});
	}

	// Close log
	log.close();
}

void LogInfo(const char *message) {
	LogRaw(message != nullptr ? message : "", LogSeverity::Info, GetCallingModule(), "INFO    | ");
}

void LogDebug(const char *message) {
	LogRaw(message != nullptr ? message : "", LogSeverity::Debug, GetCallingModule(), "DEBUG   | ");
}

void LogWarning(const char *message) {
	LogRaw(message != nullptr ? message : "", LogSeverity::Warning, GetCallingModule(), "WARNING | ");
}

void LogError(const char *message) {
	LogRaw(message != nullptr ? message : "", LogSeverity::Error, GetCallingModule(), "ERROR   | ");
}

uintptr_t ResolveAddress(MemoryAddress address) {
	uintptr_t addr = address.base;

	for (size_t i = 0; i < MAX_OFFSETS; ++i) {

		// End of offsets
		if (address.offsets[i] == 0)
			break;

		if (IsBadReadPtr((void *)addr, sizeof(uintptr_t))) {
			// Log("Bad read at: " + std::to_string(addr));
			return 0;
		}

		addr = *(uintptr_t *)addr;

		if (addr == 0) {
			// Log("Null pointer during chain at offset index: " + std::to_string(i));
			return 0;
		}

		addr += address.offsets[i];
	}

	if (IsBadReadPtr((void *)addr, sizeof(uintptr_t))) {
		// Log("Bad address at: " + std::to_string(addr));
		return 0;
	}

	return addr;
}

void AddressSetInt(uintptr_t address, int value) {
	*(int *)address = value;

#ifdef DEBUG
	std::ostringstream stream;
	stream << "Memory at " << address << " set to " << value;
	LogInfo(stream.str());
#endif
}

int AddressGetInt(uintptr_t address) {

	int value = *(int *)address;

#ifdef DEBUG
	std::ostringstream stream;
	stream << "Memory at " << address << " is " << value;
	LogInfo(stream.str());
#endif

	return value;
}

bool PatchBytes(uintptr_t address, const void *bytes, size_t size) {
	DWORD protect;
	if (!VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &protect)) {
		return false;
	}

	memcpy((LPVOID)address, bytes, size);

	DWORD temp;
	VirtualProtect((LPVOID)address, size, protect, &temp);
	return true;
}

bool ReadBytes(uintptr_t address, void *out_buffer, size_t size) {
	DWORD protect;
	if (!VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &protect)) {
		return false;
	}

	memcpy(out_buffer, (LPVOID)address, size);

	DWORD temp;
	VirtualProtect((LPVOID)address, size, protect, &temp);
	return true;
}

uintptr_t FindPattern(const void *pattern, size_t pattern_size, int occurrence) {
	HMODULE hModule = GetModuleHandle(nullptr);
	if (!hModule)
		return 0;

	MODULEINFO modInfo;
	if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
		return 0;

	uintptr_t start = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
	uintptr_t end = start + modInfo.SizeOfImage;

	int count = 0;

	for (uintptr_t addr = start; addr <= end - pattern_size; ++addr) {
		if (memcmp(reinterpret_cast<void *>(addr), pattern, pattern_size) == 0) {
			if (++count == occurrence)
				return addr;
		}
	}

	return 0;
}

#define JMP_SIZE 5 // 1 byte for the instruction, 4 for the address

bool InjectCode(uintptr_t hook_address, size_t hook_length, BYTE *code, size_t code_length, int inject_type) {

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
		memcpy((void *)newmem, code, code_length);
		memcpy((void *)(newmem + code_length), (void *)hook_address, hook_length);
		break;
	}
	case INJECT_REPLACE: {
		memcpy((void *)newmem, code, code_length);
		break;
	}
	case INJECT_AFTER: {
		memcpy((void *)newmem, (void *)hook_address, hook_length);
		memcpy((void *)(newmem + hook_length), code, code_length);
		break;
	}
	}

	// Add jumpback instruction
	uintptr_t returnAddress = hook_address + hook_length;
	uintptr_t jmpReturnOffset = returnAddress - (newmem + totalSize);

	BYTE jumpback[] = {0xE9, 0x00, 0x00, 0x00, 0x00};
	*(int32_t *)&jumpback[1] = (int32_t)jmpReturnOffset;
	memcpy((void *)(newmem + totalSize - JMP_SIZE), jumpback, JMP_SIZE);

	// Prepare the patch
	BYTE *patch = new BYTE[hook_length];
	memset(patch, 0x90, hook_length); // Fill with NOP

	// Calculate the position of the next instructions as an offset
	int32_t rel_jmp_to_newmem = (int32_t)((uintptr_t)newmem - hook_address - JMP_SIZE);

	BYTE jump[] = {0xE9, 0x00, 0x00, 0x00, 0x00};
	*(int32_t *)&jump[1] = (int32_t)rel_jmp_to_newmem;
	memcpy(patch, jump, JMP_SIZE);

	// Replace the instruction at the address with the hook
	DWORD protect;
	VirtualProtect((LPVOID)hook_address, hook_length, PAGE_EXECUTE_READWRITE, &protect);
	memcpy((void *)hook_address, patch, hook_length);
	VirtualProtect((LPVOID)hook_address, hook_length, protect, &protect);

	// Cleanup
	delete[] patch;
	return true;
}

#define PUSHAD 0x60
#define POPAD 0x61
#define PUSHFD 0x9C
#define POPFD 0x9D

// Allows you to put arbitrary C++ code
bool HookFunction(uintptr_t target, size_t length, void(__stdcall *func)(), int inject_type) {
	// Target needs to be able to fit a jump
	if (length < JMP_SIZE)
		return false;

	// Calculate trampoline size based on inject_type
	size_t trampoline_size = 0;
	switch (inject_type) {
	case INJECT_BEFORE:
	case INJECT_AFTER:
		trampoline_size = length + 1 + 1 + JMP_SIZE + 1 + 1 + JMP_SIZE + length;
		break;
	case INJECT_REPLACE:
	default:
		trampoline_size = 1 + 1 + JMP_SIZE + 1 + 1 + JMP_SIZE;
		break;
	}

	BYTE *trampoline = (BYTE *)VirtualAlloc(
		nullptr, trampoline_size,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);
	if (!trampoline)
		return false;

	BYTE *p = trampoline;

	switch (inject_type) {
	case INJECT_BEFORE: {
		*p++ = PUSHAD;
		*p++ = PUSHFD;

		*p = 0xE8;
		int32_t rel_func = (int32_t)((uintptr_t)func - ((uintptr_t)p + JMP_SIZE));
		memcpy(p + 1, &rel_func, 4);
		p += JMP_SIZE;

		*p++ = POPFD;
		*p++ = POPAD;

		memcpy(p, (void *)target, length);
		p += length;
		break;
	}
	case INJECT_AFTER: {
		memcpy(p, (void *)target, length);
		p += length;

		*p++ = PUSHAD;
		*p++ = PUSHFD;

		*p = 0xE8;
		int32_t rel_func = (int32_t)((uintptr_t)func - ((uintptr_t)p + JMP_SIZE));
		memcpy(p + 1, &rel_func, 4);
		p += JMP_SIZE;

		*p++ = POPFD;
		*p++ = POPAD;
		break;
	}
	case INJECT_REPLACE: { // INJECT_REPLACE
		*p++ = PUSHAD;
		*p++ = PUSHFD;

		*p = 0xE8;
		int32_t rel_func = (int32_t)((uintptr_t)func - ((uintptr_t)p + JMP_SIZE));
		memcpy(p + 1, &rel_func, 4);
		p += JMP_SIZE;

		*p++ = POPFD;
		*p++ = POPAD;
		break;
	}
	}

	// Jump back to target + length
	*p = 0xE9;
	int32_t rel_jumpback = (int32_t)((int64_t)(target + length) - ((uintptr_t)p + JMP_SIZE));
	memcpy(p + 1, &rel_jumpback, 4);

	// Prepare patch (NOPs + JMP to trampoline)
	BYTE patch[16];
	memset(patch, 0x90, length); // NOPs
	patch[0] = 0xE9;
	int32_t rel_trampoline = (int32_t)((uintptr_t)trampoline - (target + JMP_SIZE));
	memcpy(patch + 1, &rel_trampoline, 4);

	// Apply patch
	DWORD old;
	VirtualProtect((void *)target, length, PAGE_EXECUTE_READWRITE, &old);
	memcpy((void *)target, patch, length);
	VirtualProtect((void *)target, length, old, &old);

	return true;
}

void HookGame(int hook_type, void(__stdcall *func)()) {
	GameHooks::RegisterHook(hook_type, func);
}

// Anonymous namespace for INI helpers
namespace {

std::wstring GetIniPathForModule(HMODULE caller) {
	PathInfo pathInfo = GetModuleFilepath(caller);
	return std::regex_replace(pathInfo.path, std::wregex(L".asi$"), L".ini");
}

int ReadIniIntForModule(const std::wstring &section, const std::wstring &key, int default_value, HMODULE caller) {
	std::wstring iniPath = GetIniPathForModule(caller);
	return GetPrivateProfileIntW(section.c_str(), key.c_str(), default_value, iniPath.c_str());
}

bool WriteIniIntForModule(const std::wstring &section, const std::wstring &key, int value, HMODULE caller) {
	std::wstring iniPath = GetIniPathForModule(caller);
	wchar_t valueStr[16];
	_itow_s(value, valueStr, 10);
	return WritePrivateProfileStringW(section.c_str(), key.c_str(), valueStr, iniPath.c_str()) != 0;
}

bool ReadIniBoolForModule(const std::wstring &section, const std::wstring &key, bool default_value, HMODULE caller) {
	std::wstring iniPath = GetIniPathForModule(caller);
	return (bool)GetPrivateProfileIntW(section.c_str(), key.c_str(), (int)default_value, iniPath.c_str());
}

bool WriteIniBoolForModule(const std::wstring &section, const std::wstring &key, bool value, HMODULE caller) {
	std::wstring iniPath = GetIniPathForModule(caller);
	wchar_t valueStr[16];
	_itow_s((int)value, valueStr, 10);
	return WritePrivateProfileStringW(section.c_str(), key.c_str(), valueStr, iniPath.c_str()) != 0;
}

float ReadIniFloatForModule(const std::wstring &section, const std::wstring &key, float default_value, HMODULE caller) {
	std::wstring iniPath = GetIniPathForModule(caller);
	wchar_t defaultValueStr[64] = {};
	swprintf_s(defaultValueStr, L"%.8g", default_value);

	wchar_t valueStr[64] = {};
	GetPrivateProfileStringW(section.c_str(), key.c_str(), defaultValueStr, valueStr, _countof(valueStr), iniPath.c_str());

	wchar_t *parseEnd = nullptr;
	const float parsedValue = std::wcstof(valueStr, &parseEnd);
	if (parseEnd == valueStr) {
		return default_value;
	}
	return parsedValue;
}

bool WriteIniFloatForModule(const std::wstring &section, const std::wstring &key, float value, HMODULE caller) {
	std::wstring iniPath = GetIniPathForModule(caller);
	wchar_t valueStr[64] = {};
	swprintf_s(valueStr, L"%.8g", value);
	return WritePrivateProfileStringW(section.c_str(), key.c_str(), valueStr, iniPath.c_str()) != 0;
}

void ReadIniStringForModule(const std::wstring &section, const std::wstring &key, const wchar_t *default_value, wchar_t *buffer, DWORD buffer_size, HMODULE caller) {
	std::wstring iniPath = GetIniPathForModule(caller);
	GetPrivateProfileStringW(section.c_str(), key.c_str(), default_value, buffer, buffer_size, iniPath.c_str());
}

bool WriteIniStringForModule(const std::wstring &section, const std::wstring &key, const wchar_t *value, HMODULE caller) {
	std::wstring iniPath = GetIniPathForModule(caller);
	return WritePrivateProfileStringW(section.c_str(), key.c_str(), value, iniPath.c_str()) != 0;
}
} // namespace

int ReadIniInt(const wchar_t *section, const wchar_t *key, int default_value) {
	HMODULE caller = GetCallingModule();
	return ReadIniIntForModule(section != nullptr ? section : L"", key != nullptr ? key : L"", default_value, caller);
}

bool WriteIniInt(const wchar_t *section, const wchar_t *key, int value) {
	HMODULE caller = GetCallingModule();
	return WriteIniIntForModule(section != nullptr ? section : L"", key != nullptr ? key : L"", value, caller);
}

int SetupIniInt(const wchar_t *section, const wchar_t *key, int default_value) {
	HMODULE caller = GetCallingModule();
	const wchar_t *resolvedSection = section != nullptr ? section : L"";
	const wchar_t *resolvedKey = key != nullptr ? key : L"";
	int value = ReadIniIntForModule(resolvedSection, resolvedKey, default_value, caller);
	WriteIniIntForModule(resolvedSection, resolvedKey, value, caller);
	return value;
}

bool ReadIniBool(const wchar_t *section, const wchar_t *key, bool default_value) {
	HMODULE caller = GetCallingModule();
	return ReadIniBoolForModule(section != nullptr ? section : L"", key != nullptr ? key : L"", default_value, caller);
}

bool WriteIniBool(const wchar_t *section, const wchar_t *key, bool value) {
	HMODULE caller = GetCallingModule();
	return WriteIniBoolForModule(section != nullptr ? section : L"", key != nullptr ? key : L"", value, caller);
}

bool SetupIniBool(const wchar_t *section, const wchar_t *key, bool default_value) {
	HMODULE caller = GetCallingModule();
	const wchar_t *resolvedSection = section != nullptr ? section : L"";
	const wchar_t *resolvedKey = key != nullptr ? key : L"";
	int value = ReadIniBoolForModule(resolvedSection, resolvedKey, default_value, caller);
	WriteIniBoolForModule(resolvedSection, resolvedKey, value, caller);
	return value;
}

float ReadIniFloat(const wchar_t *section, const wchar_t *key, float default_value) {
	HMODULE caller = GetCallingModule();
	return ReadIniFloatForModule(section != nullptr ? section : L"", key != nullptr ? key : L"", default_value, caller);
}

bool WriteIniFloat(const wchar_t *section, const wchar_t *key, float value) {
	HMODULE caller = GetCallingModule();
	return WriteIniFloatForModule(section != nullptr ? section : L"", key != nullptr ? key : L"", value, caller);
}

float SetupIniFloat(const wchar_t *section, const wchar_t *key, float default_value) {
	HMODULE caller = GetCallingModule();
	const wchar_t *resolvedSection = section != nullptr ? section : L"";
	const wchar_t *resolvedKey = key != nullptr ? key : L"";
	float value = ReadIniFloatForModule(resolvedSection, resolvedKey, default_value, caller);
	WriteIniFloatForModule(resolvedSection, resolvedKey, value, caller);
	return value;
}

void ReadIniString(const wchar_t *section, const wchar_t *key, const wchar_t *default_value, wchar_t *buffer, DWORD buffer_size) {
	HMODULE caller = GetCallingModule();
	ReadIniStringForModule(section != nullptr ? section : L"", key != nullptr ? key : L"", default_value != nullptr ? default_value : L"", buffer, buffer_size, caller);
}

bool WriteIniString(const wchar_t *section, const wchar_t *key, const wchar_t *value) {
	HMODULE caller = GetCallingModule();
	return WriteIniStringForModule(section != nullptr ? section : L"", key != nullptr ? key : L"", value != nullptr ? value : L"", caller);
}

void SetupIniString(const wchar_t *section, const wchar_t *key, const wchar_t *default_value, wchar_t *buffer, DWORD buffer_size) {
	HMODULE caller = GetCallingModule();
	const wchar_t *resolvedSection = section != nullptr ? section : L"";
	const wchar_t *resolvedKey = key != nullptr ? key : L"";
	const wchar_t *resolvedDefault = default_value != nullptr ? default_value : L"";
	ReadIniStringForModule(resolvedSection, resolvedKey, resolvedDefault, buffer, buffer_size, caller);
	WriteIniStringForModule(resolvedSection, resolvedKey, buffer, caller);
}

int GetGameVersion() {
	static int cachedVersion = GAMEVER_UNCHECKED;

	if (cachedVersion != GAMEVER_UNCHECKED) {
		return cachedVersion;
	}

	char path[MAX_PATH];
	GetModuleFileNameA(nullptr, path, MAX_PATH);

	std::string hash = Sha256::ComputeFileHash(path);
	if (hash.empty()) {
		cachedVersion = GAMEVER_UNKNOWN;
		return cachedVersion;
	}

	std::transform(hash.begin(), hash.end(), hash.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

	if (hash == "BAF84A039DB9581C56A52A8C1C5A318BCE13C3A72565FB6AC19B226D926BE0DE")
		cachedVersion = GAMEVER_US;
	else if (hash == "38BDFCFF386F52824A718DE70B5710CF32F2263DD9C23FEED4FB39AE741C1720")
		cachedVersion = GAMEVER_EU;
	else if (hash == "074C0E4333C76635D3F214E6D89AB2F758FE233E6D4B718F7C5A7E0F11B7D24B")
		cachedVersion = GAMEVER_DEMO;
	else
		cachedVersion = GAMEVER_UNKNOWN;

	return cachedVersion;
}

Inputs GetInputsRaw(uintptr_t address) {

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
	return GetInputsRaw((uintptr_t)inputsRaw);
}

Inputs GetInputsPressed() {
	return GetInputsRaw((uintptr_t)inputsPressedRaw);
}

Inputs GetInputsReleased() {
	return GetInputsRaw((uintptr_t)inputsReleasedRaw);
}

void ShowInfoToast(const char *message) {
	ImGuiShowToast(message != nullptr ? message : "", LogSeverity::Info);
}

void ShowDebugToast(const char *message) {
	ImGuiShowToast(message != nullptr ? message : "", LogSeverity::Debug);
}

void ShowWarningToast(const char *message) {
	ImGuiShowToast(message != nullptr ? message : "", LogSeverity::Warning);
}

void ShowErrorToast(const char *message) {
	ImGuiShowToast(message != nullptr ? message : "", LogSeverity::Error);
}

bool RegisterMenuAction(HMODULE handle, MenuActionRegistrationFunction registration) {
	return ImGuiRegisterMenuAction(handle, registration);
}

StratEntity *GetEntity(MemoryAddress address) {
	StratEntity *entity = nullptr;

	uintptr_t addr = api->ResolveAddress(address);

	if (addr == 0)
		return nullptr;
	if (IsBadReadPtr((void *)addr, sizeof(StratEntity)))
		return nullptr;

	entity = (StratEntity *)addr;

	if (entity->next == nullptr)
		return nullptr;
	if (IsBadReadPtr((void *)(entity->next), sizeof(StratEntity)))
		return nullptr;

	// There's probably a better way to do this
	entity = entity->next;

	return entity;
}

StratEntity *FindEntity(const char *name) {
	StratEntity *current = GetEntity(rootObjRef);
	if (current == nullptr)
		return nullptr;

	while (current->prev != nullptr)
		current = current->prev;

	while (current != nullptr) {
		if (strcmp(current->name, name) == 0) {
			return current;
		}
		current = current->next;
	}

	return nullptr;
}

SaveSlot *GetSaveSlot(int slot_number) {
	return (SaveSlot *)((uintptr_t)saveSlots + slot_number * 0x2000);
}

SaveSlot *GetCurrentSaveSlot() {
	return GetSaveSlot(*currentSaveSlotIndex);
}

LevelInfo GetLevelInfo() {
	return *levelInfo;
}

ModernInput GetModernInputState() {
	return Input::GetState();
}

void GotoLevel(int tribe, int level, int map, WadFileType type) {
	SaveSlot *slot = api->GetCurrentSaveSlot();

	slot->tribe0 = tribe;
	slot->level0 = level;
	slot->map0 = map;
	slot->type0 = type;

	*gameStateComingFrom = *gameState;
	*gameStateGoingTo = GS_LOADING;
	*gameStateTransitioning = true;

	if (*fadeRoutine == nullptr) {
		initFade(FADE_NormalToBlack);
	}
}

void GotoLevelSelect() {
	if (api->GetGameVersion() == GAMEVER_DEMO) {
		api->LogWarning("Level select not available in the demo!");
		api->ShowWarningToast("Level select not available in the demo!");
		return;
	}

	*gameStateComingFrom = *gameState;
	*gameStateGoingTo = GS_LEVEL_SELECT;
	*gameStateTransitioning = true;

	if (*fadeRoutine == nullptr) {
		initFade(FADE_NormalToBlack);
	}
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
	HookGame,
	SetupIniInt,
	ReadIniInt,
	WriteIniInt,
	SetupIniBool,
	ReadIniBool,
	WriteIniBool,
	SetupIniFloat,
	ReadIniFloat,
	WriteIniFloat,
	SetupIniString,
	ReadIniString,
	WriteIniString,
	GetGameVersion,
	GetInputs,
	GetInputsPressed,
	GetInputsReleased,
	ShowInfoToast,
	ShowDebugToast,
	ShowWarningToast,
	ShowErrorToast,
	RegisterMenuAction,
	GetEntity,
	FindEntity,
	GetSaveSlot,
	GetCurrentSaveSlot,
	GetLevelInfo,
	GetModernInputState,
	GotoLevel,
	GotoLevelSelect,
};

extern "C" __declspec(dllexport) ModApi *GetModApi() {
	return &g_ModApi;
}
