#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdio>

static void glfw_error_callback(int code, const char* description) {
	printf("GLFW ERROR %d: %s\n", code, description);
}

int main() {
	glfwSetErrorCallback(glfw_error_callback);

	glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
	
	if (!glfwInit()) {
		printf("glfwInit failed\n");
		return 1;
	}

	int platform = glfwGetPlatform();
	printf("GLFW platform: %d (wayland==%d, x11=%d)\n", platform, GLFW_PLATFORM_WAYLAND, GLFW_PLATFORM_X11);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(800, 600 ,"Vulkan Triangle", nullptr, nullptr);

	if (!window) {
		printf("glfwCreateWindow failed\n");
		glfwTerminate();
		return 1;
	}

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	printf("Vulkan is reachable. %u instance extensions available.\n", extensionCount);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
