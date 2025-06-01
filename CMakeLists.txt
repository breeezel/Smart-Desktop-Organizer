cmake_minimum_required(VERSION 3.10)
project(SmartDesktopOrganizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include DesktopManager files
add_executable(SmartDesktopOrganizer src/main.cpp DesktopManager/DesktopChecker.cpp DesktopManager/DesktopInfo.cpp DesktopManager/DesktopSorter.cpp)

# Specify include directories
target_include_directories(SmartDesktopOrganizer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
