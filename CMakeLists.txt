# ====================================
#  Copyright (C) 2020 Daniel M Tyler.
#  This file is part of Minesweeper.
# ====================================

cmake_minimum_required(VERSION 3.15.6)
# Must include C or .c files will be silently ignored.
project(minesweeper C CXX)

# Set a default build type if none was specified.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(minesweeper src/main.cpp)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND NOT "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
# clang++.
    target_compile_options(minesweeper PRIVATE -Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter)
else()
    message(FATAL_ERROR "Unknown Compiler.")
endif()

find_package(SDL2 REQUIRED)
target_include_directories(minesweeper PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(minesweeper ${SDL2_LIBRARIES})
message(WARNING "SDL2 should be v2.0.10, but it can't be verified automatically.")
