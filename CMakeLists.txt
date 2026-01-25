cmake_minimum_required(VERSION 4.0)
project(CubeRender C)

set(CMAKE_C_STANDARD 11)

set(SDL3_DIR "C:/c_libs/SDL3-3.4.0/cmake")
find_package(SDL3 CONFIG REQUIRED)

add_executable(CubeRender main.c
        window.c)

target_link_libraries(CubeRender PRIVATE SDL3::SDL3)