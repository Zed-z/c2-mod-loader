#pragma once

#define LOADER_NAME "Croc 2 Mod Loader"
#define LOADER_NAME_L L"Croc 2 Mod Loader"

#define LOADER_VERSION "1.1.0"
#define LOADER_VERSION_L L"1.1.0"

#define AUTHOR_NAME "Zed-z"
#define AUTHOR_NAME_L L"Zed-z"

#define LOADER_DESCRIPTION "Provides mod loading functionality and a lightweight API for mods."
#define LOADER_DESCRIPTION_L L"Provides mod loading functionality and a lightweight API for mods."

#define LOADER_HYPERLINK "https://github.com/Zed-z/c2-mod-loader"
#define LOADER_HYPERLINK_L L"https://github.com/Zed-z/c2-mod-loader"

#define LOADER_CONFIG_TYPES "\
@Config[General Configuration|Configure the general settings];\
Config/LoaderEnabled[Loader Enabled|Enable or disable mod loading]:bool=1;\
Config/SkipLauncher[Skip Launcher|Skip the launcher and start the game directly]:bool=0;\
Config/FreeMouse[Free Mouse|Allow mouse to move freely]:bool=1;\
Config/DisplayScale[Display Scale|Launcher and UI display scale (0 = automatic)]:float=0;\
_Config/DisabledMods[Disabled Mods|List of disabled mods]:string=;\
@GUI[GUI|Configure in-game GUI settings];\
GUI/GuiEnabled[GUI Enabled|Enable the GUI]:bool=1;\
GUI/ShowGui[Show GUI|Show or hide the GUI]:bool=0;\
GUI/ShowLog[Show Log|Show the log window]:bool=0;\
GUI/ShowInputs[Show Inputs|Show input configuration]:bool=0;\
GUI/ShowObjectList[Show Object List|Show the object list]:bool=0;\
GUI/ShowCoords[Show Coordinates|Show coordinates]:bool=0;\
GUI/ShowLevelInfo[Show Level Info|Show level information]:bool=0;\
GUI/ShowSaveSlotList[Show Save Slot List|Show save slot list]:bool=0;\
@Logging[Logging|Configure logging settings];\
Logging/Info[Info Logging|Show info logs]:bool=1;\
Logging/Debug[Debug Logging|Show debug logs]:bool=0;\
Logging/Warning[Warning Logging|Show warning logs]:bool=1;\
Logging/Error[Error Logging|Show error logs]:bool=1;\
@Toasts[Toast Notifications|Configure toast notification settings];\
Toasts/Info[Info Toasts|Enable info toasts]:bool=1;\
Toasts/Debug[Debug Toasts|Enable debug toasts]:bool=0;\
Toasts/Warning[Warning Toasts|Enable warning toasts]:bool=1;\
Toasts/Error[Error Toasts|Enable error toasts]:bool=1;\
@Cheats[Cheats|Enable or disable cheats];\
Cheats/DebugMenu[Debug Menu|Enable debug menu]:bool=0;\
Cheats/PositionBar[Position Bar|Enable position bar]:bool=0;\
Cheats/Invulnerability[Invulnerability|Enable invulnerability cheat]:bool=0;\
Cheats/BonusCrystals[Bonus Crystals|Enable bonus crystals cheat]:bool=0;\
Cheats/MusicSelect[Music Select|Enable music selection cheat]:bool=0;\
@Xinput[Xinput|Enable and configure Xinput settings];\
Xinput/XinputEnabled[Xinput Enabled|Enable Xinput support]:bool=1;\
Xinput/DeviceIndex[Device Index|Index of the Xinput device to use]:int=0;\
Xinput/StickDeadzone[Stick Deadzone|Deadzone for analog sticks]:int=25;\
Xinput/StickOuterDeadzone[Stick Outer Deadzone|Outer deadzone for analog sticks]:int=75;\
Xinput/TriggerDeadzone[Trigger Deadzone|Deadzone for triggers]:int=10;\
Xinput/TriggerOuterDeadzone[Trigger Outer|Outer deadzone for triggers]:int=90"

#define LOG_FILE "C2ModLoader.log"
#define LOG_FILE_L L"C2ModLoader.log"

#define CONFIG_FILE "C2ModLoader.ini"
#define CONFIG_FILE_L L"C2ModLoader.ini"

#define IDR_FONT_TITLE 10001
#define IDR_FONT_TEXT 10002
