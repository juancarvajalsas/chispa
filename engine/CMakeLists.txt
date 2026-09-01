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

# --- The engine as a library ---
add_library(myengine STATIC engine.cpp)
target_include_directories(myengine PUBLIC ${CMAKE_SOURCE_DIR} third_party)
target_link_libraries(myengine PUBLIC Vulkan::Vulkan glfw)
add_dependencies(myengine shaders)

