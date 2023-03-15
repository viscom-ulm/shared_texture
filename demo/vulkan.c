// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#include "vk_funcs.h"
#include "vk_utils.h"

typedef struct vk_state vk_state;
typedef void (*vk_log_func)(const char *);

static VkInstance Vulkan_CreateInstance(uint32_t ExtCount, const char **ExtNames);
static bool Vulkan_InitInstance(vk_state *VK, VkInstance Instance, VkSurfaceKHR Surface, vk_log_func LogFunc);
static void Vulkan_DestroyInstance(vk_state *VK);

static bool Vulkan_Init(vk_state *VK);
static void Vulkan_Destroy(vk_state *VK);
static void Vulkan_Update(vk_state *VK, int32_t Width, int32_t Height, float Delta);

//
//
//

typedef struct vk_swap_chain
{
    uint32_t ImageCount;
    VkImage *Images;
    VkExtent2D Extent;
    VkSwapchainKHR Handle;
} vk_swap_chain;

typedef struct vk_state
{
    vk_swap_chain SwapChain;
    shared_texture SharedTexture;
    vk_shared_texture VkSharedTexture;

    // Instance
    VkInstance Instance;
    VkSurfaceKHR Surface;
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
    VkQueue Queue;
    VkQueue TransferQueue;

    // Shared
    VkCommandPool CommandPool;
    VkCommandBuffer CommandBuffer;
    VkDescriptorPool DescriptorPool;

    // Sync
    VkSemaphore SwapChainSemaphore;
    VkSemaphore RenderSemaphore;
    VkFence FrameFence;

#if defined(_DEBUG)
    vk_log_func LogFunction;
    VkDebugReportCallbackEXT DebugReportCallback;
#endif
} vk_state;

static VkBool32 Vulkan_DebugCallback(VkDebugReportFlagsEXT Flags, VkDebugReportObjectTypeEXT ObjType,
                              uint64_t Obj, size_t Location, int32_t Code, 
                              const char* LayerPrefix, const char* Msg, void* UserData)
{
    vk_state *VK = UserData;
#if defined(_DEBUG)
    VK->LogFunction(Msg);
#endif
    return VK_FALSE;
}

static VkInstance Vulkan_CreateInstance(uint32_t ExtCount, const char **ExtNames)
{
    if (!VK_LoadFunctions())
        return VK_NULL_HANDLE;

#if defined(_DEBUG)
    const char *LayerNames[] = { "VK_LAYER_KHRONOS_validation" };
    const uint32_t LayerCount = sizeof(LayerNames) / sizeof(LayerNames[0]);
#else
    const char **LayerNames = 0;
    const uint32_t LayerCount = 0;
#endif
    if (!Vulkan_CheckLayers(LayerCount, LayerNames))
        return VK_NULL_HANDLE;

    VkInstance Instance = VK_NULL_HANDLE;
    VkResult Result = vkCreateInstance(
        &(VkInstanceCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo =
                &(VkApplicationInfo) {
                    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                    .pApplicationName = "",
                    .applicationVersion = 0,
                    .pEngineName = "",
                    .engineVersion = 0,
                    .apiVersion = VK_API_VERSION_1_3,
                },
            .enabledLayerCount = LayerCount,
            .ppEnabledLayerNames = LayerNames,
            .enabledExtensionCount = ExtCount,
            .ppEnabledExtensionNames = ExtNames
        },
        0, &Instance
    );
    if (Result != VK_SUCCESS)
        return VK_NULL_HANDLE;

    if (!VK_LoadInstanceFunctions(Instance))
        return VK_NULL_HANDLE;
    
    return Instance;
}

static void Vulkan_DestroyInstance(vk_state *VK)
{
#if defined(_DEBUG)
    vkDestroyDebugReportCallbackEXT(VK->Instance, VK->DebugReportCallback, 0);
#endif

    vkDestroyDevice(VK->Device, 0);
    vkDestroySurfaceKHR(VK->Instance, VK->Surface, 0);
    vkDestroyInstance(VK->Instance, 0);

    VK_DeleteFunctions();
    *VK = (vk_state){ 0 };
}

static bool Vulkan_InitInstance(vk_state *VK, VkInstance Instance, VkSurfaceKHR Surface, vk_log_func LogFunction)
{
    VkResult Result;

    *VK = (vk_state){ 0 };
    VK->Instance = Instance;
#if defined(_DEBUG)
    VK->LogFunction = LogFunction;
#endif

#if defined(_DEBUG)
    Result = vkCreateDebugReportCallbackEXT(VK->Instance, 
        &(VkDebugReportCallbackCreateInfoEXT) {
            .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
            .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
            .pfnCallback = Vulkan_DebugCallback,
            .pUserData = VK
        },
        0, &VK->DebugReportCallback
    );
    if (Result != VK_SUCCESS)
        return false;
#endif

    VK->Surface = Surface;

    const char* ExtNames[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(_WIN32)
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME
#else
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME
#endif
    };
    const uint32_t ExtCount = sizeof(ExtNames) / sizeof(ExtNames[0]);

    // PHYSICAL DEVICE
    VK->PhysicalDevice = Vulkan_FindPhysicalDevice(VK->Instance, VK->Surface, ExtCount, ExtNames);
    if (VK->PhysicalDevice == VK_NULL_HANDLE) return false;

    // DEVICE
    uint32_t DefaultQueueIndex = Vulkan_DefaultQueueFamilyIndex(VK->PhysicalDevice, VK->Surface);
    uint32_t TransferQueueIndex = Vulkan_TransferQueueFamilyIndex(VK->PhysicalDevice);
    Result = vkCreateDevice(VK->PhysicalDevice,
        &(VkDeviceCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pQueueCreateInfos = (VkDeviceQueueCreateInfo[]) {
                    [0] = {
                        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                        .queueFamilyIndex = DefaultQueueIndex,
                        .queueCount = 1,
                        .pQueuePriorities = (float[]){ 1.0f }
                    },
                    [1] = {
                        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                        .queueFamilyIndex = TransferQueueIndex,
                        .queueCount = 1,
                        .pQueuePriorities = (float[]){ 1.0f }
                    }
                },
            .queueCreateInfoCount = 2,
            .enabledExtensionCount = ExtCount,
            .ppEnabledExtensionNames = ExtNames,
            .pEnabledFeatures = &(VkPhysicalDeviceFeatures){
                .samplerAnisotropy = VK_TRUE
            },
        },
        0, &VK->Device
    );
    if (Result != VK_SUCCESS)
        return false;

    vkGetDeviceQueue(VK->Device, DefaultQueueIndex, 0, &VK->Queue);
    vkGetDeviceQueue(VK->Device, TransferQueueIndex, 0, &VK->TransferQueue);

    return true;
}

static bool Vulkan_CreateSwapChain(vk_state *VK)
{
    VkSurfaceCapabilitiesKHR Capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VK->PhysicalDevice, VK->Surface, &Capabilities);

    if (Capabilities.currentExtent.width == 0 ||
        Capabilities.currentExtent.height == 0)
        return false;

    VK->SwapChain.Extent = Capabilities.currentExtent;
    if (Capabilities.currentExtent.width == 0xFFFFFFFF)
    {
        VK->SwapChain.Extent.width = 1280;
        if (VK->SwapChain.Extent.width < Capabilities.minImageExtent.width)
            VK->SwapChain.Extent.width = Capabilities.minImageExtent.width;
        if (VK->SwapChain.Extent.width > Capabilities.maxImageExtent.width)
            VK->SwapChain.Extent.width = Capabilities.maxImageExtent.width;
    }
    if (Capabilities.currentExtent.height == 0xFFFFFFFF)
    {
        VK->SwapChain.Extent.height = 720;
        if (VK->SwapChain.Extent.height < Capabilities.minImageExtent.height)
            VK->SwapChain.Extent.height = Capabilities.minImageExtent.height;
        if (VK->SwapChain.Extent.height > Capabilities.maxImageExtent.height)
            VK->SwapChain.Extent.height = Capabilities.maxImageExtent.height;
    }

    uint32_t ImageCount = Capabilities.minImageCount + 1;
    if (Capabilities.maxImageCount != 0 && ImageCount > Capabilities.maxImageCount)
        ImageCount = Capabilities.maxImageCount;

    VkSurfaceFormatKHR SurfaceFormat = Vulkan_GetPhysicalDeviceSurfaceFormat(VK->PhysicalDevice, VK->Surface);
    VkPresentModeKHR PresentMode = Vulkan_GetPhysicalDeviceSurfacePresentMode(VK->PhysicalDevice, VK->Surface);
    VkResult Result = vkCreateSwapchainKHR(VK->Device,
        &(VkSwapchainCreateInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = VK->Surface,
            .minImageCount = ImageCount,
            .imageFormat = SurfaceFormat.format,
            .imageColorSpace = SurfaceFormat.colorSpace,
            .imageExtent = VK->SwapChain.Extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = 0,
            .preTransform = Capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = PresentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        }, 
        0, &VK->SwapChain.Handle
    );
    if (Result != VK_SUCCESS)
        return false;

    vkGetSwapchainImagesKHR(VK->Device, VK->SwapChain.Handle, &VK->SwapChain.ImageCount, 0);
    VK->SwapChain.Images = malloc(sizeof(VkImage) * VK->SwapChain.ImageCount);
    vkGetSwapchainImagesKHR(VK->Device, VK->SwapChain.Handle, &VK->SwapChain.ImageCount, VK->SwapChain.Images);
    
    return true;
}

static void Vulkan_DestroySwapChain(vk_state *VK)
{
    vkDestroySwapchainKHR(VK->Device, VK->SwapChain.Handle, 0);
    free(VK->SwapChain.Images);
    VK->SwapChain = (vk_swap_chain){ 0 };
}

static bool Vulkan_InitCommandBuffers(vk_state *VK)
{
    VkResult Result = vkCreateCommandPool(VK->Device, 
        &(VkCommandPoolCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = Vulkan_DefaultQueueFamilyIndex(VK->PhysicalDevice, VK->Surface)
        },
        0, &VK->CommandPool
    );
    if (Result != VK_SUCCESS)
        return false;
    
    Result = vkAllocateCommandBuffers(VK->Device,
        &(VkCommandBufferAllocateInfo) {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = VK->CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        },
        &VK->CommandBuffer
    );
    if (Result != VK_SUCCESS)
        return false;

    return true;
}

static void Vulkan_DestroyCommandBuffers(vk_state *VK)
{
    vkDestroyCommandPool(VK->Device, VK->CommandPool, 0);
}

static bool Vulkan_CreateSyncObjects(vk_state *VK)
{
    VkResult Result = vkCreateSemaphore(VK->Device,
        &(VkSemaphoreCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        },
        0, &VK->SwapChainSemaphore
    );
    if (Result != VK_SUCCESS)
        return false;

    Result = vkCreateSemaphore(VK->Device,
        &(VkSemaphoreCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        },
        0, &VK->RenderSemaphore
    );
    if (Result != VK_SUCCESS)
        return false;

    Result = vkCreateFence(VK->Device,
        &(VkFenceCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        },
        0, &VK->FrameFence
    );
    if (Result != VK_SUCCESS)
        return false;

    return true;
}

static void Vulkan_DestroySyncObjects(vk_state *VK)
{
    vkDestroyFence(VK->Device, VK->FrameFence, 0);
    vkDestroySemaphore(VK->Device, VK->RenderSemaphore, 0);
    vkDestroySemaphore(VK->Device, VK->SwapChainSemaphore, 0);
}

static bool Vulkan_Init(vk_state *VK)
{
    if (!Vulkan_InitCommandBuffers(VK) ||
        !Vulkan_CreateSyncObjects(VK))
    {
        Vulkan_Destroy(VK);
        return false;
    }

    VK->SharedTexture = SharedTexture_OpenOrCreate("demo", 1280, 720, SHARED_TEXTURE_RGBA8);
    VK->VkSharedTexture = SharedTexture_ToVulkan(VK->SharedTexture, VK->Device, VK->PhysicalDevice);

    return true;
}

static void Vulkan_Destroy(vk_state *VK)
{
    vkDeviceWaitIdle(VK->Device);

    SharedTexture_DestroyVulkanTexture(VK->VkSharedTexture, VK->Device);
    SharedTexture_Close(VK->SharedTexture);

    Vulkan_DestroyCommandBuffers(VK);
    Vulkan_DestroySyncObjects(VK);
    Vulkan_DestroySwapChain(VK);
}

static void Vulkan_Update(vk_state *VK, int32_t Width, int32_t Height, float Delta)
{
    vkWaitForFences(VK->Device, 1, &VK->FrameFence, VK_TRUE, UINT64_MAX);

    if (VK->SwapChain.Handle == VK_NULL_HANDLE)
        if (!Vulkan_CreateSwapChain(VK))
            return;

    uint32_t CurrentSwapChainImage;
    if (vkAcquireNextImageKHR(VK->Device, VK->SwapChain.Handle, UINT64_MAX, VK->SwapChainSemaphore,
                              VK_NULL_HANDLE, &CurrentSwapChainImage) == VK_ERROR_OUT_OF_DATE_KHR)
    {
        vkDeviceWaitIdle(VK->Device);
        Vulkan_DestroySwapChain(VK);
        return;
    }

    vkResetCommandBuffer(VK->CommandBuffer, 0);
    vkBeginCommandBuffer(VK->CommandBuffer,
        &(VkCommandBufferBeginInfo) {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0,
            .pInheritanceInfo = 0,
        }
    );

    // BLIT
    vkCmdPipelineBarrier(VK->CommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, (VkMemoryBarrier[]) { 0 },
        0, (VkBufferMemoryBarrier[]) { 0 },
        1, (VkImageMemoryBarrier[]) {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .image = VK->VkSharedTexture.Image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            }
        }
    );

    vkCmdPipelineBarrier(VK->CommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, (VkMemoryBarrier[]) { 0 },
        0, (VkBufferMemoryBarrier[]) { 0 },
        1, (VkImageMemoryBarrier[]) {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .image = VK->SwapChain.Images[CurrentSwapChainImage],
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            }
        }
    );

    vkCmdBlitImage(VK->CommandBuffer,
        VK->VkSharedTexture.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK->SwapChain.Images[CurrentSwapChainImage], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, (VkImageBlit[]) {
            {
                .srcSubresource = (VkImageSubresourceLayers) {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .srcOffsets = {
                    [0] = {
                        .x = 0,
                        .y = VK->SharedTexture.Height,
                        .z = 0
                    },
                    [1] = {
                        .x = VK->SharedTexture.Width,
                        .y = 0,
                        .z = 1
                    }
                },
                .dstSubresource = (VkImageSubresourceLayers) {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .dstOffsets = {
                    [1] = {
                        .x = VK->SwapChain.Extent.width,
                        .y = VK->SwapChain.Extent.height,
                        .z = 1,
                    }
                }
            }
        },
        VK_FILTER_NEAREST);

    vkCmdPipelineBarrier(VK->CommandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, (VkMemoryBarrier[]) { 0 },
        0, (VkBufferMemoryBarrier[]) { 0 },
        1, (VkImageMemoryBarrier[]) {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_NONE,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .image = VK->VkSharedTexture.Image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            }
        }
    );

    vkCmdPipelineBarrier(VK->CommandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, (VkMemoryBarrier[]) { 0 },
        0, (VkBufferMemoryBarrier[]) { 0 },
        1, (VkImageMemoryBarrier[]) {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_NONE,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .image = VK->SwapChain.Images[CurrentSwapChainImage],
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            }
        }
    );

    vkEndCommandBuffer(VK->CommandBuffer);

    vkResetFences(VK->Device, 1, &VK->FrameFence);

    VkResult SubmitResult = vkQueueSubmit(VK->Queue,
        1, &(VkSubmitInfo) {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 2,
            .pWaitSemaphores = (VkSemaphore[]){ VK->SwapChainSemaphore, VK->VkSharedTexture.Semaphore },
            .pWaitDstStageMask = (VkPipelineStageFlags[]){ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT },
            .commandBufferCount = 1,
            .pCommandBuffers = (VkCommandBuffer[]){ VK->CommandBuffer },
            .signalSemaphoreCount = 2,
            .pSignalSemaphores = (VkSemaphore[]){ VK->RenderSemaphore, VK->VkSharedTexture.Semaphore },
        },
        VK->FrameFence
    );
    if (SubmitResult != VK_SUCCESS)
        return;
    
    VkResult PresentResult = vkQueuePresentKHR(VK->Queue,
        &(VkPresentInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = (VkSemaphore[]){ VK->RenderSemaphore },
            .swapchainCount = 1,
            .pSwapchains = (VkSwapchainKHR[]){ VK->SwapChain.Handle },
            .pImageIndices = (uint32_t[]){ CurrentSwapChainImage },
            .pResults = 0
        }
    );
    if (PresentResult == VK_ERROR_OUT_OF_DATE_KHR || PresentResult == VK_SUBOPTIMAL_KHR)
    {
        vkDeviceWaitIdle(VK->Device);
        Vulkan_DestroySwapChain(VK);
    }
}
