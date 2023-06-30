cmake_minimum_required(VERSION 3.5)
project(AtividadesCG LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Debug")

add_library(glad)
target_sources(glad PRIVATE dependencies/GLAD-linux/src/glad.c)
target_include_directories(glad PUBLIC dependencies/GLAD-linux/include)

add_subdirectory("Hello3D")
add_subdirectory("Hello3D - Cube")
add_subdirectory("Hello3D - OBJ")
add_subdirectory("Hello3D - OBJ - Texture")
add_subdirectory("NewHello3D - Lighting")
add_subdirectory("NewHello3D")
