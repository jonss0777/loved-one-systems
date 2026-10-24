cmake_minimum_required(VERSION 3.16)
project(DrogonGStreamerApp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically copy DLLs (including GStreamer plugins) to your build folder
set(VCPKG_APPLOCAL_DEPS ON)

# 1. Find Drogon Package
find_package(Drogon REQUIRED)
include(DrogonUtilities)

# 2. Find GStreamer using vcpkg's PkgConfig implementation
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)

# Define your main application executable
add_executable(${PROJECT_NAME} main.cpp)

# 3. Link dependencies to your target
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    Drogon::Drogon
    PkgConfig::GSTREAMER  # Modern, clean CMake tracking for GStreamer
)
