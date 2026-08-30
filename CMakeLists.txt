cmake_minimum_required(VERSION 3.20)
project(vk_triangle)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

find_program(GLSLC glslc REQUIRED)

file(GLOB SHADER_SOURCES
	"${CMAKE_SOURCE_DIR}/shaders/*.vert"
	"${CMAKE_SOURCE_DIR}/shaders/*.frag"
	"${CMAKE_SOURCE_DIR}/shaders/*.comp"
)

set(SPV_OUTPUTS "")
foreach(SHADER ${SHADER_SOURCES})
	get_filename_component(FILENAME ${SHADER} NAME)
	set(SPV "${CMAKE_SOURCE_DIR}/shaders/${FILENAME}.spv")
	add_custom_command(
		OUTPUT ${SPV}
		COMMAND ${GLSLC} ${SHADER} -o ${SPV}
		DEPENDS ${SHADER}
		COMMENT "Compiling ${FILENAME}"
	)
	list(APPEND SPV_OUTPUTS ${SPV})
endforeach()

add_custom_target(shaders DEPENDS ${SPV_OUTPUTS})

add_executable(vk_triangle src/main.cpp src/engine.cpp)

add_dependencies(vk_triangle shaders)
target_include_directories(vk_triangle PRIVATE third_party)
target_link_libraries(vk_triangle
	Vulkan::Vulkan
	glfw
)
