cmake_minimum_required(VERSION 3.20)
project(gamedev)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_subdirectory(engine)
add_subdirectory(pong)
#add_subdirectory(tetris)
