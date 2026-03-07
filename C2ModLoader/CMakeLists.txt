cmake_minimum_required(VERSION 3.16)

# Source files
set(SOURCES
    CheatsManager.cpp
    dllmain.cpp
    ImGuiHandler.cpp
    Launcher.cpp
    Loader.cpp
    ModApi.cpp
    MouseCaptureRemover.cpp
    Resource.rc
)

# ImGui Core
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
)

# ImGui Backends for Windows + DirectX11
set(IMGUI_BACKENDS
    imgui/backends/imgui_impl_dx11.cpp
    imgui/backends/imgui_impl_win32.cpp
)

# Create DLL
add_library(C2ModLoader SHARED
    ${SOURCES}
    ${IMGUI_SOURCES}
    ${IMGUI_BACKENDS}
)

# Set output extension to .asi
set_target_properties(C2ModLoader PROPERTIES SUFFIX ".asi")

# Include directories
target_include_directories(C2ModLoader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Shared
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
)

# Windows resource compiler
enable_language(RC)
target_compile_options(C2ModLoader PRIVATE
    $<$<COMPILE_LANGUAGE:RC>:/I"${CMAKE_CURRENT_SOURCE_DIR}">
    $<$<COMPILE_LANGUAGE:RC>:/I"${CMAKE_SOURCE_DIR}/Shared">
)

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
    target_compile_definitions(C2ModLoader PRIVATE NDEBUG)
	target_compile_options(C2ModLoader PRIVATE /O2 /Gy)
	target_link_options(C2ModLoader PRIVATE /OPT:REF /OPT:ICF)
else()
    target_compile_definitions(C2ModLoader PRIVATE _DEBUG)
endif()

# Windows libraries
target_link_libraries(C2ModLoader PRIVATE
    d3d11
    dxgi
    d3dcompiler
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
)

# Find and link MinHook
find_path(MINHOOK_INCLUDE_DIR NAMES MinHook.h)
find_library(MINHOOK_LIB NAMES minhook minhook.x32 MinHook libMinHook)

if(MINHOOK_INCLUDE_DIR AND MINHOOK_LIB)
    target_include_directories(C2ModLoader PRIVATE ${MINHOOK_INCLUDE_DIR})
    target_link_libraries(C2ModLoader PRIVATE ${MINHOOK_LIB})
    message(STATUS "Found MinHook include: ${MINHOOK_INCLUDE_DIR}")
    message(STATUS "Found MinHook library: ${MINHOOK_LIB}")
else()
    include(FetchContent)
    FetchContent_Declare(
        minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG v1.3.4
    )
    FetchContent_MakeAvailable(minhook)

    if(TARGET minhook)
        target_link_libraries(C2ModLoader PRIVATE minhook)
    elseif(TARGET minhook::minhook)
        target_link_libraries(C2ModLoader PRIVATE minhook::minhook)
    elseif(TARGET libMinHook)
        target_link_libraries(C2ModLoader PRIVATE libMinHook)
    else()
        message(FATAL_ERROR "MinHook target not found after FetchContent.")
    endif()

    message(STATUS "Using MinHook via FetchContent")
endif()

# Embed manifest with mt.exe
add_custom_command(TARGET C2ModLoader POST_BUILD
	COMMAND mt.exe -manifest "${CMAKE_CURRENT_SOURCE_DIR}/Manifest.manifest" 
					-outputresource:"$<TARGET_FILE:C2ModLoader>"\;2
	COMMENT "Embedding manifest into C2ModLoader.asi"
)

# Copy to Release folder
add_custom_command(TARGET C2ModLoader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:C2ModLoader>
        ${CMAKE_BINARY_DIR}/Release/$<TARGET_FILE_NAME:C2ModLoader>
    COMMENT "Copying C2ModLoader.asi to Release folder"
)
