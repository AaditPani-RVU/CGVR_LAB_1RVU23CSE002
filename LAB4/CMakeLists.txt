cmake_minimum_required(VERSION 3.20)

# This macOS environment has incomplete Apple C++ headers; use Homebrew GCC
# when available, matching the working configuration in the earlier labs.
if(APPLE AND EXISTS "/opt/homebrew/bin/g++-14")
    set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/g++-14" CACHE FILEPATH "C++ compiler" FORCE)
endif()

project(Lab04CompositeTransformations LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(composite_transformations composite_transformations.cpp)

if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(composite_transformations PRIVATE _Alignof=alignof)
endif()
