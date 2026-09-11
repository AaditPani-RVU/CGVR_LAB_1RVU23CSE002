cmake_minimum_required(VERSION 3.20)

# Keep the compiler choice consistent with LAB1 on this macOS installation.
if(APPLE AND EXISTS "/opt/homebrew/bin/g++-14")
    set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/g++-14" CACHE FILEPATH "C++ compiler" FORCE)
endif()

project(Lab02MidpointCircle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(midpoint_circle midpoint_circle.cpp)
add_executable(dda_line dda_line.cpp)
add_executable(bresenham_line bresenham_line.cpp)

# Current Apple SDK headers use C11's _Alignof spelling; GCC needs the C++
# spelling when including those declarations from C++ standard headers.
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(midpoint_circle PRIVATE _Alignof=alignof)
    target_compile_definitions(dda_line PRIVATE _Alignof=alignof)
    target_compile_definitions(bresenham_line PRIVATE _Alignof=alignof)
endif()
