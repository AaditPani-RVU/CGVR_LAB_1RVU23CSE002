cmake_minimum_required(VERSION 3.20)

# Match the established macOS configuration used in the earlier labs.
if(APPLE AND EXISTS "/opt/homebrew/bin/gcc-14" AND EXISTS "/opt/homebrew/bin/g++-14")
    set(CMAKE_C_COMPILER "/opt/homebrew/bin/gcc-14" CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/g++-14" CACHE FILEPATH "C++ compiler" FORCE)
endif()

project(Lab08Shaders LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 3.3 REQUIRED)

add_executable(shader_triangle
    shader_triangle.cpp
    ../LAB1/glad_build/src/glad.c
)
target_include_directories(shader_triangle PRIVATE ../LAB1/glad_build/include)
target_link_libraries(shader_triangle PRIVATE glfw)

# Creates the reference screenshot when a graphical OpenGL context is not
# available (for example, automated/headless execution).
add_executable(shader_preview shader_preview.cpp)

if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(shader_triangle PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:_Alignof=alignof>")
    target_compile_definitions(shader_preview PRIVATE _Alignof=alignof)
endif()

if(APPLE)
    target_link_libraries(shader_triangle PRIVATE "-framework Cocoa" "-framework IOKit" "-framework OpenGL")
endif()
