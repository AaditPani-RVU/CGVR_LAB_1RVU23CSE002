cmake_minimum_required(VERSION 3.20)

# This Mac has an incomplete Apple C++ standard library installation. Prefer
# Homebrew GCC when it is available; other platforms keep CMake's default
# compiler selection.
if(APPLE AND EXISTS "/opt/homebrew/bin/gcc-14" AND EXISTS "/opt/homebrew/bin/g++-14")
    set(CMAKE_C_COMPILER "/opt/homebrew/bin/gcc-14" CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/g++-14" CACHE FILEPATH "C++ compiler" FORCE)
endif()

project(Lab01 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 3.3 REQUIRED)

function(add_lab_program target source)
    add_executable(${target}
        ${source}
        glad_build/src/glad.c
    )
    target_include_directories(${target} PRIVATE glad_build/include)
    target_link_libraries(${target} PRIVATE glfw)
endfunction()

add_lab_program(bresenham main.cpp)
add_lab_program(dda dda.cpp)

# Apple SDK headers currently use the C11 `_Alignof` spelling in declarations.
# GCC accepts it in C but needs the C++ spelling when compiling main.cpp.
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(bresenham PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:_Alignof=alignof>")
    target_compile_definitions(dda PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:_Alignof=alignof>")
endif()

if(APPLE)
    target_link_libraries(bresenham PRIVATE "-framework Cocoa" "-framework IOKit" "-framework OpenGL")
    target_link_libraries(dda PRIVATE "-framework Cocoa" "-framework IOKit" "-framework OpenGL")
endif()
