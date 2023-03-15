// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#pragma once

static VkPhysicalDevice Vulkan_FindPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface, uint32_t ExtCount, const char **ExtNames);
static int32_t Vulkan_DefaultQueueFamilyIndex(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
static int32_t Vulkan_TransferQueueFamilyIndex(VkPhysicalDevice PhysicalDevice);
static int32_t Vulkan_FindPhysicalDeviceMemoryIndex(VkPhysicalDevice PhysicalDevice, uint32_t TypeFilter, VkMemoryPropertyFlagBits Properties);

static VkSurfaceFormatKHR Vulkan_GetPhysicalDeviceSurfaceFormat(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
static VkPresentModeKHR Vulkan_GetPhysicalDeviceSurfacePresentMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
static bool Vulkan_CheckLayers(uint32_t LayerCount, const char **LayerNames);
static bool Vulkan_CheckDeviceExtensions(VkPhysicalDevice PhysicalDevice, uint32_t ReqExtensionCount, const char **ReqExtensionNames);
static bool Vulkan_CheckSwapChainSupport(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);

//
//
//

static VkPhysicalDevice Vulkan_FindPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface, 
                                           uint32_t ExtCount, const char **ExtNames)
{
    uint32_t DeviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, 0);
    if (DeviceCount == 0)
        return VK_NULL_HANDLE;

    VkPhysicalDevice *Devices = malloc(sizeof(VkPhysicalDevice) * DeviceCount);
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, Devices);

    VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    for (uint32_t i = 0; i < DeviceCount; ++i)
    {
        VkPhysicalDeviceProperties Properties;
        vkGetPhysicalDeviceProperties(Devices[i], &Properties);
        VkPhysicalDeviceFeatures Features;
        vkGetPhysicalDeviceFeatures(Devices[i], &Features);
        VkFormatProperties FormatProperties;
        vkGetPhysicalDeviceFormatProperties(Devices[i], VK_FORMAT_D32_SFLOAT, &FormatProperties);
        
        if (Vulkan_DefaultQueueFamilyIndex(Devices[i], Surface) == -1) continue;
        if (!Vulkan_CheckDeviceExtensions(Devices[i], ExtCount, ExtNames)) continue;
        if (!Vulkan_CheckSwapChainSupport(Devices[i], Surface)) continue;
        if (!Features.samplerAnisotropy) continue;
        if (!(FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) continue;
        
        PhysicalDevice = Devices[i];
        break;
    }

    free(Devices);
    return PhysicalDevice;
}

static int32_t Vulkan_DefaultQueueFamilyIndex(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
{
    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, 0);

    VkQueueFamilyProperties *QueueFamilies = malloc(sizeof(VkQueueFamilyProperties) * QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies);

    int32_t QueueFamily = -1;
    for (uint32_t i = 0; i < QueueFamilyCount; ++i)
    {
        if (Surface != VK_NULL_HANDLE)
        {
            VkBool32 PresentSupport = true;
            vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &PresentSupport);
            if (!PresentSupport) continue;
        }
        
        if (QueueFamilies[i].queueCount > 0 && 
            (QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            QueueFamily = i;
            break;
        }
    }

    free(QueueFamilies);
    return QueueFamily;
}

static int32_t Vulkan_TransferQueueFamilyIndex(VkPhysicalDevice PhysicalDevice)
{
    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, 0);

    VkQueueFamilyProperties *QueueFamilies = malloc(sizeof(VkQueueFamilyProperties) * QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies);

    int32_t QueueFamily = -1;
    for (uint32_t i = 0; i < QueueFamilyCount; ++i)
    {
        if (QueueFamilies[i].queueCount > 0 && 
            (QueueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
        {
            QueueFamily = i;
            if (!(QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                !(QueueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
                break;
        }
    }

    free(QueueFamilies);
    return QueueFamily;
}

static int32_t Vulkan_FindPhysicalDeviceMemoryIndex(VkPhysicalDevice PhysicalDevice, uint32_t TypeFilter, VkMemoryPropertyFlagBits Properties)
{
    VkPhysicalDeviceMemoryProperties MemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);
    for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i)
        if ((TypeFilter & (1 << i)))
            if ((MemoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
                return i;

    return -1;
}

static VkSurfaceFormatKHR Vulkan_GetPhysicalDeviceSurfaceFormat(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
{
    uint32_t FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatCount, 0);
    if (FormatCount == 1)
    {
        VkSurfaceFormatKHR SurfaceFormat;
        vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatCount, &SurfaceFormat);
        return SurfaceFormat;
    }
    
    VkSurfaceFormatKHR *Formats = malloc(sizeof(VkSurfaceFormatKHR) * FormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatCount, Formats);
    VkSurfaceFormatKHR SurfaceFormat = Formats[0];
    for (uint32_t i = 0; i < FormatCount; ++i)
    {
        if (Formats[i].format == VK_FORMAT_R8G8B8_SRGB ||
            Formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
            Formats[i].format == VK_FORMAT_B8G8R8_SRGB ||
            Formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
        {
            SurfaceFormat = Formats[i];
            break;
        }
    }
    free(Formats);
    return SurfaceFormat;
}

static VkPresentModeKHR Vulkan_GetPhysicalDeviceSurfacePresentMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
{
    uint32_t PresentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModeCount, 0);
    VkPresentModeKHR *PresentModes = malloc(sizeof(VkPresentModeKHR) * PresentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModeCount, PresentModes);
    VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    // for (u32 i = 0; i < PresentModeCount; ++i)
    // {
    //     if (PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
    //     {
    //         PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    //         break;
    //     }
    //     else if (PresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
    //     {
    //         PresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    //     }
    // }
    free(PresentModes);
    return PresentMode;
}

static bool Vulkan_CheckLayers(uint32_t LayerCount, const char **LayerNames)
{
    uint32_t AvailLayerCount;
    vkEnumerateInstanceLayerProperties(&AvailLayerCount, 0);
    VkLayerProperties *Layers = malloc(sizeof(VkLayerProperties) * AvailLayerCount);
    vkEnumerateInstanceLayerProperties(&AvailLayerCount, Layers);
    
    bool FoundAllLayers = true;
    for (uint32_t i = 0; i < LayerCount; ++i)
    {
        bool FoundLayer = false;
        for (uint32_t j = 0; j < AvailLayerCount; ++j)
        {
            if (!strcmp(Layers[j].layerName, LayerNames[i]))
            {
                FoundLayer = true;
                break;
            }
        }
        if (!FoundLayer)
        {
            FoundAllLayers = false;
            break;
        }
    }
    
    free(Layers);
    return FoundAllLayers;
}

static bool Vulkan_CheckDeviceExtensions(VkPhysicalDevice PhysicalDevice, uint32_t ReqExtensionCount, const char **ReqExtensionNames)
{
    uint32_t ExtensionCount;
    vkEnumerateDeviceExtensionProperties(PhysicalDevice, 0, &ExtensionCount, 0);

    VkExtensionProperties *Extensions = malloc(sizeof(VkExtensionProperties) * ExtensionCount);
    vkEnumerateDeviceExtensionProperties(PhysicalDevice, 0, &ExtensionCount, Extensions);
    
    bool FoundAllExtensions = true;
    for (uint32_t i = 0; i < ReqExtensionCount; ++i)
    {
        bool FoundExtension = false;
        for (uint32_t j = 0; j < ExtensionCount; ++j)
        {
            if (!strcmp(ReqExtensionNames[i], Extensions[j].extensionName))
            {
                FoundExtension = true;
                break;
            }
        }
        if (!FoundExtension)
        {
            FoundAllExtensions = false;
            break;
        }
    }
    
    free(Extensions);
    return FoundAllExtensions;
}

static bool Vulkan_CheckSwapChainSupport(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
{
    if (Surface == VK_NULL_HANDLE)
        return true;

    VkSurfaceCapabilitiesKHR Capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &Capabilities);

    uint32_t FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatCount, 0);
    if (!FormatCount)
        return false;

    uint32_t PresentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModeCount, 0);
    if (!PresentModeCount)
        return false;

    return true;
}
