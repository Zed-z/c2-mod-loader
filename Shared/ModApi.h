#pragma once
/*
 * Since windres invokes the C preprocessor,
 * it's unable to find C++ headers, tell it not
 * to include C++ headers
 */
#ifndef RC_INVOKED
#include <cstddef>
#include <cstdint>
#endif
#include <windows.h>

#define API_VERSION 2

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define API_VERSION_STR STRINGIFY(API_VERSION)

#define MOD_DIRECTORY "mods"
#define MOD_DIRECTORY_L L"mods"

#define FILE_OVERRIDE_DIRECTORY "mods\\.overrides"
#define FILE_OVERRIDE_DIRECTORY_L L"mods\\.overrides"

// Addresses --------------------------------------------------------

#define ADDR_FOG_DISTANCE {0x4B7B48}
#define ADDR_RENDER_DISTANCE {0x4B7B18}

#define ADDR_ROOT_OBJ {0x4A8C3C, {0x14, 0x28, 0x00}}
#define ADDR_CROC_OBJ {0x4A8C3C, {0x14, 0x30, 0x00}}
#define ADDR_CAMERA_OBJ {0x4A8C3C, {0x14, 0x3C, 0x00}}
#define ADDR_DIALOG_OBJ {0x4A8C3C, {0x14, 0x40, 0x00}}
#define ADDR_STRAT_COUNT {0x636160}

#define ADDR_CURRENT_SAVE_SLOT {0x6220FC}
#define ADDR_SAVE_SLOT_BASE 0x6042C4
#define ADDR_SAVE_SLOT_OFFSET 0x2000
#define SAVE_SLOT_NUMBER 5

#define ADDR_INPUTS 0x52A590
#define ADDR_INPUTS_PRESSED 0x52A554
#define ADDR_INPUTS_RELEASED 0x52A558
#define ADDR_ANALOG_STRENGTH 0x52A54C

#define ADDR_CAMERA_POS_X 0x622B38
#define ADDR_CAMERA_POS_Y 0x622B3C
#define ADDR_CAMERA_POS_Z 0x622B40

#define ADDR_CAMERA_ROT_X 0x4AF328
#define ADDR_CAMERA_ROT_Y 0x4AF32C
#define ADDR_CAMERA_ROT_Z 0x4AF330

#define ADDR_CAMERA_LOOKAT_X 0x622B94
#define ADDR_CAMERA_LOOKAT_Y 0x622B98
#define ADDR_CAMERA_LOOKAT_Z 0x622B9C

#define ADDR_CONTROL_SCHEME_SLOT 0x60438C
#define ADDR_CONTROL_SCHEME_COPY1 0x52A5F4
#define ADDR_CONTROL_SCHEME_COPY2 0x52AE64

#define ADDR_CURRENT_TRIBE 0x4A8C44
#define ADDR_CURRENT_LEVEL 0x4A8C48
#define ADDR_CURRENT_MAP 0x4A8C4C

// Game structs ----------------------------------------------------------

struct Vec3i {
	int x, y, z;
};

struct Vec3fx {
	float x, y, z;
};

struct RotPos3i {
	Vec3i rotation;
	Vec3i position;
};

struct RotPos3fx {
	Vec3fx rotation;
	Vec3fx position;
};

struct Mat3x4i {
	int m[3][4];
};

typedef int32_t StratEntityFlags;

#define LOCAL_VAR_COUNT 20
struct LocalVarsStruct {
	int32_t vars[LOCAL_VAR_COUNT];
	int32_t *triggers;
};

struct StratEntity {
	StratEntity *next;
	StratEntity *prev;
	StratEntity *parent;
	int32_t *InstrStream;
	void *model;
	void *animation;
	const char *name;
	int32_t distanceToPlayer;

	union {
		struct {
			Vec3i newRotation;
			Vec3i newPosition;
		};
		RotPos3i newRotPos;
		RotPos3fx RotPosFx;
	};
	RotPos3i OldRotPos;
	Vec3i scale;
	Mat3x4i matrix0;
	RotPos3i StartRotPos;

	void *collPoints;
	int16_t collExtent;
	uint16_t collisionBoneCount;
	void *collisionBones;
	Vec3i collisionOffset;
	int32_t collRadius;

	int32_t gapField6;
	int16_t wField7;
	int16_t wField8;
	int16_t wField9;
	int16_t wField10;
	Vec3i vec0;

	StratEntityFlags flags0;
	StratEntityFlags flags1;

	void *map;

	LocalVarsStruct *localVars;
	void *triggers;

	int16_t wField0;
	int16_t wField1;
	int32_t field1;
	int32_t triggerCount;
	int32_t *StackPtr;

	int32_t Fade;
	int32_t animIndex0;
	int32_t animSpeed;
	int32_t animFrame;

	int32_t verticalVelocity;

	void *wpFirst;
	void *wpLast;
	void *wpCurrent;
	void *wpField;

	int16_t shadowSpriteIndex;
	int16_t shadowSize;

	BYTE bField2;

	BYTE blinkCount;
	BYTE blinkCountdown;
	BYTE blinkNum;

	BYTE field6[2];
	int16_t gap13[2];
	int32_t gap14[1];
};

struct SaveSlot {
	char name[4];
	uint8_t _pad1[4];
	uint32_t heartPots;
	uint32_t health;
	uint32_t crystals;
	uint8_t _pad2[8];
	char tribe[16];
	uint8_t _pad3[86];
	uint32_t binoculars;
	uint32_t keys;
	uint32_t purpleGummis;
	uint32_t blueGummis;
	uint32_t greenGummis;
	uint8_t _pad4[4];
	uint32_t clockworkGobbos;
};

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

// Api definition --------------------------------------------------------

#define MAX_OFFSETS 256
typedef struct {
	uintptr_t base;
	uintptr_t offsets[MAX_OFFSETS];
} MemoryAddress;

#define CTRL_TYPE_1 1
#define CTRL_TYPE_2 0
constexpr const char *ControlSchemeNames[] = {"Type 2", "Type 1"};

#define PHYSICS_FPS 30

typedef void (*LogFunction)(const char *message);

typedef uintptr_t (*ResolveAddressFunction)(MemoryAddress address);
typedef uintptr_t (*FindPatternFunction)(const void *pattern, size_t pattern_size, int occurrence);

typedef void (*AddressSetIntFunction)(uintptr_t address, int value);
typedef int (*AddressGetIntFunction)(uintptr_t address);

typedef bool (*PatchBytesFunction)(uintptr_t address, const void *bytes, size_t size);
typedef bool (*ReadBytesFunction)(uintptr_t address, void *out_buffer, size_t size);

#define INJECT_REPLACE 0
#define INJECT_BEFORE 1
#define INJECT_AFTER 2
typedef bool (*InjectCodeFunction)(uintptr_t hook_address, size_t hook_length, BYTE *code, size_t code_length, int inject_type);
typedef bool (*HookFunctionFunction)(uintptr_t target, size_t length, void(__stdcall *func)());
typedef bool (*HookPhysicsFunction)(void(__stdcall *func)());

typedef int (*SetupIniIntFunction)(const wchar_t *section, const wchar_t *key, int default_value);
typedef int (*ReadIniIntFunction)(const wchar_t *section, const wchar_t *key, int default_value);
typedef bool (*WriteIniIntFunction)(const wchar_t *section, const wchar_t *key, int value);

typedef bool (*SetupIniBoolFunction)(const wchar_t *section, const wchar_t *key, bool default_value);
typedef bool (*ReadIniBoolFunction)(const wchar_t *section, const wchar_t *key, bool default_value);
typedef bool (*WriteIniBoolFunction)(const wchar_t *section, const wchar_t *key, bool value);

typedef void (*SetupIniStringFunction)(const wchar_t *section, const wchar_t *key, const wchar_t *default_value, wchar_t *buffer, DWORD buffer_size);
typedef void (*ReadIniStringFunction)(const wchar_t *section, const wchar_t *key, const wchar_t *default_value, wchar_t *buffer, DWORD buffer_size);
typedef bool (*WriteIniStringFunction)(const wchar_t *section, const wchar_t *key, const wchar_t *value);

#define GAMEVER_UNKNOWN 0
#define GAMEVER_US 1
#define GAMEVER_EU 2
#define GAMEVER_DEMO 3
constexpr const char *GameVersions[] = {"UNKNOWN", "US", "EU", "DEMO"};

typedef int (*GetGameVersionFunction)();

typedef void (*ShowToastFunction)(const char *message);

struct MenuActionRegistration {
	const char *label;
	const char *tooltip;
	void(__stdcall *callback)();
	bool enabled;
};
typedef MenuActionRegistration(__stdcall *MenuActionRegistrationFunction)();
typedef bool (*RegisterMenuActionFunction)(HMODULE handle, MenuActionRegistrationFunction registration);

typedef StratEntity *(*GetEntityFunction)(MemoryAddress address);

typedef SaveSlot *(*GetSaveSlotFunction)(int slot_number);
typedef SaveSlot *(*GetCurrentSaveSlotFunction)();

typedef Inputs (*GetInputsFunction)();
typedef Inputs (*GetInputsPressedFunction)();
typedef Inputs (*GetInputsReleasedFunction)();

struct LevelInfo {
	int tribe;
	int level;
	int map;
};
typedef LevelInfo (*GetLevelInfoFunction)();

struct ModApi {
	LogFunction LogInfo;
	LogFunction LogDebug;
	LogFunction LogWarning;
	LogFunction LogError;

	ResolveAddressFunction ResolveAddress;
	FindPatternFunction FindPattern;

	AddressSetIntFunction AddressSetInt;
	AddressGetIntFunction AddressGetInt;

	PatchBytesFunction PatchBytes;
	ReadBytesFunction ReadBytes;

	InjectCodeFunction InjectCode;
	HookFunctionFunction HookFunction;
	HookPhysicsFunction HookPhysics;

	SetupIniIntFunction SetupIniInt;
	ReadIniIntFunction ReadIniInt;
	WriteIniIntFunction WriteIniInt;

	SetupIniBoolFunction SetupIniBool;
	ReadIniBoolFunction ReadIniBool;
	WriteIniBoolFunction WriteIniBool;

	SetupIniStringFunction SetupIniString;
	ReadIniStringFunction ReadIniString;
	WriteIniStringFunction WriteIniString;

	GetGameVersionFunction GetGameVersion;

	GetInputsFunction GetInputs;
	GetInputsPressedFunction GetInputsPressed;
	GetInputsReleasedFunction GetInputsReleased;

	ShowToastFunction ShowInfoToast;
	ShowToastFunction ShowDebugToast;
	ShowToastFunction ShowWarningToast;
	ShowToastFunction ShowErrorToast;
	RegisterMenuActionFunction RegisterMenuAction;

	GetEntityFunction GetEntity;

	GetSaveSlotFunction GetSaveSlot;
	GetCurrentSaveSlotFunction GetCurrentSaveSlot;

	GetLevelInfoFunction GetLevelInfo;
};

extern "C" __declspec(dllexport) ModApi *GetModApi();

// Api client --------------------------------------------------------

inline ModApi *LoadModApi(const char *moduleName = "C2ModLoader.asi") {
#ifdef IS_MOD_LOADER
	return nullptr;
#else
	HMODULE hLoader = GetModuleHandleA(moduleName);
	if (!hLoader)
		return nullptr;

	auto getApi = (ModApi * (*)()) GetProcAddress(hLoader, "GetModApi");
	if (!getApi)
		return nullptr;

	return getApi();
#endif
}
