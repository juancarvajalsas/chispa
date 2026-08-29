glslc shaders/triangle.frag -o shaders/triangle.frag.spv
glslc shaders/triangle.vert -o shaders/triangle.vert.spv

cmake --build build
./build/vk_triangle
