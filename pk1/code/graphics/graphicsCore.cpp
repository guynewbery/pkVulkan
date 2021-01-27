#include "graphicsCore.h"

#include "graphics/graphicsUtils.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <set>

#ifdef _DEBUG
#define VULKAN_VALIDATION_ENABLED 1
#else
#define VULKAN_VALIDATION_ENABLED 0
#endif //_DEBUG

static const uint32_t WINDOW_WIDTH = 1280;
static const uint32_t WINDOW_HEIGHT = 720;

struct PkGraphicsCoreData
{
    GLFWwindow* pWindow = nullptr;
    bool windowResized = false;

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    glm::mat4 viewMatrix = glm::mat4(1.0f);
    float fieldOfView = 45.0f;
    float nearViewPlane = 0.1f;
    float farViewPlane = 100.0f;

    const std::vector<const char*> deviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#if VULKAN_VALIDATION_ENABLED
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    const std::vector<const char*> validationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };
#endif //VULKAN_VALIDATION_ENABLED
};

static PkGraphicsCoreData* s_pData = nullptr;

#if VULKAN_VALIDATION_ENABLED
VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_pData->instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) 
    {
        return func(s_pData->instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else 
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_pData->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) 
    {
        func(s_pData->instance, debugMessenger, pAllocator);
    }
}

static bool checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : s_pData->validationLayers) 
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

static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugMessengerCallback;
}

static void setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(&createInfo, nullptr, &s_pData->debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}
#endif //VULKAN_VALIDATION_ENABLED

static std::vector<const char*> getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if VULKAN_VALIDATION_ENABLED
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif //VULKAN_VALIDATION_ENABLED

    return extensions;
}

static void windowResizeCallback(GLFWwindow* pWindow, int width, int height)
{
    s_pData->windowResized = true;
}

static void createWindow(const char* pWindowName)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    s_pData->pWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, pWindowName, nullptr, nullptr);
    glfwSetFramebufferSizeCallback(s_pData->pWindow, windowResizeCallback);
}

static void createInstance()
{
#if VULKAN_VALIDATION_ENABLED
    if (!checkValidationLayerSupport()) 
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }
#endif //VULKAN_VALIDATION_ENABLED

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

#if VULKAN_VALIDATION_ENABLED
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

    createInfo.enabledLayerCount = static_cast<uint32_t>(s_pData->validationLayers.size());
    createInfo.ppEnabledLayerNames = s_pData->validationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
#endif //VULKAN_VALIDATION_ENABLED

    if (vkCreateInstance(&createInfo, nullptr, &s_pData->instance) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create instance!");
    }
}

static void createSurface()
{
    if (glfwCreateWindowSurface(s_pData->instance, PkGraphicsCore::GetWindow(), nullptr, &s_pData->surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

static bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(s_pData->deviceExtensions.begin(), s_pData->deviceExtensions.end());

    for (const auto& extension : availableExtensions) 
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static bool isDeviceSuitable(VkPhysicalDevice physicalDevice)
{
    PkGraphicsQueueFamilyIndices indices = pkGraphicsUtils_FindQueueFamilies(physicalDevice, PkGraphicsCore::GetSurface());

    bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        PkGraphicsSwapChainSupport swapChainSupport = pkGraphicsUtils_QuerySwapChainSupport(physicalDevice, PkGraphicsCore::GetSurface());
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

static VkSampleCountFlagBits getMaxUsableSampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    PkGraphicsCore::GetPhysicalDeviceProperties(&physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

static void pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(s_pData->instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(s_pData->instance, &deviceCount, devices.data());

    for (const auto& physicalDevice : devices)
    {
        if (isDeviceSuitable(physicalDevice))
        {
            s_pData->physicalDevice = physicalDevice;
            s_pData->msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }

    if (s_pData->physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

static void createLogicalDevice()
{
    PkGraphicsQueueFamilyIndices indices = pkGraphicsUtils_FindQueueFamilies(s_pData->physicalDevice, PkGraphicsCore::GetSurface());

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) 
    {
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

    createInfo.enabledExtensionCount = static_cast<uint32_t>(s_pData->deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = s_pData->deviceExtensions.data();

#if VULKAN_VALIDATION_ENABLED
    createInfo.enabledLayerCount = static_cast<uint32_t>(s_pData->validationLayers.size());
    createInfo.ppEnabledLayerNames = s_pData->validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif //VULKAN_VALIDATION_ENABLED

    if (vkCreateDevice(s_pData->physicalDevice, &createInfo, nullptr, &s_pData->device) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(s_pData->device, indices.graphicsFamily.value(), 0, &s_pData->graphicsQueue);
    vkGetDeviceQueue(s_pData->device, indices.presentFamily.value(), 0, &s_pData->presentQueue);
}

static void createAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorInfo.physicalDevice = s_pData->physicalDevice;
    allocatorInfo.device = s_pData->device;
    allocatorInfo.instance = s_pData->instance;

    if (vmaCreateAllocator(&allocatorInfo, &s_pData->allocator) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create allocator!");
    }
}

/*static*/ GLFWwindow* PkGraphicsCore::GetWindow()
{
    return s_pData->pWindow;
}

/*static*/ bool PkGraphicsCore::HasWindowBeenResized()
{
    return s_pData->windowResized;
}

/*static*/ void PkGraphicsCore::ResetWindowResizedFlag()
{
    s_pData->windowResized = false;
}

/*static*/ VkInstance PkGraphicsCore::GetInstance()
{
    return s_pData->instance;
}

/*static*/ VkSurfaceKHR PkGraphicsCore::GetSurface()
{
    return s_pData->surface;
}

/*static*/ VkPhysicalDevice PkGraphicsCore::GetPhysicalDevice()
{
    return s_pData->physicalDevice;
}

/*static*/ VkDevice PkGraphicsCore::GetDevice()
{
    return s_pData->device;
}

/*static*/ VmaAllocator PkGraphicsCore::GetAllocator()
{
    return s_pData->allocator;
}

/*static*/ VkQueue PkGraphicsCore::GetGraphicsQueue()
{
    return s_pData->graphicsQueue;
}

/*static*/ VkQueue PkGraphicsCore::GetPresentQueue()
{
    return s_pData->presentQueue;
}

/*static*/ void PkGraphicsCore::GetPhysicalDeviceProperties(VkPhysicalDeviceProperties* physicalDeviceProperties)
{
    vkGetPhysicalDeviceProperties(s_pData->physicalDevice, physicalDeviceProperties);
}

/*static*/ void PkGraphicsCore::GetFormatProperties(VkFormat imageFormat, VkFormatProperties* formatProperties)
{
    vkGetPhysicalDeviceFormatProperties(s_pData->physicalDevice, imageFormat, formatProperties);
}

/*static*/ VkSampleCountFlagBits PkGraphicsCore::GetMaxMsaaSampleCount()
{
    return s_pData->msaaSamples;
}

/*static*/ glm::mat4& PkGraphicsCore::GetViewMatrix()
{
    return s_pData->viewMatrix;
}

/*static*/ void PkGraphicsCore::SetViewMatrix(const glm::mat4& rMat)
{
    s_pData->viewMatrix = rMat;
}

/*static*/ float PkGraphicsCore::GetFieldOfView()
{
    return s_pData->fieldOfView;
}

/*static*/ void PkGraphicsCore::SetFieldOfView(const float fov)
{
    s_pData->fieldOfView = fov;
}

/*static*/ float PkGraphicsCore::GetNearViewPlane()
{
    return 0.1f;
}

/*static*/ float PkGraphicsCore::GetFarViewPlane()
{
    return 100.0f;
}

/*static*/ void PkGraphicsCore::InitialiseGraphicsCore(const char* pWindowName)
{
    s_pData = new PkGraphicsCoreData();

    createWindow(pWindowName);
    createInstance();

#if VULKAN_VALIDATION_ENABLED
    setupDebugMessenger();
#endif //VULKAN_VALIDATION_ENABLED

    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
}

/*static*/ void PkGraphicsCore::CleanupGraphicsCore()
{
    vmaDestroyAllocator(s_pData->allocator);
    vkDestroyDevice(s_pData->device, nullptr);
    vkDestroySurfaceKHR(s_pData->instance, s_pData->surface, nullptr);

#if VULKAN_VALIDATION_ENABLED
    DestroyDebugUtilsMessengerEXT(s_pData->debugMessenger, nullptr);
#endif //VULKAN_VALIDATION_ENABLED

    vkDestroyInstance(s_pData->instance, nullptr);
    glfwDestroyWindow(s_pData->pWindow);

    delete s_pData;
}
