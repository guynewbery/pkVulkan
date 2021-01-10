#include "pkGraphicsBackend.h"

#include "graphics/pkGraphicsUtils.h"
#include "graphics/pkGraphicsSurface.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <set>

static GLFWwindow* s_pWindow = nullptr;

VkDebugUtilsMessengerEXT s_debugMessenger;
VmaAllocator s_allocator;

VkInstance s_instance;
VkPhysicalDevice s_physicalDevice = VK_NULL_HANDLE;
VkDevice s_device;
VkCommandPool s_commandPool;

VkQueue s_graphicsQueue;
VkQueue s_presentQueue;

VkSampleCountFlagBits s_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

const std::vector<const char*> validationLayers = 
{
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = 
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef _DEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) 
    {
        return func(s_instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else 
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) 
    {
        func(s_instance, debugMessenger, pAllocator);
    }
}

bool checkValidationLayerSupport() 
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) 
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) 
        {
            if (strcmp(layerName, layerProperties.layerName) == 0) 
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) 
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*> getRequiredExtensions() 
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) 
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugMessengerCallback;
}

void createInstance() 
{
    if (enableValidationLayers && !checkValidationLayerSupport()) 
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "zodquod";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "pocket";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) 
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else 
    {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &s_instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void setupDebugMessenger()
{
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(&createInfo, nullptr, &s_debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) 
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool isDeviceSuitable(VkPhysicalDevice physicalDevice)
{
    PkGraphicsUtilsQueueFamilyIndices indices = pkGraphicsUtils_FindQueueFamilies(physicalDevice, pkGraphicsSurface_GetSurface());

    bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        PkGraphicsUtilsSwapChainSupport swapChainSupport = pkGraphicsUtils_QuerySwapChainSupport(physicalDevice, pkGraphicsSurface_GetSurface());
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

VkSampleCountFlagBits getMaxUsableSampleCount() 
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    pkGraphicsBackend_GetPhysicalDeviceProperties(&physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

void pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(s_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(s_instance, &deviceCount, devices.data());

    for (const auto& physicalDevice : devices)
    {
        if (isDeviceSuitable(physicalDevice))
        {
            s_physicalDevice = physicalDevice;
            s_msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }

    if (s_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void createLogicalDevice() 
{
    PkGraphicsUtilsQueueFamilyIndices indices = pkGraphicsUtils_FindQueueFamilies(s_physicalDevice, pkGraphicsSurface_GetSurface());

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) 
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(s_physicalDevice, &createInfo, nullptr, &s_device) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(s_device, indices.graphicsFamily.value(), 0, &s_graphicsQueue);
    vkGetDeviceQueue(s_device, indices.presentFamily.value(), 0, &s_presentQueue);
}

void createAllocator() 
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorInfo.physicalDevice = s_physicalDevice;
    allocatorInfo.device = s_device;
    allocatorInfo.instance = s_instance;

    if (vmaCreateAllocator(&allocatorInfo, &s_allocator) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create allocator!");
    }
}

void createCommandPool()
{
    PkGraphicsUtilsQueueFamilyIndices queueFamilyIndices = pkGraphicsUtils_FindQueueFamilies(s_physicalDevice, pkGraphicsSurface_GetSurface());

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(s_device, &poolInfo, nullptr, &s_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

VkFormat pkGraphicsBackend_FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
{
    for (VkFormat format : candidates) 
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(s_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) 
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) 
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VmaAllocator pkGraphicsBackend_GetAllocator()
{
    return s_allocator;
}

VkInstance pkGraphicsBackend_GetInstance()
{
    return s_instance;
}

VkPhysicalDevice pkGraphicsBackend_GetPhysicalDevice()
{
    return s_physicalDevice;
}

VkDevice pkGraphicsBackend_GetDevice()
{
    return s_device;
}

VkCommandPool pkGraphicsBackend_GetCommandPool()
{
    return s_commandPool;
}

VkQueue pkGraphicsBackend_GetGraphicsQueue()
{
    return s_graphicsQueue;
}

VkQueue pkGraphicsBackend_GetPresentQueue()
{
    return s_presentQueue;
}

void pkGraphicsBackend_GetPhysicalDeviceProperties(VkPhysicalDeviceProperties* physicalDeviceProperties)
{
    vkGetPhysicalDeviceProperties(s_physicalDevice, physicalDeviceProperties);
}

void pkGraphicsBackend_GetFormatProperties(VkFormat imageFormat, VkFormatProperties* formatProperties)
{
    vkGetPhysicalDeviceFormatProperties(s_physicalDevice, imageFormat, formatProperties);
}

VkSampleCountFlagBits pkGraphicsBackend_GetMaxMsaaSampleCount()
{
    return s_msaaSamples;
}

void pkGraphicsBackend_Initialise(GLFWwindow& rWindow)
{
    s_pWindow = &rWindow;

    createInstance();
    setupDebugMessenger();

    pkGraphicsSurface_Create(s_instance, s_pWindow);

    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
    createCommandPool();
}

void pkGraphicsBackend_Cleanup()
{
    vkDestroyCommandPool(s_device, s_commandPool, nullptr);

    vmaDestroyAllocator(s_allocator);

    vkDestroyDevice(s_device, nullptr);

    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(s_debugMessenger, nullptr);
    }

    pkGraphicsSurface_Destroy(s_instance);

    vkDestroyInstance(s_instance, nullptr);

    s_pWindow = nullptr;
}
