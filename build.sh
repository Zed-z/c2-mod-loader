#!/bin/sh

CC=i686-w64-mingw32-gcc
CXX=i686-w64-mingw32-g++
RC=i686-w64-mingw32-windres
TYPE=Release
VERSION=US
export WINEPREFIX="$HOME/.local/share/wineprefixes/croc2"
PATH_TO_GAME="$WINEPREFIX/drive_c/Program Files (x86)/Fox/Croc 2/$VERSION"
DEPLOY=false
PACKAGE=false
LAUNCH=false

usage() {
    echo "Usage: $0 [-c <Debug|Release>] [-v <EU|US|DEMO>] [-Cdplh]"
    echo "  -c - build configuration (default: Release)"
    echo "  -v - game version to use (EU, US, DEMO). Default: US"
    echo "  -C - clean the build directory before building"
    echo "  -d - copy the mod loader and mods"
    echo "  -p - create package folder + zip"
    echo "  -l - deploy and launch Croc 2 with mods"
    echo "  -h - show this help message and exit"
}

clean_build() {
    rm -rf ./build/
}

deploy() {
    cmake --build build --target Deploy
}

package() {
    cmake --build build --target Package
}

launch() {
    wine "$PATH_TO_GAME/Croc2.exe"
}

command_availability() {
    missing=0
    for cmd in \
        $CC \
        $CXX \
        $RC \
        cmake \
        wine
    do
        command -v $cmd >/dev/null 2>&1 || { echo "Command not found: $cmd"; missing=1; }
    done

    if [ $missing -eq 1 ]; then
        exit 1;
    fi
}

command_availability

while getopts "c:v:Cdplh" opt; do
    case $opt in
        c) TYPE="$OPTARG" ;;
        v) VERSION="$OPTARG" ; PATH_TO_GAME="$WINEPREFIX/drive_c/Program Files (x86)/Fox/Croc 2/$VERSION" ;;
        C) clean_build ;;
        d) DEPLOY=true ;;
        p) PACKAGE=true ;;
        l) LAUNCH=true ;;
        h)
            usage
            exit 0
            ;;
        :)
            usage
            exit 1
            ;;
        \?)
            usage
            exit 1
            ;;
    esac
done

cmake -B build/ \
    -DCMAKE_BUILD_TYPE=$TYPE         \
    -DCMAKE_C_COMPILER=$CC           \
    -DCMAKE_CXX_COMPILER=$CXX        \
    -DCROC2_GAME_DIR="$PATH_TO_GAME" \
    -DCROC2_MODS_DIR="$PATH_TO_GAME/mods"
cmake --build build/

if [ "$DEPLOY" = true ] || [ "$LAUNCH" = true ]; then
    deploy
    if [ "$LAUNCH" = true ]; then
        launch
    fi
fi

if [ "$PACKAGE" = true ]; then
    package
fi
