# c2-mod-loader
ASI Mod Loader for Croc 2

Will automatically load `.asi` files from the `mods/` folder.

Doesn't interfere with [dgVoodoo2](https://dege.freeweb.hu/dgVoodoo2/dgVoodoo2/), in fact you're encouraged to use it!

Additionally, provides a lightweight API for mod developers, check this [header file](https://github.com/Zed-z/c2-mod-loader/blob/main/Shared/ModApi.h).

# Compatible versions
- Tested on the US PC version.
	- SHA1: `C7E9ED848E311706DDE83116FD122A0B28B99261`
- Your mileage may vary on other versions!

# Installation instructions
1) Make sure you have [Microsoft Visual C++ Redistributable (x86)](https://aka.ms/vs/17/release/vc_redist.x86.exe) installed
1) Download [dinput8.dll](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/Win32-latest/dinput-Win32.zip) from ThirteenAG's [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)
1) Download the mod loader files from [Releases](https://github.com/Zed-z/c2-mod-loader/releases)
1) Extract both and place them into the game's directory
1) You're done, the game will now load mods from the `mods/` folder!
	- The folder will be created at first startup after installing the mod loader
1) EXTRA: Unpack the provided example mods into the `mods/` folder after installation

# Mod development
1) Create a `Dynamic-Link Library (DLL)` Project in Visual Studio
1) Select the `Release x86` launch configuration
1) Configure the project (`Right Click Project > Properties`):
	- `Advanced > Target File Extension`: .asi
	- `C/C++ > Code Generation > Runtime Library`: Multi-threaded DLL (/MD)
	- `C/C++ > Precompiled Headers > Precompiled Header`: Not Using Precompiled Headers
	- Include the `ModApi.h` header file
		- If using the provided Visual Studio solution:
			- `C/C++ > General > Additional Include Directories`: ..\Shared\
			- `Right Click "Headers" > Add > Existing Item`: ..\Shared\ModApi.h
1) Add your desired code to the `DllMain()` function
1) Example code template:
	```c++
	#include "ModApi.h"
	#include <Windows.h>
	#include <iostream>

	ModApi* api = nullptr;

	BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
		if (reason == DLL_PROCESS_ATTACH) {
			api = LoadSharedModApi();
			if (!api) return FALSE;
			api->Log("Hello world!");
		}
		return TRUE;
	}
	```
1) Build the project
1) You now have an `.asi` file, congratulations!
1) Put it in `mods/` to use

# Special thanks
- Thanks to ThirteenAG for developing Ultimate ASI Loader (donate [here](https://ko-fi.com/thirteenag))
- Thanks to Dege for developing dgVoodoo2 (donate [here](https://dege.freeweb.hu/))
