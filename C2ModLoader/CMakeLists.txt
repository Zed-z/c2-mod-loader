cmake_minimum_required(VERSION 3.16)

# Generate license header
set(EMBEDDED_LICENSE_FILES
    "${CMAKE_SOURCE_DIR}/LICENSE.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/LICENSE.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/minhook/LICENSE.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/scabber_font/License.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/inter_font/OFL.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/roboto_mono_font/OFL.txt"
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${EMBEDDED_LICENSE_FILES})

function(append_embedded_license DISPLAY_NAME FILE_PATH OUT_VAR)
    file(READ "${FILE_PATH}" _license_content)
    # Strip UTF-8 BOM (EF BB BF)
    string(ASCII 239 187 191 _utf8_bom)
    string(REGEX REPLACE "^${_utf8_bom}" "" _license_content "${_license_content}")
    string(REPLACE "\r\n" "\n" _license_content "${_license_content}")
    string(APPEND _license_entry "        {\"${DISPLAY_NAME}\", R\"(\n${_license_content}\n)\"},\n")
    set(${OUT_VAR} "${${OUT_VAR}}${_license_entry}" PARENT_SCOPE)
endfunction()

set(EMBEDDED_LICENSE_ENTRIES "")
append_embedded_license("C2ModLoader" "${CMAKE_SOURCE_DIR}/LICENSE.txt" EMBEDDED_LICENSE_ENTRIES)
append_embedded_license("Dear ImGui" "${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/LICENSE.txt" EMBEDDED_LICENSE_ENTRIES)
append_embedded_license("MinHook" "${CMAKE_CURRENT_SOURCE_DIR}/libs/minhook/LICENSE.txt" EMBEDDED_LICENSE_ENTRIES)
append_embedded_license("Scabber Font" "${CMAKE_CURRENT_SOURCE_DIR}/libs/scabber_font/License.txt" EMBEDDED_LICENSE_ENTRIES)
append_embedded_license("Inter Font" "${CMAKE_CURRENT_SOURCE_DIR}/libs/inter_font/OFL.txt" EMBEDDED_LICENSE_ENTRIES)
append_embedded_license("Roboto Mono Font" "${CMAKE_CURRENT_SOURCE_DIR}/libs/roboto_mono_font/OFL.txt" EMBEDDED_LICENSE_ENTRIES)

set(GENERATED_EMBEDDED_LICENSES_HEADER "${CMAKE_CURRENT_BINARY_DIR}/GeneratedEmbeddedLicenses.h")
string(CONCAT GENERATED_EMBEDDED_LICENSES_CONTENT
    "#pragma once\n"
    "\n"
    "#include \"Launcher/EmbeddedLicenses.h\"\n"
    "#include <vector>\n"
    "\n"
    "namespace Launcher {\n"
    "\n"
    "inline const std::vector<EmbeddedLicense>& GetGeneratedEmbeddedLicenses() {\n"
    "    static const std::vector<EmbeddedLicense> licenses = {\n"
    "${EMBEDDED_LICENSE_ENTRIES}"
    "    };\n"
    "\n"
    "    return licenses;\n"
    "}\n"
    "\n"
    "}\n"
)
file(WRITE "${GENERATED_EMBEDDED_LICENSES_HEADER}" "${GENERATED_EMBEDDED_LICENSES_CONTENT}")

# Source files
set(SOURCES
    CheatsManager.cpp
    ConsoleLogging.cpp
    dllmain.cpp
    FileOverrides.cpp
    GameHooks.cpp
    Input/AnalogInput.cpp
    Input/DpadMovement.cpp
    Input/Input.cpp
    Input/MidAirTurning.cpp
    Input/TypeSwitching.cpp
    Input/Vibration.cpp
    Launcher/Backend.cpp
    Launcher/EmbeddedLicenses.cpp
    Launcher/IniConfig.cpp
    Launcher/Launcher.cpp
    Loader.cpp
    Logs.cpp
    MenuActions.cpp
    ModApi.cpp
    MouseCaptureRemover.cpp
    Overlay/Backend.cpp
    Overlay/Components/CameraStack.cpp
    Overlay/Components/Coords.cpp
    Overlay/Components/Inputs.cpp
    Overlay/Components/Minimap.cpp
    Overlay/Components/LevelInfo.cpp
    Overlay/Components/LevelSelect.cpp
    Overlay/Components/Log.cpp
    Overlay/Components/MenuBar.cpp
    Overlay/Components/ObjectList.cpp
    Overlay/Components/SaveSlotList.cpp
    Overlay/Components/Toast.cpp
    Overlay/Components/VersionInfo.cpp
    Overlay/Overlay.cpp
    Registry/ManagedKeys.cpp
    Registry/PathOverrides.cpp
    Registry/QueryReturns.cpp
    Registry/RegistryManager.cpp
    Registry/ValueMapParser.cpp
    Resource.rc
    Toasts.cpp
    Utils/Fonts.cpp
    Utils/ResourceLoader.cpp
    Utils/Sha256.cpp
    Utils/Style.cpp
    Utils/UiScale.cpp
    WadNames.cpp
)

# ImGui Core + Backends
set(IMGUI_SOURCES
    libs/imgui/imgui.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
)
set(IMGUI_BACKENDS
    libs/imgui/backends/imgui_impl_dx11.cpp
    libs/imgui/backends/imgui_impl_win32.cpp
)

# MinHook
set(MINHOOK_SOURCES
    libs/minhook/src/buffer.c
    libs/minhook/src/hook.c
    libs/minhook/src/trampoline.c
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND MINHOOK_SOURCES libs/minhook/src/hde/hde64.c)
else()
    list(APPEND MINHOOK_SOURCES libs/minhook/src/hde/hde32.c)
endif()

# Create DLL
add_library(C2ModLoader SHARED
    ${SOURCES}
    ${IMGUI_SOURCES}
    ${IMGUI_BACKENDS}
    ${MINHOOK_SOURCES}
)

# Set output extension to .asi
set_target_properties(C2ModLoader PROPERTIES SUFFIX ".asi")

# Include directories
target_include_directories(C2ModLoader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Shared
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/minhook/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/minhook/src
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/minhook/src/hde
)

# Windows resource compiler
enable_language(RC)
target_compile_options(C2ModLoader PRIVATE
    $<$<COMPILE_LANGUAGE:RC>:/I"${CMAKE_CURRENT_SOURCE_DIR}">
    $<$<COMPILE_LANGUAGE:RC>:/I"${CMAKE_SOURCE_DIR}/Shared">
)

# Link libgcc statically
if(LINUX)
    target_link_options(C2ModLoader PRIVATE
        "-static-libgcc"
        "-static-libstdc++"
        "--static"
    )
endif()

# Compile definitions
target_compile_definitions(C2ModLoader PRIVATE
    WIN32
    _WINDOWS
    _USRDLL
    UNICODE
    _UNICODE
    C2MODLOADER_EXPORTS
    _CRT_SECURE_NO_WARNINGS
)

# Release build optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_definitions(C2ModLoader PRIVATE NDEBUG)
        target_compile_options(C2ModLoader PRIVATE /O2 /Gy)
        target_link_options(C2ModLoader PRIVATE /OPT:REF /OPT:ICF)
    elseif(LINUX)
        target_compile_definitions(C2ModLoader PRIVATE NDEBUG)
        target_compile_options(C2ModLoader PRIVATE -O2)
        target_link_options(C2ModLoader PRIVATE -flto)
    endif()
else()
    target_compile_definitions(C2ModLoader PRIVATE _DEBUG)
endif()

# Windows libraries
target_link_libraries(C2ModLoader PRIVATE
    d3d11
    dxgi
    d3dcompiler
    gdiplus
    kernel32
    user32
    gdi32
    winspool
    comdlg32
    advapi32
    shell32
    ole32
    oleaut32
    uuid
    odbc32
    odbccp32
    ws2_32
    comctl32
    shlwapi
    dwmapi
    version
    xinput
)

# Copy to Release folder
add_custom_command(TARGET C2ModLoader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:C2ModLoader>
        ${CMAKE_BINARY_DIR}/Release/$<TARGET_FILE_NAME:C2ModLoader>
    COMMENT "Copying C2ModLoader.asi to Release folder"
)
