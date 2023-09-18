cmake_minimum_required(VERSION 3.0.0)

# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
project(VPP LANGUAGES CXX)

if (BUILD_SHARED_LIBS)
    set(VPP_BUILD_DLL 1)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

configure_file(src/Config.h.in src/Config.h @ONLY)

add_subdirectory(src)
