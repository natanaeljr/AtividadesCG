cmake_minimum_required(VERSION 3.5)
project(AtividadesCG LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(glad)
target_sources(glad PRIVATE dependencies/GLAD-linux/src/glad.c)
target_include_directories(glad PUBLIC dependencies/GLAD-linux/include)

add_subdirectory("Hello3D")
add_subdirectory("Hello3D - Cube")
add_subdirectory("Hello3D - OBJ")
