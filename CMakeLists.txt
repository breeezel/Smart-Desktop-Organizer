cmake_minimum_required(VERSION 3.10) # SmartDesktopOrganizer uses C++17, some features might benefit from a slightly newer CMake for better C++17 support, but 3.10 is a common baseline. Consider 3.15+ if issues persist with C++ features.
project(SmartDesktopOrganizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer not to use compiler-specific extensions like /Ze

# Define sources for the application
set(APP_SOURCES
    src/mfc/stdafx.cpp # Added for PCH
    src/mfc/SmartDesktopOrganizerApp.cpp
    src/mfc/MainFrm.cpp
    src/mfc/SettingsDlg.cpp
    src/mfc/CheckResultsDlg.cpp
    DesktopManager/DesktopChecker.cpp
    DesktopManager/DesktopInfo.cpp
    DesktopManager/DesktopSorter.cpp
    DesktopManager/DesktopFileOperations.cpp
    WebClassifier/WebClassifier.cpp
    ConfigManager/ConfigManager.cpp
    Logging/Logging.cpp
    DesktopArrangement/DesktopLayoutManager.cpp
)

add_executable(SmartDesktopOrganizer WIN32 ${APP_SOURCES})

# Precompiled Header Setup
# For SmartDesktopOrganizer, we use src/mfc/stdafx.h and src/mfc/stdafx.cpp
# The PCH is primarily for the MFC parts of the application.
# Note: target_precompile_headers is available CMake 3.16+.
# The project uses cmake_minimum_required(VERSION 3.10). If issues arise,
# this might need manual PCH setup (/Yc, /Yu flags) or CMake version update.
target_precompile_headers(SmartDesktopOrganizer
    PUBLIC # Or PRIVATE. PUBLIC makes PCH available to linked targets if any, not relevant here.
    src/mfc/stdafx.h
)

# Specify include directories
target_include_directories(SmartDesktopOrganizer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} # General project root
    "${CMAKE_CURRENT_SOURCE_DIR}/libs" # For nlohmann_json
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mfc" # For MFC headers created in this project

    # Explicitly add reported MFC include path
    "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.40.33807/atlmfc/include"

    # Adding individual component directories for clarity, though ${CMAKE_CURRENT_SOURCE_DIR} might cover them
    "${CMAKE_CURRENT_SOURCE_DIR}/DesktopManager"
    "${CMAKE_CURRENT_SOURCE_DIR}/WebClassifier"
    "${CMAKE_CURRENT_SOURCE_DIR}/ConfigManager"
    "${CMAKE_CURRENT_SOURCE_DIR}/Logging"
    "${CMAKE_CURRENT_SOURCE_DIR}/DesktopArrangement"
)

# MFC Configuration and Windows specific settings
if(MSVC)
    message(STATUS "Configuring for MSVC (MFC settings)")

    # Tell CMake to use MFC in a shared DLL. This sets appropriate compiler and linker flags.
    # 1 = Static MFC, 2 = Shared MFC
    set(CMAKE_MFC_FLAG 2)

    target_compile_definitions(SmartDesktopOrganizer PRIVATE
        _AFXDLL # Already implies use of MFC in a shared DLL
        _UNICODE
        UNICODE
        NOMINMAX # To prevent windows.h from defining min/max macros
        _CRT_SECURE_NO_WARNINGS # To suppress warnings for older C runtime functions
    )

    # Link against common Windows libraries
    # MFC applications often need these. Some might be pulled in by MFC itself,
    # but being explicit can help. Shlwapi.lib for PathFileExistsW.
    target_link_libraries(SmartDesktopOrganizer PRIVATE
        User32.lib
        Shell32.lib
        Ole32.lib
        Shlwapi.lib
        Wininet.lib # For WebClassifier's InternetOpenUrl etc.
    )

    # The CMAKE_MFC_FLAG should handle most MFC library linking automatically.
    # If specific MFC import libraries are still missing, they would be added here,
    # but this is usually not needed if the VS environment is set up correctly.
    # e.g. # target_link_libraries(SmartDesktopOrganizer PRIVATE mfc140ud.lib uafxcwd.lib) # Example for specific debug libs

    # Ensure the correct C++ runtime library for shared MFC (/MD or /MDd)
    # This is typically handled by CMAKE_BUILD_TYPE (Debug/Release)
    # Forcing it can be done with CMAKE_MSVC_RUNTIME_LIBRARY if needed, but usually automatic.
    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

    # For MSVC, explicitly set the source file that creates the PCH from stdafx.h
    # This is often needed even with target_precompile_headers for MSVC generators.
    set_source_files_properties("src/mfc/stdafx.cpp"
        PROPERTIES COMPILE_FLAGS "/Ycstdafx.h" # Create PCH from stdafx.h
    )
    # For other MFC source files that use the PCH, target_precompile_headers should automatically
    # add the /Yustdafx.h flag. If it doesn't, manual addition of COMPILE_FLAGS would be needed
    # for those specific MFC sources. For now, relying on target_precompile_headers.

else()
    message(WARNING "MFC is Windows-specific and typically requires MSVC. This CMake configuration for MFC might be incomplete for other compilers/environments.")
endif()

# Add nlohmann_json as a library (header-only, but good practice for structure)
# Assuming json.hpp is in libs/nlohmann_json/json.hpp
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/nlohmann_json"
)
# Link our application to nlohmann_json (important if it were a compiled lib, good for include path management for header-only)
target_link_libraries(SmartDesktopOrganizer PRIVATE nlohmann_json)
