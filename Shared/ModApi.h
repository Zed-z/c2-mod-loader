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

// Game structs ----------------------------------------------------------

#define MAX_OFFSETS 16
typedef struct {
	uintptr_t base;
	uintptr_t offsets[MAX_OFFSETS];
} MemoryAddress;

typedef struct {
	MemoryAddress us;
	MemoryAddress eu;
	MemoryAddress demo;
} VersionAddresses;

typedef int32_t StratEntityFlags;
typedef int32_t bool32_t;
typedef int16_t bool16_t;

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

enum DoorRotY : uint32_t {
	DOOR_DOWN = 0x1,
	DOOR_UP = 0x2,
	DOOR_LEVEL = 0x4,
	DOOR_START = 0x8,
};

enum WadFileType : int32_t {
	WAD_TYPE_INVALID = -1,
	WAD_TYPE_LEVEL = 0x0,
	WAD_TYPE_BOSS = 0x1,
	WAD_TYPE_SECRET = 0x2,
	WAD_TYPE_CUTSCENE = 0x3,
};

constexpr const char *wadFileTypeNames[] = {
	"Invalid",
	"Level",
	"Boss",
	"Secret",
	"Cutscene",
};

struct ColorBGRA32 {
	uint8_t b, g, r, a;
};

using ModelStruct = void;

struct DoorStruct {
	Vec3i position;
	int GotoTribe;
	int GotoLevel;
	int GotoMap;
	WadFileType GotoType;
	DoorRotY GotoRotY;
	DoorRotY ThisRotY;
	int Fade;
	ModelStruct *Background;
	int BackgroundAddYRotation;
	int BackgroundHeightAdjust;
	int DrawMode;
	int16_t MoveForward;
	int16_t __padding;
	int renderDistance;
	int fogDistance;
	int field5;
	int field6;
	ColorBGRA32 AmbientLight;
	ColorBGRA32 BackColor;
	uint16_t EffectFlags;
	uint16_t ReverbType;
	int MusicTrack;
};

struct StratEntity {
	StratEntity *next;
	StratEntity *prev;
	StratEntity *parent;
	int32_t *InstrStream;
	ModelStruct *model;
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

	/*int localVarCount = ((Int32*)strat->triggers - (Int32*)strat->LocalVars) - 20;*/
	int32_t *localVars;
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

#define SAVE_SLOT_NUMBER 5
struct SaveSlot {
	uint8_t byte0;
	uint8_t byte1;
	uint8_t byte2;
	uint8_t byte3;

	uint16_t Title[32];
	uint8_t reserve[28];
	uint16_t Clut[16];
	uint8_t Icon[384];

	uint8_t Initial1;
	uint8_t Initial2;
	uint8_t Initial3;
	uint8_t Pad00;

	char name[8];

	int heartPots;
	int health;
	uint32_t crystals;

	int _pad0;
	int _pad1;

	char fileName[32];

	int16_t effectVolume;
	int16_t musicVolume;
	int16_t dialogVolume;
	int16_t adsSpeakerConfig;

	int _pad2;

	int tribe0;
	int level0;
	int map0;
	WadFileType type0;

	int _pad3;

	int Language;
	int vibration;

	int totalBossHearts;
	int bossHearts;

	uint32_t _pad4;

	int CameraMode;
	int goldenGobbos;

	int _pad5;

	int jigsawPieces;
	int levelCrystals;

	int binoculars;
	int keys;
	union {
		int purpleGummis;
		int redJellies;
	};
	union {
		int blueGummis;
		int orangeJellies;
	};
	union {
		int greenGummis;
		int greenJellies;
	};
	int tempItems;
	int clockworkGobbos;
	int unknownItem;
	int wheels;

	int itemCount;
	int selectedItem;

	int doorTribe;
	int doorLevel;
	int doorMap;
	int doorType;

	int rewardPoints;

	int analogOn;

	uint32_t controlMethod;

	uint32_t levelFlags[10][10][4];

	uint8_t _pad6[100];

	int _pad7;

	int saveChecksum;
};

enum GameState : uint32_t {
	GS_NONE = 0x0,
	GS_SELECT_LOAD_SLOT = 0x1,
	GS_SELECT_SAVE_SLOT = 0x2,
	GS_CONTINUE = 0x3,
	GS_CREDITS = 0x4,
	GS_DEMO_MODE = 0x5,
	GS_SHUTDOWN = 0x6,
	GS_SHUTDOWN_2 = 0x7,
	GS_INITIALIZE = 0x8,
	GS_ENTER_INITIALS = 0x9,
	GS_UNKNOWN_10 = 0xA,
	GS_LOADING = 0xB,
	GS_GAME_OVER = 0xC,
	GS_LANGUAGE_SELECT = 0xD,
	GS_OPTIONS_MENU = 0xE,
	GS_UNKNOWN_15 = 0xF,
	GS_MAIN_MENU = 0x10,
	GS_UNKNOWN_17 = 0x11,
	GS_LEVEL_SELECT = 0x12,
	GS_UNKNOWN_19 = 0x13,
	GS_UNKNOWN_20 = 0x14,
};

constexpr const char *gameStateNames[] = {
	"None",
	"Select Load Slot",
	"Select Save Slot",
	"Continue",
	"Credits",
	"Demo Mode",
	"Shutdown",
	"Shutdown 2",
	"Initialize",
	"Enter Initials",
	"Unknown 10",
	"Loading",
	"Game Over",
	"Language Select",
	"Options Menu",
	"Unknown 15",
	"Main Menu",
	"Unknown 17",
	"Level Select",
	"Unknown 19",
	"Unknown 20",
};

typedef void (*GotoLevelFunction)(int tribe, int level, int map, WadFileType type);
typedef void (*GotoLevelSelectFunction)();

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

typedef void(__cdecl *fnFadeRoutine)();
typedef void(__cdecl *fnInitFade)(uint32_t fadeType);
#define FADE_NormalToBlack 1

struct CameraStackEntry {
	StratEntity *target;
	StratEntity *camera;
	RotPos3i rotPos0;
	RotPos3i rotPos1;
	Vec3i Goto;
};

typedef void MapStrat;
typedef void WorldCell;
typedef void Waypoint;
typedef void PieceStruct;
typedef void LightStruct;
typedef uint32_t WadFlags;
typedef void FontStruct;
typedef void ADSData;

struct WadChunkData {
	byte *fileData;
	StratEntity *FirstStrat;
	StratEntity *RootEntity;
	StratEntity *Player;
	StratEntity *OldPlayer;
	StratEntity *TargetStrat;
	StratEntity *TargetStrat2;
	StratEntity *Boss;
	StratEntity *Camera;
	StratEntity *Dialog;
	CameraStackEntry cameraStack[8];
	uint32_t cameraStackCount;
};

struct WadChunk {
	MapStrat *Background;
	WorldCell ***worldCells;
	int cellCountTotal;
	int cellCountX;
	int cellCountZ;
	int lightCount;
	LightStruct **lights;
	WadChunkData data;
	int gap1[3];
	int *ZoneData;
	RotPos3i *Positions;
	int *modelIndices;
	int gap3[2];
	int doorCount;
	DoorStruct *doors;
	int alienCount;
	Waypoint *aliens;
	int StratCount;
	int MaxStrats;
	MapStrat *MapStrats;
	int ParamCount;
	int *Params;
	PieceStruct *pieces;
	int pieceCount;
	int16_t wField2;
	int16_t wField3;
	int array3Count;
	char (*strArray)[0x30];
};

struct WadFile {
	WadFlags WadFlags;
	uint32_t tribe;
	uint32_t level;
	uint32_t map;
	WadFileType type;
	WadChunk *chunkData;
	FontStruct *font;
	uint8_t stratLoadedMaybe;
	uint8_t bytes1[3];
	ADSData **soundCount; // Named incorrectly
	uint32_t adsElement0Count;
	ADSData **adsResources;
	uint32_t adsResourceCount;
	char ***LanguageStrings;
	uint32_t languageCount;
	uint32_t languageStringCount;
	uint32_t **languageStringLengths;
};

// Api definition --------------------------------------------------------

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
typedef bool (*HookFunctionFunction)(uintptr_t target, size_t length, void(__stdcall *func)(), int inject_type);

#define GAME_HOOK_POST_STEP 0
#define GAME_HOOK_DOOR_CHANGE 1
#define GAME_HOOK_MAP_CHANGE 2
#define GAME_HOOK_PLAYER_DEATH 3
#define GAME_HOOK_PRE_STEP 4
#define GAME_HOOK_PRE_INPUT 5
typedef void (*HookGameFunction)(int hook_type, void(__stdcall *func)());

typedef int (*SetupIniIntFunction)(const wchar_t *section, const wchar_t *key, int default_value);
typedef int (*ReadIniIntFunction)(const wchar_t *section, const wchar_t *key, int default_value);
typedef bool (*WriteIniIntFunction)(const wchar_t *section, const wchar_t *key, int value);

typedef bool (*SetupIniBoolFunction)(const wchar_t *section, const wchar_t *key, bool default_value);
typedef bool (*ReadIniBoolFunction)(const wchar_t *section, const wchar_t *key, bool default_value);
typedef bool (*WriteIniBoolFunction)(const wchar_t *section, const wchar_t *key, bool value);

typedef float (*SetupIniFloatFunction)(const wchar_t *section, const wchar_t *key, float default_value);
typedef float (*ReadIniFloatFunction)(const wchar_t *section, const wchar_t *key, float default_value);
typedef bool (*WriteIniFloatFunction)(const wchar_t *section, const wchar_t *key, float value);

typedef void (*SetupIniStringFunction)(const wchar_t *section, const wchar_t *key, const wchar_t *default_value, wchar_t *buffer, DWORD buffer_size);
typedef void (*ReadIniStringFunction)(const wchar_t *section, const wchar_t *key, const wchar_t *default_value, wchar_t *buffer, DWORD buffer_size);
typedef bool (*WriteIniStringFunction)(const wchar_t *section, const wchar_t *key, const wchar_t *value);

#define GAMEVER_UNCHECKED -1
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
typedef StratEntity *(*FindEntityFunction)(const char *name);

typedef SaveSlot *(*GetSaveSlotFunction)(int slot_number);
typedef SaveSlot *(*GetCurrentSaveSlotFunction)();

typedef Inputs (*GetInputsFunction)();
typedef Inputs (*GetInputsPressedFunction)();
typedef Inputs (*GetInputsReleasedFunction)();

struct LevelInfo {
	int tribe;
	int level;
	int map;
	int type;
};
typedef LevelInfo (*GetLevelInfoFunction)();

struct ModernInput {
	struct {
		bool enabled;
		float stickScale;
		float triggerScale;
	} config;
	struct {
		int x;
		int y;
		bool click;
	} leftStick, rightStick;
	struct {
		bool up;
		bool down;
		bool left;
		bool right;
	} dpad;
	int leftTrigger;
	int rightTrigger;
	bool leftShoulder;
	bool rightShoulder;
	bool aButton;
	bool bButton;
	bool xButton;
	bool yButton;
	bool startButton;
	bool backButton;
};

typedef ModernInput (*GetModernInputStateFunction)();

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
	HookGameFunction HookGame;

	SetupIniIntFunction SetupIniInt;
	ReadIniIntFunction ReadIniInt;
	WriteIniIntFunction WriteIniInt;

	SetupIniBoolFunction SetupIniBool;
	ReadIniBoolFunction ReadIniBool;
	WriteIniBoolFunction WriteIniBool;

	SetupIniFloatFunction SetupIniFloat;
	ReadIniFloatFunction ReadIniFloat;
	WriteIniFloatFunction WriteIniFloat;

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
	FindEntityFunction FindEntity;

	GetSaveSlotFunction GetSaveSlot;
	GetCurrentSaveSlotFunction GetCurrentSaveSlot;

	GetLevelInfoFunction GetLevelInfo;

	GetModernInputStateFunction GetModernInputState;

	GotoLevelFunction GotoLevel;
	GotoLevelSelectFunction GotoLevelSelect;
};

// Address helpers --------------------------------------------------

#ifdef IS_MOD_LOADER
int GetGameVersion();
#endif

ModApi *LoadModApi(const char *moduleName);

static int GetRuntimeGameVersion() {
#ifdef IS_MOD_LOADER
	return GetGameVersion();
#else
	ModApi *modApi = LoadModApi("C2ModLoader.asi");
	if (modApi == nullptr)
		return GAMEVER_UNKNOWN;

	return modApi->GetGameVersion();
#endif
}

static MemoryAddress _address(VersionAddresses address) {
	int gameVersion = GetRuntimeGameVersion();
	if (gameVersion == GAMEVER_US)
		return address.us;
	if (gameVersion == GAMEVER_EU)
		return address.eu;
	if (gameVersion == GAMEVER_DEMO)
		return address.demo;
	return address.us;
}

static void *_pointer(VersionAddresses address) {
	return (void *)_address(address).base;
}

// Addresses --------------------------------------------------------

inline uint32_t *cheats = (uint32_t *)_pointer({{0x4B7964}, {0x4BEB54}, {0x4B6964}});

inline uint32_t *fogDistance = (uint32_t *)_pointer({{0x4B7B48}, {0x4BED38}, {0x4B6B48}});
inline uint32_t *renderDistance = (uint32_t *)_pointer({{0x4B7B18}, {0x4BED08}, {0x4B6B18}});

inline WadFile **currentWadFile = (WadFile **)_pointer({{0x4A8C3C}, {0x4A9C3C}, {0x4A7C3C}});

inline MemoryAddress rootObjRef = _address({{0x4A8C3C, {0x14, 0x28, 0x00}}, {0x4A9C3C, {0x14, 0x28, 0x00}}, {0x4A7C3C, {0x14, 0x28, 0x00}}});
inline MemoryAddress crocObjRef = _address({{0x4A8C3C, {0x14, 0x30, 0x00}}, {0x4A9C3C, {0x14, 0x30, 0x00}}, {0x4A7C3C, {0x14, 0x30, 0x00}}});
inline MemoryAddress bossObjRef = _address({{0x4A8C3C, {0x14, 0x38, 0x00}}, {0x4A9C3C, {0x14, 0x38, 0x00}}, {0x4A7C3C, {0x14, 0x38, 0x00}}});
inline MemoryAddress cameraObjRef = _address({{0x4A8C3C, {0x14, 0x3C, 0x00}}, {0x4A9C3C, {0x14, 0x3C, 0x00}}, {0x4A7C3C, {0x14, 0x3C, 0x00}}});
inline MemoryAddress dialogObjRef = _address({{0x4A8C3C, {0x14, 0x40, 0x00}}, {0x4A9C3C, {0x14, 0x40, 0x00}}, {0x4A7C3C, {0x14, 0x40, 0x00}}});
inline uint32_t *stratCount = (uint32_t *)_pointer({{0x636160}, {0x63D350}, {0x635158}});

inline uint32_t *inputsRaw = (uint32_t *)_pointer({{0x52A590}, {0x531780}, {0x529588}});
inline uint32_t *inputsPressedRaw = (uint32_t *)_pointer({{0x52A554}, {0x531744}, {0x52954C}});
inline uint32_t *inputsReleasedRaw = (uint32_t *)_pointer({{0x52A558}, {0x531748}, {0x529550}});

inline uint32_t *analogX = (uint32_t *)_pointer({{0x52A564}, {0x531754}, {0x52955C}});
inline uint32_t *analogY = (uint32_t *)_pointer({{0x52A574}, {0x531764}, {0x52956C}});
inline uint32_t *inputDeviceType = (uint32_t *)_pointer({{0x52A560}, {0x531750}, {0x529558}});
inline uint32_t *analogStrength = (uint32_t *)_pointer({{0x52A54C}, {0x53173C}, {0x529544}});

inline Vec3i *cameraPos = (Vec3i *)_pointer({{0x622B38}, {0x629D28}, {0x621B30}});
inline Vec3i *cameraRot = (Vec3i *)_pointer({{0x4AF328}, {0x4B0378}, {0x4AE3B0}});
inline Vec3i *cameraLookAt = (Vec3i *)_pointer({{0x622B94}, {0x629D84}, {0x621B8C}});

inline uint32_t *movementAllowedState = (uint32_t *)_pointer({{0x622C44}, {0x629E34}, {0x621C3C}});

inline DoorStruct **doorStruct = (DoorStruct **)_pointer({{0x4B7888}, {0x4BEA78}, {0x4B6888}});

inline SaveSlot *saveSlots = (SaveSlot *)_pointer({{0x6040C0}, {0x60B2B0}, {0x6030B8}});
inline uint32_t *currentSaveSlotIndex = (uint32_t *)_pointer({{0x6220FC}, {0x6292EC}, {0x6210F4}});

inline GameState *gameState = (GameState *)_pointer({{0x4B793C}, {0x4BEB2C}, {0x4B693C}});
inline GameState *gameStateComingFrom = (GameState *)_pointer({{0x4B7894}, {0x4BEA84}, {0x4B6894}});
inline GameState *gameStateGoingTo = (GameState *)_pointer({{0x4B7930}, {0x4BEB20}, {0x4B6930}});
inline bool32_t *gameStateTransitioning = (bool32_t *)_pointer({{0x4B7934}, {0x4BEB24}, {0x4B6934}});

inline fnFadeRoutine *fadeRoutine = (fnFadeRoutine *)_pointer({{0x4B776C}, {0x4BE95C}, {0x4B676C}});
inline fnInitFade initFade = (fnInitFade)_pointer({{0x418240}, {0x4186F0}, {0x418240}});

inline LevelInfo *levelInfo = (LevelInfo *)_pointer({{0x4A8C44}, {0x4A9C44}, {0x4A7C44}});

inline bool16_t *binocsActive = (bool16_t *)_pointer({{0x52AEAC}, {0x53209C}, {0x529EA4}});

#define CROC_VAR_HAZARD_BOUNCE_COUNT 19
#define CROC_VAR_STATE 190
#define CROC_VAR_IN_AIR 197
#define CROC_VAR_FALL_TIMER 201

#define CROC_STATE_NORMAL 4096
#define CROC_STATE_ATTACKING 8192
#define CROC_STATE_STOMPING 24576
#define CROC_STATE_FLIP_JUMPING 36864
#define CROC_STATE_CLIMBING_WALL 49152
#define CROC_STATE_HANGING_CEILING 53248

// Api client --------------------------------------------------------

extern "C" __declspec(dllexport) ModApi *GetModApi();

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
