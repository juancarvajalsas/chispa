cmake_minimum_required(VERSION 3.20)
project(vk_triangle)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(vk_triangle src/main.cpp)

target_include_directories(vk_triangle PRIVATE third_party)
target_link_libraries(vk_triangle
	Vulkan::Vulkan
	glfw
)
