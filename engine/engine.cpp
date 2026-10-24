#include "engine.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

static const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
static const bool enableValidation = false;
#else
static const bool enableValidation = true;
#endif

static bool checkValidationLayerSupport() {
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> available(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, available.data());

  for (const char *wanted : validationLayers) {
    bool found = false;
    for (const auto &layer : available) {
      if (strcmp(wanted, layer.layerName) == 0) {
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }
  return true;
}

static VkInstance instance;
static VkSurfaceKHR surface;
static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
static GLFWwindow *window;
static VkDevice device;
static VkQueue graphicsQueue;
static VkQueue presentQueue;
static VkSwapchainKHR swapChain;
static std::vector<VkImage> swapChainImages;
static VkFormat swapChainImageFormat;
static VkExtent2D swapChainExtent;
static VkPipelineLayout pipelineLayout;
static VkPipeline graphicsPipeline;
static VkCommandPool commandPool;
static VkCommandBuffer commandBuffer;
static VkSemaphore imageAvailableSemaphore;
static std::vector<VkSemaphore> renderFinishedSemaphore;
static VkFence inFlighFence;
static VkDescriptorSetLayout descriptorSetLayout;
static VkDescriptorPool descriptorPool;
static uint32_t currentImageIndex = 0;
static std::unordered_map<int, bool> prevKeyState;
static VmaAllocator allocator;

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

struct Texture {
  VkImage image;
  VmaAllocation allocation;
  VkImageView view;
  VkSampler sampler;
  VkDescriptorSet descriptorSet;
};

struct PushConstants {
  float offset[2];
  float halfSize[2];
  float proj[16];
  float uvOffset[2];
  float uvScale[2];
  int useTexture;
  float angle;
};

static const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static std::vector<VkImageView> swapChainImageViews;
static const uint32_t TEX_SIZE = 8;
static std::vector<Texture> textures;

static void createAllocator() {
  VmaAllocatorCreateInfo info{};
  info.physicalDevice = physicalDevice;
  info.device = device;
  info.instance = instance;
  info.vulkanApiVersion = VK_API_VERSION_1_3;

  if (vmaCreateAllocator(&info, &allocator) != VK_SUCCESS) {
    printf("Failed to create VWA allocator!\n");
    exit(1);
  }
  printf("VMA allocator created.\n");
}

static std::array<float, 16> makeOrthoMatrix(float aspect) {
  float sx = 1.0f / aspect;
  float sy = 1.0f;

  // 4x4 matrix
  return {sx,   0.0f, 0.0f, 0.0f, 0.0f, sy,   0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
}

static void createInstance() {
  if (enableValidation && !checkValidationLayerSupport()) {
    printf("Validation layers requested but not available!\n");
    exit(1);
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Chispa";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Chispa";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  uint32_t glfwExtCount = 0;
  const char **glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

  printf("GLFW required extension count: %u\n", glfwExtCount);

  if (glfwExts == nullptr) {
    printf("glfwGetRequiredInstanceExtensions returned NULL - GLFW has no "
           "vulkan surface support for this platform.\n");
    exit(1);
  }

  for (uint32_t i = 0; i < glfwExtCount; i++) {
    printf("   required: %s\n", glfwExts[i]);
  }

  std::vector<const char *> extensions(glfwExts, glfwExts + glfwExtCount);

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

static void createSurface() {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    printf("Failed to create window surface!\n");
    exit(1);
  }
  printf("Surface created.\n");
}

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t familyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
  std::vector<VkQueueFamilyProperties> families(familyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount,
                                           families.data());

  for (uint32_t i = 0; i < familyCount; i++) {
    if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete())
      break;
  }

  return indices;
}

static bool checkDeviceExtensionSupport(VkPhysicalDevice dev) {
  uint32_t extCount = 0;
  vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);

  std::vector<VkExtensionProperties> available(extCount);
  vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount,
                                       available.data());

  std::set<std::string> required(deviceExtensions.begin(),
                                 deviceExtensions.end());
  for (const auto &ext : available) {
    required.erase(ext.extensionName);
  }
  return required.empty();
}

static bool isDeviceSuitable(VkPhysicalDevice device) {
  QueueFamilyIndices indices = findQueueFamilies(device);
  bool extensionsSupported = checkDeviceExtensionSupport(device);
  return indices.isComplete() && extensionsSupported;
}

static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface,
                                            &details.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t modeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &modeCount, nullptr);
  if (modeCount != 0) {
    details.presentModes.resize(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &modeCount,
                                              details.presentModes.data());
  }

  return details;
}

static void pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    printf("No GPUs with Vulkan support found!\n");
    exit(1);
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto &device : devices) {
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
  printf("	min image counts: %u, max image count: %u\n",
         swap.capabilities.minImageCount, swap.capabilities.maxImageCount);
  printf("	current extent: %u x %u\n",
         swap.capabilities.currentExtent.width,
         swap.capabilities.currentExtent.height);
}

static void createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  std::set<uint32_t> uniqueFamilies = {indices.graphicsFamily.value(),
                                       indices.presentFamily.value()};

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

  VkPhysicalDeviceSynchronization2Features sync2{};
  sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
  sync2.synchronization2 = VK_TRUE;

  VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
  dynamicRendering.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  dynamicRendering.dynamicRendering = VK_TRUE;
  dynamicRendering.pNext = &sync2;

  createInfo.pNext = &dynamicRendering;

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    printf("Failed to create logical device!\n");
    exit(1);
  }

  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

  printf("Logical device created, queues retrieved.\n");
}

static VkSurfaceFormatKHR
chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available) {
  for (const auto &f : available) {
    if (f.format == VK_FORMAT_R8G8B8A8_UNORM &&
        f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return f;
    }
  }
  return available[0];
}

static VkPresentModeKHR
chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available) {
  for (const auto &m : available) {
    if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
      return m;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &caps) {
  if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return caps.currentExtent;
  }

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  VkExtent2D actual = {(uint32_t)width, (uint32_t)height};
  actual.width = std::clamp(actual.width, caps.minImageExtent.width,
                            caps.maxImageExtent.width);
  actual.height = std::clamp(actual.height, caps.minImageExtent.height,
                             caps.maxImageExtent.height);

  return actual;
}

static void createSwapChain() {
  SwapChainSupportDetails support = querySwapChainSupport(physicalDevice);
  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
  VkExtent2D extent = chooseSwapExtent(support.capabilities);

  uint32_t imageCount = support.capabilities.minImageCount + 1;
  if (support.capabilities.maxImageCount > 0 &&
      imageCount > support.capabilities.maxImageCount) {
    imageCount = support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  uint32_t familyIndices[] = {indices.graphicsFamily.value(),
                              indices.presentFamily.value()};
  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = familyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = support.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
      VK_SUCCESS) {
    printf("Failed to create swap chain!\n");
    exit(1);
  }

  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                          swapChainImages.data());

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;

  printf("Swapchain created. Images: %u, extent %u x %u\n", imageCount,
         extent.width, extent.height);
}

static void createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());

  for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapChainImageFormat;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &createInfo, nullptr,
                          &swapChainImageViews[i]) != VK_SUCCESS) {
      printf("Failed to create image view %zu!\n", i);
      exit(1);
    }
  }

  printf("Created %zu image views.\n", swapChainImageViews.size());
}

static std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    printf("Failed to open file: %s\n", filename.c_str());
    exit(1);
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  return buffer;
}

static VkShaderModule createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    printf("Failed to create shader module!\n");
    exit(1);
  }
  return shaderModule;
}

static void createGraphicsPipeline() {
  auto vertCode = readFile("engine/shaders/quad.vert.spv");
  auto fragCode = readFile("engine/shaders/quad.frag.spv");
  VkShaderModule vert = createShaderModule(vertCode);
  VkShaderModule frag = createShaderModule(fragCode);

  // -- Stage 1 Shader stages
  VkPipelineShaderStageCreateInfo vertStage{};
  vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertStage.module = vert;
  vertStage.pName = "main";

  VkPipelineShaderStageCreateInfo fragStage{};
  fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragStage.module = frag;
  fragStage.pName = "main";

  VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

  // -- Stage 2 Vertex input - EMPTY
  VkPipelineVertexInputStateCreateInfo vertexInput{};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount = 0;
  vertexInput.vertexAttributeDescriptionCount = 0;

  // -- Stage 3 input assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // -- Stage 4 viewport and scissor
  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  // -- Stage 5 rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  // -- Stage 6 multisampling
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // -- Stage 7 color blending
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  // color: src * srcAlpha + dst * (1 - srcAlpha)
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  // alpha channel itself: keep the source alpha
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
  dynamicState.pDynamicStates = dynamicStates.data();

  // -- Stage 8 Pipeline Layout
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstants);

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &descriptorSetLayout;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) !=
      VK_SUCCESS) {
    printf("Failed to create pipeline layout!\n");
    exit(1);
  }

  // -- Stage 9 dynamic rendering
  VkPipelineRenderingCreateInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachmentFormats = &swapChainImageFormat;

  // -- Assemble everything
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = &renderingInfo;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = stages;
  pipelineInfo.pVertexInputState = &vertexInput;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = VK_NULL_HANDLE;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &graphicsPipeline) != VK_SUCCESS) {
    printf("Failed to create graphics pipeline!\n");
    exit(1);
  }

  vkDestroyShaderModule(device, vert, nullptr);
  vkDestroyShaderModule(device, frag, nullptr);

  printf("Graphics pipeline created.\n");
}

static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                         VmaMemoryUsage memUsage, VkBuffer &buffer,
                         VmaAllocation &allocation) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = memUsage;
  allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

  if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation,
                      nullptr) != VK_SUCCESS) {
    printf("Failed to create buffer");
    exit(1);
  }
}

static std::vector<uint32_t> generateCheckerboard() {
  std::vector<uint32_t> pixels(static_cast<uint32_t>(TEX_SIZE * TEX_SIZE));
  for (uint32_t y = 0; y < TEX_SIZE; y++) {
    for (uint32_t x = 0; x < TEX_SIZE; x++) {
      bool white = ((x + y) % 2) == 0;
      pixels[y * TEX_SIZE + x] = white ? 0xFFFFFFFF : 0xFF000000;
    }
  }
  return pixels;
}

static VkCommandBuffer beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;
  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(device, &allocInfo, &cmd);

  VkCommandBufferBeginInfo begin{};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &begin);
  return cmd;
}

static void endSingleTimeCommands(VkCommandBuffer cmd) {
  vkEndCommandBuffer(cmd);
  VkSubmitInfo submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &cmd;
  vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);
  vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}

static void transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                  VkImageLayout oldLayout,
                                  VkImageLayout newLayout,
                                  VkPipelineStageFlags2 srcStage,
                                  VkAccessFlags2 srcAccess,
                                  VkPipelineStageFlags2 dstStage,
                                  VkAccessFlags2 dstAccess) {
  VkImageMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrier.srcStageMask = srcStage;
  barrier.srcAccessMask = srcAccess;
  barrier.dstStageMask = dstStage;
  barrier.dstAccessMask = dstAccess;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.layerCount = 1;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &barrier;

  vkCmdPipelineBarrier2(cmd, &depInfo);
}

static void createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding samplerBinding{};
  samplerBinding.binding = 0;
  samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerBinding.descriptorCount = 1;
  samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = 1;
  info.pBindings = &samplerBinding;

  if (vkCreateDescriptorSetLayout(device, &info, nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    printf("Failed to create descriptor set layout!\n");
    exit(1);
  }

  printf("Descriptor set layout created.\n");
}

static void createDescriptorPool() {
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = 64;

  VkDescriptorPoolCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.poolSizeCount = 1;
  info.pPoolSizes = &poolSize;
  info.maxSets = 64;

  if (vkCreateDescriptorPool(device, &info, nullptr, &descriptorPool) !=
      VK_SUCCESS) {
    printf("Failed to create descriptor pool!\n");
    exit(1);
  }
  printf("Descriptor pool created.\n");
}

static void createCommandPool() {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    printf("Failed to create command pool!\n");
    exit(1);
  }
  printf("Command pool created\n");
}

static void createCommandBuffer() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) !=
      VK_SUCCESS) {
    printf("Failed to allocate command buffer!\n");
    exit(1);
  }
  printf("Command buffer allocated!\n");
}

static void createSyncObjects() {
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                        &imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateFence(device, &fenceInfo, nullptr, &inFlighFence) != VK_SUCCESS) {
    printf("Failed to create sync objects!\n");
    exit(1);
  }

  renderFinishedSemaphore.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphore[i]) != VK_SUCCESS) {
      printf("Failed to create render-finished semaphore %zu!\n", i);
      exit(1);
    }
  }
  printf("Sync objects created.\n");
}

void engineInit(int width, int height, const char *title) {
  glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);

  if (!glfwInit()) {
    printf("glfwInit failed\n");
    exit(1);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(width, height, title, nullptr, nullptr);

  if (!window) {
    printf("glfwCreateWindow failed\n");
    glfwTerminate();
    exit(1);
  }

  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createAllocator();
  createSwapChain();
  createImageViews();
  createDescriptorSetLayout();
  createGraphicsPipeline();
  createCommandPool();
  createDescriptorPool();
  createCommandBuffer();
  createSyncObjects();

  printf("Engine Initialized");
}

void engineShutdown() {
  vkDeviceWaitIdle(device);

  for (auto &tex : textures) {
    vkDestroySampler(device, tex.sampler, nullptr);
    vkDestroyImageView(device, tex.view, nullptr);
    vmaDestroyImage(allocator, tex.image, tex.allocation);
  }

  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
  for (auto sem : renderFinishedSemaphore) {
    vkDestroySemaphore(device, sem, nullptr);
  }
  vkDestroyFence(device, inFlighFence, nullptr);
  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

  for (auto view : swapChainImageViews) {
    vkDestroyImageView(device, view, nullptr);
  }

  vkDestroySwapchainKHR(device, swapChain, nullptr);
  vmaDestroyAllocator(allocator);
  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
  printf("Engine shut down.\n");
}

void engineBeginFrame() {
  vkWaitForFences(device, 1, &inFlighFence, VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &inFlighFence);

  vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore,
                        VK_NULL_HANDLE, &currentImageIndex);

  vkResetCommandBuffer(commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    printf("Failed to begin command buffer!\n");
    exit(1);
  }

  transitionImageLayout(commandBuffer, swapChainImages[currentImageIndex],
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
                        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = swapChainImageViews[currentImageIndex];
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

  VkRenderingInfo renderInfo{};
  renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderInfo.renderArea.offset = {0, 0};
  renderInfo.renderArea.extent = swapChainExtent;
  renderInfo.layerCount = 1;
  renderInfo.colorAttachmentCount = 1;
  renderInfo.pColorAttachments = &colorAttachment;

  vkCmdBeginRendering(commandBuffer, &renderInfo);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

static void drawInternal(int texHandle, float x, float y, float hw, float hh,
                         float uvX, float uvY, float uvW, float uvH,
                         int useTexture, float angle) {
  if (useTexture && texHandle >= 0 && texHandle < (int)textures.size()) {
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1,
                            &textures[texHandle].descriptorSet, 0, nullptr);
  } else if (!useTexture && (int)textures.size() > 0) {
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &textures[0].descriptorSet, 0,
                            nullptr);
  } else {
    printf("Need to have at least one Texture!\n");
    exit(1);
  }
  float aspect = (float)swapChainExtent.width / (float)swapChainExtent.height;
  auto proj = makeOrthoMatrix(aspect);

  PushConstants pc{};
  pc.offset[0] = x;
  pc.offset[1] = y;
  pc.halfSize[0] = hw;
  pc.halfSize[1] = hh;
  pc.useTexture = useTexture;
  pc.angle = angle;

  for (int i = 0; i < 16; i++) {
    pc.proj[i] = proj[i];
  }

  pc.uvOffset[0] = uvX;
  pc.uvOffset[1] = uvY;
  pc.uvScale[0] = uvW;
  pc.uvScale[1] = uvH;
  vkCmdPushConstants(commandBuffer, pipelineLayout,
                     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                     0, sizeof(PushConstants), &pc);

  vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void drawQuad(float x, float y, float hw, float hh, float angle) {
  drawInternal(-1, x, y, hw, hh, 0, 0, 1, 1, 0, angle);
}

void drawSprite(int handle, float x, float y, float hw, float hh, float uvX,
                float uvY, float uvW, float uvH, float angle) {
  drawInternal(handle, x, y, hw, hh, uvX, uvY, uvW, uvH, 1, angle);
}

void engineEndFrame() {
  vkCmdEndRendering(commandBuffer);

  transitionImageLayout(commandBuffer, swapChainImages[currentImageIndex],
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    printf("Failed to end command buffer!\n");
    exit(1);
  }

  VkSemaphoreSubmitInfo waitSem{};
  waitSem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  waitSem.semaphore = imageAvailableSemaphore;
  waitSem.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSemaphoreSubmitInfo signalSem{};
  signalSem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  signalSem.semaphore = renderFinishedSemaphore[currentImageIndex];
  signalSem.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkCommandBufferSubmitInfo cmdInfo{};
  cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmdInfo.commandBuffer = commandBuffer;

  VkSubmitInfo2 submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit.waitSemaphoreInfoCount = 1;
  submit.pWaitSemaphoreInfos = &waitSem;
  submit.signalSemaphoreInfoCount = 1;
  submit.pSignalSemaphoreInfos = &signalSem;
  submit.commandBufferInfoCount = 1;
  submit.pCommandBufferInfos = &cmdInfo;

  if (vkQueueSubmit2(graphicsQueue, 1, &submit, inFlighFence) != VK_SUCCESS) {
    printf("Failed to submit draw!\n");
    exit(1);
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &renderFinishedSemaphore[currentImageIndex];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapChain;
  presentInfo.pImageIndices = &currentImageIndex;

  vkQueuePresentKHR(presentQueue, &presentInfo);
}

bool engineShouldClose() { return glfwWindowShouldClose(window); }

void enginePollEvents() { glfwPollEvents(); }

bool isKeyDown(int key) { return glfwGetKey(window, key); }

bool isKeyPressed(int key) {
  bool down = glfwGetKey(window, key) == GLFW_PRESS;
  bool wasDown = prevKeyState[key];
  prevKeyState[key] = down;
  return down && !wasDown;
}

int loadTexture(const char *path, Filter filter) {
  Texture tex{};

  int w, h, ch;
  stbi_uc *pixels = stbi_load(path, &w, &h, &ch, STBI_rgb_alpha);
  if (!pixels) {
    printf("Failed to load texture: %s (%s)\n", path, stbi_failure_reason());
    return -1;
  }

  VkDeviceSize imageSize = w * h * 4;

  VkBuffer staging;
  VmaAllocation stagingAlloc;
  createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VMA_MEMORY_USAGE_AUTO, staging, stagingAlloc);

  void *data;
  vmaMapMemory(allocator, stagingAlloc, &data);
  memcpy(data, pixels, (size_t)imageSize);
  vmaUnmapMemory(allocator, stagingAlloc);
  stbi_image_free(pixels);

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent = {(uint32_t)w, (uint32_t)h, 1};
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo imgAllocInfo{};
  imgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

  if (vmaCreateImage(allocator, &imageInfo, &imgAllocInfo, &tex.image,
                     &tex.allocation, nullptr) != VK_SUCCESS) {
    printf("Failed to create texture image (VMA)!\n");
    exit(1);
  }

  VkCommandBuffer cmd = beginSingleTimeCommands();
  transitionImageLayout(
      cmd, tex.image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
      0, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

  VkBufferImageCopy region{};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount = 1;
  region.imageExtent = {(uint32_t)w, (uint32_t)h, 1};
  vkCmdCopyBufferToImage(cmd, staging, tex.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  transitionImageLayout(
      cmd, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
  endSingleTimeCommands(cmd);
  vmaDestroyBuffer(allocator, staging, stagingAlloc);

  VkImageViewCreateInfo imageViewInfo{};
  imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewInfo.image = tex.image;
  imageViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageViewInfo.subresourceRange.layerCount = 1;
  imageViewInfo.subresourceRange.levelCount = 1;
  vkCreateImageView(device, &imageViewInfo, nullptr, &tex.view);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  VkFilter vkFilter =
      (filter == Filter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST);
  samplerInfo.magFilter = VK_FILTER_NEAREST;
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  vkCreateSampler(device, &samplerInfo, nullptr, &tex.sampler);

  VkDescriptorSetAllocateInfo dsAlloc{};
  dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsAlloc.descriptorPool = descriptorPool;
  dsAlloc.descriptorSetCount = 1;
  dsAlloc.pSetLayouts = &descriptorSetLayout;
  vkAllocateDescriptorSets(device, &dsAlloc, &tex.descriptorSet);

  VkDescriptorImageInfo imgInfo{};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgInfo.imageView = tex.view;
  imgInfo.sampler = tex.sampler;
  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = tex.descriptorSet;
  write.dstBinding = 0;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.descriptorCount = 1;
  write.pImageInfo = &imgInfo;
  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  textures.push_back(tex);
  int handle = (int)textures.size() - 1;
  printf("Load texture [%d]: %s (%dx%d)\n", handle, path, w, h);
  return handle;
}
