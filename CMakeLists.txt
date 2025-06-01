cmake_minimum_required(VERSION 3.10)
project(SmartDesktopOrganizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define sources for the application
# Note: src/main.cpp is removed as MFC uses its own app entry point.
set(APP_SOURCES
    src/mfc/SmartDesktopOrganizerApp.cpp
    src/mfc/MainFrm.cpp
    # Add other existing cpp files that are part of the core logic, not UI
    DesktopManager/DesktopChecker.cpp
    DesktopManager/DesktopInfo.cpp
    DesktopManager/DesktopSorter.cpp
    DesktopManager/DesktopFileOperations.cpp # Added DesktopFileOperations
    WebClassifier/WebClassifier.cpp
    ConfigManager/ConfigManager.cpp
    Logging/Logging.cpp
)

# WIN32 creates a Windows subsystem executable (no console window)
# This is appropriate for MFC GUI applications.
add_executable(SmartDesktopOrganizer WIN32 ${APP_SOURCES})

# Specify include directories
target_include_directories(SmartDesktopOrganizer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/libs" # For nlohmann_json
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mfc" # For MFC headers created in this project
    # Potentially add DesktopManager and WebClassifier if their headers are not found
    # For now, assuming they are found due to being relative to CMAKE_CURRENT_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/DesktopManager"
    "${CMAKE_CURRENT_SOURCE_DIR}/WebClassifier"
    "${CMAKE_CURRENT_SOURCE_DIR}/ConfigManager"
    "${CMAKE_CURRENT_SOURCE_DIR}/Logging" # Added Logging
)

# MFC Configuration for MSVC (Visual Studio)
if(MSVC)
    message(STATUS "Configuring for MSVC (MFC settings)")
    # Use MFC in a shared DLL
    target_compile_definitions(SmartDesktopOrganizer PRIVATE _AFXDLL)
    # Set Character Set to Unicode
    target_compile_definitions(SmartDesktopOrganizer PRIVATE _UNICODE UNICODE)

    # Set the correct runtime library (/MD or /MDd)
    # This is typically handled by CMAKE_BUILD_TYPE (Debug/Release)
    # but ensure it's compatible with _AFXDLL.
    # CMAKE_MSVC_RUNTIME_LIBRARY can be set, e.g. "MultiThreadedDLL" or "MultiThreadedDebugDLL"
    # For simplicity, relying on CMake's defaults which usually align with VS project settings for /MD[d].

    # Add MFC-specific compiler options if needed
    # target_compile_options(SmartDesktopOrganizer PRIVATE /some_mfc_flag)

    # Linking against MFC libraries is usually handled automatically by Visual Studio
    # when /D_AFXDLL is defined and the project type is set.
    # CMake doesn't have a standard find_package(MFC).
    # If linking errors occur, specific MFC import libraries might need to be added.
    # e.g. for MFC version 14.x (VS 2015+), names like uafxcw.lib, mfc140u.lib etc.
    # This is highly dependent on the Visual Studio version and MFC installation.
    # For now, we assume the compiler/linker will find necessary MFC libs via default setup.
    # If not, one might need to add: CMAKE_EXE_LINKER_FLAGS "/ENTRY:wWinMainCRTStartup" for Unicode MFC.
else()
    message(WARNING "MFC is Windows-specific and typically requires MSVC. This CMake configuration for MFC might be incomplete for other compilers/environments.")
endif()

# Ensure any Qt specific configuration from previous attempts is removed or fully commented out.
# (The overwrite ensures this)
