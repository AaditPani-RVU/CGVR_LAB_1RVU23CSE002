cmake_minimum_required(VERSION 3.20)

# This Mac has an incomplete Apple C++ standard library installation. Prefer
# Homebrew GCC when it is available; other platforms keep CMake's default
# compiler selection.
if(APPLE AND EXISTS "/opt/homebrew/bin/gcc-14" AND EXISTS "/opt/homebrew/bin/g++-14")
    set(CMAKE_C_COMPILER "/opt/homebrew/bin/gcc-14" CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/g++-14" CACHE FILEPATH "C++ compiler" FORCE)
endif()

project(Lab05 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 3.3 REQUIRED)

add_executable(cohen_sutherland
    cohen_sutherland.cpp
    glad_build/src/glad.c
)
target_include_directories(cohen_sutherland PRIVATE glad_build/include)
target_link_libraries(cohen_sutherland PRIVATE glfw)

# Apple SDK headers currently use the C11 `_Alignof` spelling in
# declarations. GCC accepts it in C but needs the C++ spelling when
# compiling cohen_sutherland.cpp.
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(cohen_sutherland PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:_Alignof=alignof>")
endif()

if(APPLE)
    target_link_libraries(cohen_sutherland PRIVATE "-framework Cocoa" "-framework IOKit" "-framework OpenGL")
endif()
