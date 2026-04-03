# Croc 2 Mod Loader

ASI Mod Loader for Croc 2

Will automatically load `.asi` files from the `mods/` folder.

Additionally, provides a lightweight API for mod developers.

Overview and install instructions are available on [the website](https://zed-z.github.io/c2-mod-loader/).

# Mod development

## Prerequisites

### Windows

1. Install Visual Studio with C++ desktop tools
1. Install CMake and Ninja

### Linux

1. Install a 32-bit MinGW toolchain (mingw-w64)

## Mod creation

1. Copy `ModTemplate/` located in the `Mods/` directory
1. Adjust the copied `Resource.rc` metadata fields
1. Modify code in `dllmain.cpp` as needed
1. The toolchain should automatically build any additional `.cpp` files, including subdirectories

## Building

### Windows

1. Run `./build.ps1 -Configuration Release`
1. Mods are autodetected from `Mods/` and built to `build/Release/*.asi`
1. For automatic mod installation and game launching, put Croc 2 game files in `Croc2/mods/` and use the `-Deploy` or `-Launch` flags

### Linux

1. Run `./build.sh`
1. Mods are autodetected from `Mods/` and built to `build/Release/*.asi`
1. For automatic mod installation and game launching, put Croc 2 game files in `Croc2/mods/` and use the `-d` or `-l` flags

# Third-Party Licenses

This project uses the following third party assets:

-   [Dear ImGui](https://github.com/ocornut/imgui) - Copyright (c) 2014-2025 Omar Cornut
-   [MinHook](https://github.com/TsudaKageyu/minhook) - Copyright (c) 2009-2017 Tsuda Kageyu
-   [Scabber Font](https://ggbot.itch.io/scabber-font) by GGBotNet, licensed under the Creative Commons Zero v1.0 Universal license.

Licenses for the above mentioned are available on the GitHub repository.

# Special thanks

-   Thanks to ThirteenAG for developing Ultimate ASI Loader (support [here](https://ko-fi.com/thirteenag))
-   Thanks to Dege for developing dgVoodoo2 (support [here](https://dege.freeweb.hu/))
-   Thanks to ocornut for developing Dear ImGui (support [here](https://github.com/ocornut/imgui/wiki/Funding))
-   Thanks to TsudaKageyu for developing Minhook (support [here](https://github.com/TsudaKageyu))
-   Thanks to Thermospore, hdc0, limbus, Ray and Rartrin from the Croc & Stuff Discord server for valuable insight about the game!
-   Thanks to Argonaut Games for developing Croc 2!
