#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <optional>
#include <vector>

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
	const bool enableValidation = false;
#else
	const bool enableValidation = true;
#endif

bool checkValidationLayerSupport() {
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> available(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, available.data());

	for (const char* wanted : validationLayers) {
		bool found = false;
		for (const auto& layer : available) {
			if (strcmp(wanted, layer.layerName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) return false;
	}
	return true;
}

VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
GLFWwindow* window;
VkDevice device;
VkQueue graphicsQueue;
VkQueue presentQueue;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, details.formats.data());
	}

	uint32_t modeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &modeCount, nullptr);
	if (modeCount != 0) {
		details.presentModes.resize(modeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &modeCount, details.presentModes.data());
	}

	return details;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice dev) {
	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);

	std::vector<VkExtensionProperties> available(extCount);
	vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, available.data());

	std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& ext : available) {
		required.erase(ext.extensionName);
	}
	return required.empty();
}

void createSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		printf("Failed to create window surface!\n");
		exit(1);
	}
	printf("Surface created.\n");
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

	for (uint32_t i = 0; i < familyCount; i++) {
		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if ( presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) break;
	}

	return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = findQueueFamilies(device);
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	return indices.isComplete() && extensionsSupported;
}

void pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		printf("No GPUs with Vulkan support found!\n");
		exit(1);
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		printf("No suitable GPU found!\n");
		exit(1);
	}

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(physicalDevice, &props);
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	printf("Selected GPU: %s\n", props.deviceName);
	printf("	graphics family		%u\n", indices.graphicsFamily.value());
	printf("	present family:	%u\n", indices.presentFamily.value());

	SwapChainSupportDetails swap = querySwapChainSupport(physicalDevice);
	printf("Swapchain support:\n");
	printf("	formats available: %zu\n", swap.formats.size());
	printf("	present modes available: %zu\n", swap.presentModes.size());
	printf("	min image counts: %u, max image count: %u\n", swap.capabilities.minImageCount, swap.capabilities.maxImageCount);
	printf("	current extent: %u x %u\n", swap.capabilities.currentExtent.width, swap.capabilities.currentExtent.height);
}

void createInstance() {
	if (enableValidation && !checkValidationLayerSupport()) {
		printf("Validation layers requested but not available!\n");
		exit(1);
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	uint32_t glfwExtCount = 0;
	const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

	printf("GLFW required extension count: %u\n", glfwExtCount);

	if (glfwExts == nullptr) {
		printf("glfwGetRequiredInstanceExtensions returned NULL - GLFW has no vulkan surface support for this platform.\n");
		exit(1);
	}

	for (uint32_t i = 0; i < glfwExtCount; i++) {
		printf("   required: %s\n", glfwExts[i]);
	}

	std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidation) {
		createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS) {
		printf("Failed to create Vulkan instance! VkResult: %d\n", result);
		exit(1);
	}
	
	printf("Vulkan instance created succesfully.\n");
}

void createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	std::set<uint32_t> uniqueFamilies = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	float queuePriority = 1.0f;
	for (uint32_t family : uniqueFamilies) {
		VkDeviceQueueCreateInfo qInfo{};
		qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qInfo.queueFamilyIndex = family;
		qInfo.queueCount = 1;
		qInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(qInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		printf("Failed to create logical device!\n");
		exit(1);
	}

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

	printf("Logical device created, queues retrieved.\n");
}

int main() {
	glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
	
	if (!glfwInit()) {
		printf("glfwInit failed\n");
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(800, 600 ,"Vulkan Triangle", nullptr, nullptr);

	if (!window) {
		printf("glfwCreateWindow failed\n");
		glfwTerminate();
		return 1;
	}
	
	createInstance();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
