cmake_minimum_required(VERSION 3.16)
project(C2ModLoader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Configuration options
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configuration types" FORCE)

# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Game deployment paths
set(CROC2_GAME_DIR "${CMAKE_SOURCE_DIR}/Croc2" CACHE PATH "Path to local Croc 2 game directory")
set(CROC2_MODS_DIR "${CROC2_GAME_DIR}/mods")

# Find required packages
find_package(PkgConfig QUIET)

# Add main loader
add_subdirectory(C2ModLoader)

# Macro for building mod projects
macro(add_mod_project MOD_NAME)
    
    # Set mod directory (look in Mods/ subdirectory)
    set(MOD_DIR "${CMAKE_SOURCE_DIR}/Mods/${MOD_NAME}")
    
    # Enable RC compiler if on Windows
    if(WIN32 AND NOT CMAKE_RC_COMPILER_WORKS)
        enable_language(RC)
    endif()
    
    # Find C++ sources recursively
    file(GLOB_RECURSE MOD_CPP_SOURCES CONFIGURE_DEPENDS "${MOD_DIR}/*.cpp")
    set(RC_FILE "${MOD_DIR}/Resource.rc")
    set(SOURCES ${MOD_CPP_SOURCES})
    if(EXISTS "${RC_FILE}")
        list(APPEND SOURCES "${RC_FILE}")
    endif()
    
    add_library(${MOD_NAME} SHARED
        ${SOURCES}
    )
    
    # Set output extension to .asi
    set_target_properties(${MOD_NAME} PROPERTIES SUFFIX ".asi")
    
    # Include directories (mod root + all subdirectories + shared)
    file(GLOB_RECURSE MOD_HEADER_FILES CONFIGURE_DEPENDS "${MOD_DIR}/*.h" "${MOD_DIR}/*.hpp")
    set(MOD_INCLUDE_DIRS "${MOD_DIR}")
    foreach(HEADER_FILE ${MOD_HEADER_FILES})
        get_filename_component(HEADER_DIR "${HEADER_FILE}" DIRECTORY)
        list(APPEND MOD_INCLUDE_DIRS "${HEADER_DIR}")
    endforeach()
    list(REMOVE_DUPLICATES MOD_INCLUDE_DIRS)

    target_include_directories(${MOD_NAME} PRIVATE
        ${MOD_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/Shared
    )
    
    # Windows resource compiler
    if(EXISTS "${RC_FILE}")
        string(APPEND CMAKE_RC_FLAGS " /I \"${MOD_DIR}\" /I \"${CMAKE_SOURCE_DIR}/Shared\"")
    endif()
    
    # Compile definitions
    target_compile_definitions(${MOD_NAME} PRIVATE
        WIN32
        _WINDOWS
        _USRDLL
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
    )
    
    # Release build optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_definitions(${MOD_NAME} PRIVATE NDEBUG)
		target_compile_options(${MOD_NAME} PRIVATE /O2 /Gy)
		target_link_options(${MOD_NAME} PRIVATE /OPT:REF /OPT:ICF)
    else()
        target_compile_definitions(${MOD_NAME} PRIVATE _DEBUG)
    endif()
    
    # Windows libraries
    target_link_libraries(${MOD_NAME} PRIVATE
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

    # Copy to Release folder
    add_custom_command(TARGET ${MOD_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:${MOD_NAME}>
            ${CMAKE_BINARY_DIR}/Release/$<TARGET_FILE_NAME:${MOD_NAME}>
        COMMENT "Copying ${MOD_NAME}.asi to Release folder"
    )
    
endmacro()

# Autodetect mods in the Mods/ directory
set(MODS_DIR ${CMAKE_SOURCE_DIR}/Mods)
if(EXISTS ${MODS_DIR})
    file(GLOB MOD_CANDIDATES RELATIVE ${MODS_DIR} ${MODS_DIR}/*)
    
    # Find all valid mod directories
    set(MOD_LIST "")
    foreach(DIR_NAME ${MOD_CANDIDATES})
        # Skip if it's not a directory
        if(NOT IS_DIRECTORY ${MODS_DIR}/${DIR_NAME})
            continue()
        endif()
        
        # Check if dllmain.cpp exists in this directory
        if(EXISTS ${MODS_DIR}/${DIR_NAME}/dllmain.cpp)
            list(APPEND MOD_LIST ${DIR_NAME})
            message(STATUS "Auto-detected mod: ${DIR_NAME}")
        endif()
    endforeach()
    
    # Add all detected mods
    foreach(MOD_NAME ${MOD_LIST})
        add_mod_project(${MOD_NAME})
    endforeach()
else()
    message(STATUS "Mods directory not found - no mods will be built")
endif()

message(STATUS "C2ModLoader CMake configuration complete")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Game directory: ${CROC2_GAME_DIR}")

# Packaging paths
set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/package")
set(PACKAGE_MODS_DIR "${PACKAGE_DIR}/mods")
set(PACKAGE_LICENSES_DIR "${PACKAGE_DIR}/LICENSES")
set(PACKAGE_ZIP_NAME "package.zip")
set(PACKAGE_ZIP "${CMAKE_BINARY_DIR}/${PACKAGE_ZIP_NAME}")

# Create custom target for deployment and launch
add_custom_target(Deploy
    COMMAND ${CMAKE_COMMAND} -E echo "Deploying mods to Croc 2 directory..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CROC2_MODS_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/Release/C2ModLoader.asi ${CROC2_GAME_DIR}/C2ModLoader.asi
    DEPENDS C2ModLoader ${MOD_LIST}
    COMMENT "Deploying all mods to game directory"
)

# Add copy commands for each mod dynamically
foreach(MOD_NAME ${MOD_LIST})
    add_custom_command(TARGET Deploy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/Release/${MOD_NAME}.asi ${CROC2_MODS_DIR}/${MOD_NAME}.asi
        COMMENT "Copying ${MOD_NAME}.asi to mods directory"
    )
endforeach()

# Package release
add_custom_target(Package
    COMMAND ${CMAKE_COMMAND} -E echo "Creating release package..."
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_MODS_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_LICENSES_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/Release/C2ModLoader.asi ${PACKAGE_DIR}/C2ModLoader.asi
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/Shared/global.ini ${PACKAGE_DIR}/global.ini
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/LICENSES ${PACKAGE_LICENSES_DIR}
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR} ${CMAKE_COMMAND} -E rm -f ${PACKAGE_ZIP_NAME}
    DEPENDS C2ModLoader ${MOD_LIST}
    COMMENT "Packaging C2ModLoader release artifacts"
)

foreach(MOD_NAME ${MOD_LIST})
    add_custom_command(TARGET Package POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/Release/${MOD_NAME}.asi ${PACKAGE_MODS_DIR}/${MOD_NAME}.asi
        COMMENT "Packaging ${MOD_NAME}.asi"
    )
endforeach()

add_custom_command(TARGET Package POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E chdir ${PACKAGE_DIR} ${CMAKE_COMMAND} -E tar cf ../${PACKAGE_ZIP_NAME} --format=zip C2ModLoader.asi global.ini mods LICENSES
    COMMENT "Creating package zip archive"
)

# Launch the game
add_custom_target(Launch
	COMMAND ${CMAKE_COMMAND} -E echo "Launching Croc 2..."
	COMMAND cmd.exe /C "start /D \"${CROC2_GAME_DIR}\" /WAIT Croc2.exe & exit 0"
	DEPENDS Deploy
	COMMENT "Launching Croc 2..."
)
