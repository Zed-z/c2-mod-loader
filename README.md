# c2-mod-loader

ASI Mod Loader for Croc 2

Will automatically load `.asi` files from the `mods/` folder.

Additionally, provides a lightweight API for mod developers.

# Compatible versions

-   Tested on the US PC version.
    -   SHA1: `C7E9ED848E311706DDE83116FD122A0B28B99261`
-   Your mileage may vary on other versions!

# Installation instructions

1. Make sure you have [Microsoft Visual C++ Redistributable (x86)](https://aka.ms/vs/17/release/vc_redist.x86.exe) installed
1. Download [dinput8.dll](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/Win32-latest/dinput-Win32.zip) from ThirteenAG's [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)
1. Download the mod loader files from [Releases](https://github.com/Zed-z/c2-mod-loader/releases)
1. Extract both and place them into the game's directory
1. You're done, the game will now load mods from the `mods/` folder!
1. EXTRA: Unpack the provided example mods into the `mods/` folder after installation

# Recommended: Custom GUI support

In order to display custom GUIs, the mod loader needs the game to have [dgVoodoo2](https://dege.freeweb.hu/dgVoodoo2/dgVoodoo2/) applied.

Without it, the mod loader will continue to function, but you will not see the custom GUI overlay.

# Mod development

## Prerequisites

1. Install Visual Studio and vcpkg
1. Install `imgui:x86-windows-static` and `minhook:x86-windows-static` with vcpkg

## Project setup

### A) From scratch

1. Create a `Dynamic-Link Library (DLL)` Project in Visual Studio
1. Select the `Release x86` launch configuration
1. Configure the project (`Right Click Project > Properties`):
    - `Advanced > Target File Extension`: .asi
    - `C/C++ > Language > C++ Language Standard`: ISO C++17 Standard (/std:c++17)
    - `C/C++ > Code Generation > Runtime Library`: Multi-threaded (/MT)
    - `C/C++ > Precompiled Headers > Precompiled Header`: Not Using Precompiled Headers
    - `vcpkg > Use Static Libraries`: Yes
    - Include the `ModApi.h` [header file](https://github.com/Zed-z/c2-mod-loader/blob/main/Shared/ModApi.h)
        - If using the provided Visual Studio solution:
            - `C/C++ > General > Additional Include Directories`: ..\Shared\
            - `Resources > General > Additional Include Directories`: ..\Shared\
            - `Right Click "Headers" > Add > Existing Item`: ..\Shared\ModApi.h
    - Copy the `Shared\Resource.rc` file into your project folder and add it as a Resource File
        - Adjust the `Name`, `Author`, `Description`, `Version` fields
1. Add your desired code to the `DllMain()` function
1. Example code template:

    ```c++
    #include "ModApi.h"
    #include <Windows.h>
    #include <iostream>

    ModApi* api = nullptr;

    BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    	if (reason == DLL_PROCESS_ATTACH) {
    		api = LoadModApi();
    		if (!api) return FALSE;
    		api->LogInfo("Hello world!");
    	}
    	return TRUE;
    }
    ```

### B) From a template

1. Pull the repository and duplicate the `HelloWorld` project folder
1. Load the project into Visual Studio
1. Adjust the project's Resource File
    - Adjust the `Name`, `Author`, `Description`, `Version` fields
1. Modify the code to your hearts content

## Building

1. Build the project with `Build > Build Solution / Build Project`
1. You now have an `.asi` file in the `Release` folder, congratulations!
1. Put it in `mods/` to use

# Third-Party Libraries

This project uses the following libraries:

-   [Dear ImGui](https://github.com/ocornut/imgui) - Copyright (c) 2014-2025 Omar Cornut
-   [MinHook](https://github.com/TsudaKageyu/minhook) - Copyright (c) 2009-2017 Tsuda Kageyu

Licenses for the above mentioned libraries are included in `LICENSES/`.

# Third-Party Code

-   This project includes implementation code from [Dear ImGui](https://github.com/ocornut/imgui), licensed under the MIT License.

# Special thanks

-   Thanks to ThirteenAG for developing Ultimate ASI Loader (support [here](https://ko-fi.com/thirteenag))
-   Thanks to Dege for developing dgVoodoo2 (support [here](https://dege.freeweb.hu/))
-   Thanks to ocornut for developing Dear ImGui (support [here](https://github.com/ocornut/imgui/wiki/Funding))
-   Thanks to TsudaKageyu for developing Minhook (support [here](https://github.com/TsudaKageyu))
-   Thanks to Thermospore, hdc0, limbus, Ray and Rartrin from the Croc & Stuff Discord server for valuable insight about the game!
-   Thanks to Argonaut Games for developing Croc 2!
