# c2-mod-loader
ASI Mod Loader for Croc 2

Will automatically load .asi files from the `mods/` folder.

Doesn't interfere with [dgVoodoo2](https://dege.freeweb.hu/dgVoodoo2/dgVoodoo2/), in fact you're encouraged to use it!

Additionally, provides a lightweight API for mod developers, check this [header file](https://github.com/Zed-z/c2-mod-loader/blob/main/Shared/ModApi.h).

# Installation instructions
1) Download [dinput8.dll](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/Win32-latest/dinput-Win32.zip) from ThirteenAG's [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)
1) Download the mod loader files from [Releases](https://github.com/Zed-z/c2-mod-loader/releases)
1) Extract both and place them into the game's directory
1) You're done, the game will now load mods from the `mods/` folder!
	- The folder will be created at first startup after installing the mod loader
1) EXTRA: Unpack the provided example mods into the `mods/` folder after installation

# Special thanks
- Thanks to ThirteenAG for developing Ultimate ASI Loader (donate [here](https://ko-fi.com/thirteenag))
- Thanks to Dege for developing dgVoodoo2 (donate [here](https://dege.freeweb.hu/))
