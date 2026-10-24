cmake_minimum_required(VERSION 3.20)
project(chispa)
if (MSVC)
	add_compile_definitions(
		NOMINMAX
		_USE_MATH_DEFINES
		_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
	include(FetchContent)
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(glfw
		GIT_REPOSITORY https://github.com/glfw/glfw.git
		GIT_TAG 3.4)
	FetchContent_MakeAvailable(glfw)
endif()

add_subdirectory(engine)
add_subdirectory(pong)
add_subdirectory(tetris)
add_subdirectory(asteroids)
