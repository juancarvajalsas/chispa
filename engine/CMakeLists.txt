find_program(GLSLC glslc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin" REQUIRED)

file(GLOB SHADER_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
	"${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp"
)

set(SPV_OUTPUTS "")
foreach(SHADER ${SHADER_SOURCES})
	get_filename_component(FILENAME ${SHADER} NAME)
	set(SPV "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${FILENAME}.spv")
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
add_library(chispa STATIC engine.cpp)
target_include_directories(chispa PUBLIC ${CMAKE_SOURCE_DIR} third_party)
target_link_libraries(chispa PUBLIC Vulkan::Vulkan glfw)
add_dependencies(chispa shaders)

