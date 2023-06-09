// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#define VK_NO_PROTOTYPES
#include <Unity/IUnityGraphics.h>
#include <Unity/IUnityGraphicsVulkan.h>

#include "unity.h"

static IUnityInterfaces* UnityInterfaces = NULL;
static IUnityGraphics* UnityGraphics = NULL;
static IUnityGraphicsVulkanV2 *UnityVulkan = NULL;

static uint32_t GlobalSharedTextureCount = 0;
static vk_shared_texture **GlobalSharedTextures = NULL;

static VkResult UnityHook_VkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence)
{
    VkSubmitInfo *SubmitInfos = malloc(sizeof(VkSubmitInfo) * submitCount);
    for (uint32_t i = 0; i < submitCount; ++i)
    {
        SubmitInfos[i] = pSubmits[i];
        SubmitInfos[i].waitSemaphoreCount = pSubmits[i].waitSemaphoreCount + GlobalSharedTextureCount;
        SubmitInfos[i].signalSemaphoreCount = pSubmits[i].signalSemaphoreCount + GlobalSharedTextureCount;

        VkSemaphore *WaitSemaphores = malloc(sizeof(VkSemaphore) * SubmitInfos[i].waitSemaphoreCount);
        VkPipelineStageFlags *WaitDstStageMask = malloc(sizeof(VkPipelineStageFlags) * SubmitInfos[i].waitSemaphoreCount);
        VkSemaphore *SignalSemaphores = malloc(sizeof(VkSemaphore) * SubmitInfos[i].signalSemaphoreCount);

        memcpy(WaitSemaphores, pSubmits[i].pWaitSemaphores, sizeof(VkSemaphore) * pSubmits[i].waitSemaphoreCount);
        memcpy(WaitDstStageMask, pSubmits[i].pWaitDstStageMask, sizeof(VkPipelineStageFlags) * pSubmits[i].waitSemaphoreCount);
        memcpy(SignalSemaphores, pSubmits[i].pSignalSemaphores, sizeof(VkSemaphore) * pSubmits[i].signalSemaphoreCount);

        for (uint32_t j = 0; j < GlobalSharedTextureCount; ++j)
        {
            WaitSemaphores[pSubmits[i].waitSemaphoreCount + j] = GlobalSharedTextures[j]->Semaphore;
            WaitDstStageMask[pSubmits[i].waitSemaphoreCount + j] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            SignalSemaphores[pSubmits[i].signalSemaphoreCount + j] = GlobalSharedTextures[j]->Semaphore;
        }

        SubmitInfos[i].pWaitSemaphores = WaitSemaphores;
        SubmitInfos[i].pWaitDstStageMask = WaitDstStageMask;
        SubmitInfos[i].pSignalSemaphores = SignalSemaphores;
    }

    VkResult Result = vkQueueSubmit(queue, submitCount, SubmitInfos, fence);

    for (uint32_t i = 0; i < submitCount; ++i)
    {
        free((void *)SubmitInfos[i].pWaitSemaphores);
        free((void *)SubmitInfos[i].pWaitDstStageMask);
        free((void *)SubmitInfos[i].pSignalSemaphores);
    }

    free(SubmitInfos);

    return Result;
}

static VkResult UnityHook_vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance)
{
    const uint32_t AdditionalExtCount = 3;
    const char *AdditionalExtNames[] = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
    };

    const uint32_t AllExtCount = pCreateInfo->enabledExtensionCount + AdditionalExtCount;
    const char **AllExtNames = malloc(sizeof(char *) * AllExtCount);
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i)
        AllExtNames[i] = pCreateInfo->ppEnabledExtensionNames[i];
    for (uint32_t i = 0; i < AdditionalExtCount; ++i)
        AllExtNames[pCreateInfo->enabledExtensionCount + i] = AdditionalExtNames[i];

    VkInstanceCreateInfo CreateInfo = *pCreateInfo;
    CreateInfo.enabledExtensionCount = AllExtCount;
    CreateInfo.ppEnabledExtensionNames = AllExtNames;
    VkResult Result = vkCreateInstance(pCreateInfo, pAllocator, pInstance);
    VK_LoadInstanceFunctions(*pInstance);
    free((void *)AllExtNames);
    return Result;
}

static VkResult UnityHook_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice)
{
    const uint32_t AdditionalExtCount = 4;
    const char *AdditionalExtNames[] = {
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
#if defined(_WIN32)
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
#else
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
#endif
    };
    const uint32_t AllExtCount = pCreateInfo->enabledExtensionCount + AdditionalExtCount;
    const char **AllExtNames = malloc(sizeof(char *) * AllExtCount);
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i)
        AllExtNames[i] = pCreateInfo->ppEnabledExtensionNames[i];
    for (uint32_t i = 0; i < AdditionalExtCount; ++i)
        AllExtNames[pCreateInfo->enabledExtensionCount + i] = AdditionalExtNames[i];

    VkDeviceCreateInfo CreateInfo = *pCreateInfo;
    CreateInfo.enabledExtensionCount = AllExtCount;
    CreateInfo.ppEnabledExtensionNames = AllExtNames;
    VkResult Result = vkCreateDevice(physicalDevice, &CreateInfo, pAllocator, pDevice);
    free((void *)AllExtNames);
    return Result;
}

static PFN_vkVoidFunction UnityHook_getInstanceProcAddr(VkInstance instance, const char *pName)
{
    if (!strcmp("vkCreateInstance", pName))
        return (PFN_vkVoidFunction)UnityHook_vkCreateInstance;
    if (!strcmp("vkCreateDevice", pName))
        return (PFN_vkVoidFunction)UnityHook_vkCreateDevice;
    if (!strcmp("vkQueueSubmit", pName))
        return (PFN_vkVoidFunction)UnityHook_VkQueueSubmit;
    return vkGetInstanceProcAddr(instance, pName);
}

static PFN_vkGetInstanceProcAddr UNITY_INTERFACE_API Unity_VulkanInitCallback(PFN_vkGetInstanceProcAddr getInstanceProcAddr, void* userdata)
{
    vkGetInstanceProcAddr = getInstanceProcAddr;
    VK_LoadFunctions();
    return UnityHook_getInstanceProcAddr;
}

static void UNITY_INTERFACE_API Unity_OnGraphicsDeviceEvent(UnityGfxDeviceEventType EventType)
{
    if (EventType == kUnityGfxDeviceEventInitialize)
    {
    }

    if (EventType == kUnityGfxDeviceEventShutdown)
    {
        if (GlobalSharedTextureCount)
        {
            UnityVulkanInstance Instance = UnityVulkan->Instance();

            for (uint32_t i = 0; i < GlobalSharedTextureCount; ++i)
            {
                SharedTexture_DestroyVulkanTexture(*GlobalSharedTextures[i], Instance.device);
                free(GlobalSharedTextures[i]);
            }
            free(GlobalSharedTextures);
            GlobalSharedTextureCount = 0;
            GlobalSharedTextures = NULL;
        }
    }
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* Interfaces)
{
    SharedTexture_Init();

    UnityInterfaces = Interfaces;
    UnityGraphics = UNITY_GET_INTERFACE(UnityInterfaces, IUnityGraphics);
    UnityGraphics->RegisterDeviceEventCallback(Unity_OnGraphicsDeviceEvent);
    UnityVulkan = UNITY_GET_INTERFACE(UnityInterfaces, IUnityGraphicsVulkanV2);
    bool Success = UnityVulkan->InterceptInitialization(Unity_VulkanInitCallback, 0);

    if (UnityGraphics->GetRenderer() == kUnityGfxRendererVulkan)
    {
    }
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    UnityGraphics->UnregisterDeviceEventCallback(Unity_OnGraphicsDeviceEvent);
}

unity_shared_texture UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API CreateSharedTexture(const char *Name, int32_t Width, int32_t Height, uint32_t Format)
{
    UnityVulkanInstance Instance = UnityVulkan->Instance();
    shared_texture SharedTexture = SharedTexture_OpenOrCreate(Name, Width, Height, Format);
    GlobalSharedTextures = realloc(GlobalSharedTextures, sizeof(vk_shared_texture*) * (++GlobalSharedTextureCount));
    vk_shared_texture *VkSharedTexture = malloc(sizeof(vk_shared_texture));
    *VkSharedTexture = SharedTexture_ToVulkan(SharedTexture, Instance.device, Instance.physicalDevice);
    GlobalSharedTextures[GlobalSharedTextureCount-1] = VkSharedTexture;

    return (unity_shared_texture) {
        .NativeTex = &VkSharedTexture->Image,
        .Format = SharedTexture.Format,
        .Width = SharedTexture.Width,
        .Height = SharedTexture.Height,
    };
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API DestroySharedTexture(unity_shared_texture SharedTexture)
{
    UnityVulkanInstance Instance = UnityVulkan->Instance();
    
    for (uint32_t i = 0; i < GlobalSharedTextureCount; ++i)
    {
        if (GlobalSharedTextures[i]->Image != SharedTexture.NativeTex)
            continue;

        SharedTexture_DestroyVulkanTexture(*GlobalSharedTextures[i], Instance.device);
        free(GlobalSharedTextures[i]);

        if (--GlobalSharedTextureCount)
        {
            for (uint32_t j = i; j < GlobalSharedTextureCount; ++j)
                GlobalSharedTextures[j] = GlobalSharedTextures[j + 1];
            GlobalSharedTextures = realloc(GlobalSharedTextures, sizeof(vk_shared_texture*) * (GlobalSharedTextureCount));
        }
        else
        {
            free(GlobalSharedTextures);
            GlobalSharedTextures = NULL;
        }
        break;
    }
}
