cmake_minimum_required(VERSION 3.10)
project(SmartDesktopOrganizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include DesktopManager files
add_executable(SmartDesktopOrganizer
    src/main.cpp
    DesktopManager/DesktopChecker.cpp
    DesktopManager/DesktopInfo.cpp
    DesktopManager/DesktopSorter.cpp
    WebClassifier/WebClassifier.cpp # Added WebClassifier
)

# Specify include directories
target_include_directories(SmartDesktopOrganizer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/libs" # For nlohmann_json or other libs
)

# libcurl integration removed for now, using WinINet on Windows.
# include(FetchContent)
# FetchContent_Declare(
#     curl
#     GIT_REPOSITORY https://github.com/curl/curl.git
#     GIT_TAG curl-8.6.0
# )
# FetchContent_MakeAvailable(curl)
# target_link_libraries(SmartDesktopOrganizer PRIVATE curl::libcurl)
