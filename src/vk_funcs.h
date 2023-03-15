// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#if defined(_WIN32)
  #include <vulkan/vulkan_win32.h>
#endif

static bool VK_LoadFunctions(void);
static bool VK_LoadInstanceFunctions(VkInstance Instance);
static void VK_DeleteFunctions(void);

//
//
//

#define VK_FUNC(Name) PFN_##Name Name

/* 1.0 */
VK_FUNC(vkGetInstanceProcAddr);
VK_FUNC(vkCreateInstance);
VK_FUNC(vkEnumerateInstanceExtensionProperties);
VK_FUNC(vkEnumerateInstanceLayerProperties);
VK_FUNC(vkDestroyInstance);
VK_FUNC(vkEnumeratePhysicalDevices);
VK_FUNC(vkGetPhysicalDeviceProperties);
VK_FUNC(vkGetPhysicalDeviceFeatures);
VK_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
VK_FUNC(vkGetPhysicalDeviceMemoryProperties);
VK_FUNC(vkGetPhysicalDeviceFormatProperties);
VK_FUNC(vkCreateDevice);
VK_FUNC(vkDestroyDevice);
VK_FUNC(vkGetDeviceQueue);
VK_FUNC(vkEnumerateDeviceExtensionProperties);
VK_FUNC(vkCreateImage);
VK_FUNC(vkDestroyImage);
VK_FUNC(vkCreateImageView);
VK_FUNC(vkDestroyImageView);
VK_FUNC(vkCreateShaderModule);
VK_FUNC(vkDestroyShaderModule);
VK_FUNC(vkCreatePipelineLayout);
VK_FUNC(vkDestroyPipelineLayout);
VK_FUNC(vkCreateRenderPass);
VK_FUNC(vkDestroyRenderPass);
VK_FUNC(vkCreateGraphicsPipelines);
VK_FUNC(vkDestroyPipeline);
VK_FUNC(vkCreateFramebuffer);
VK_FUNC(vkDestroyFramebuffer);
VK_FUNC(vkDestroyCommandPool);
VK_FUNC(vkCreateCommandPool);
VK_FUNC(vkAllocateCommandBuffers);
VK_FUNC(vkFreeCommandBuffers);
VK_FUNC(vkBeginCommandBuffer);
VK_FUNC(vkEndCommandBuffer);
VK_FUNC(vkResetCommandBuffer);
VK_FUNC(vkCmdExecuteCommands);
VK_FUNC(vkCmdBeginRenderPass);
VK_FUNC(vkCmdBindPipeline);
VK_FUNC(vkCmdBindVertexBuffers);
VK_FUNC(vkCmdDraw);
VK_FUNC(vkCmdEndRenderPass);
VK_FUNC(vkCmdBlitImage);
VK_FUNC(vkCmdResolveImage);
VK_FUNC(vkCmdCopyBuffer);
VK_FUNC(vkCmdCopyBufferToImage);
VK_FUNC(vkCmdPipelineBarrier);
VK_FUNC(vkCmdBindDescriptorSets);
VK_FUNC(vkCmdPushConstants);
VK_FUNC(vkCmdNextSubpass);
VK_FUNC(vkCmdSetViewport);
VK_FUNC(vkCmdSetScissor);
VK_FUNC(vkCreateSemaphore);
VK_FUNC(vkDestroySemaphore);
VK_FUNC(vkCreateFence);
VK_FUNC(vkDestroyFence);
VK_FUNC(vkWaitForFences);
VK_FUNC(vkResetFences);
VK_FUNC(vkQueueSubmit);
VK_FUNC(vkQueueWaitIdle);
VK_FUNC(vkQueuePresentKHR);
VK_FUNC(vkDeviceWaitIdle);
VK_FUNC(vkGetImageMemoryRequirements);
VK_FUNC(vkGetBufferMemoryRequirements);
VK_FUNC(vkGetDeviceImageMemoryRequirements);
VK_FUNC(vkGetDeviceBufferMemoryRequirements);
VK_FUNC(vkAllocateMemory);
VK_FUNC(vkBindImageMemory);
VK_FUNC(vkBindBufferMemory);
VK_FUNC(vkFreeMemory);
VK_FUNC(vkCreateBuffer);
VK_FUNC(vkDestroyBuffer);
VK_FUNC(vkMapMemory);
VK_FUNC(vkUnmapMemory);
VK_FUNC(vkCreateDescriptorSetLayout);
VK_FUNC(vkDestroyDescriptorSetLayout);
VK_FUNC(vkCreateDescriptorPool);
VK_FUNC(vkDestroyDescriptorPool);
VK_FUNC(vkAllocateDescriptorSets);
VK_FUNC(vkFreeDescriptorSets);
VK_FUNC(vkUpdateDescriptorSets);
VK_FUNC(vkCreateSampler);
VK_FUNC(vkDestroySampler);

#if defined(_DEBUG)
	VK_FUNC(vkCreateDebugReportCallbackEXT);
	VK_FUNC(vkDestroyDebugReportCallbackEXT);
#endif

/* VK_KHR_surface */
VK_FUNC(vkDestroySurfaceKHR);
VK_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
VK_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
VK_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
VK_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);

/* VK_KHR_swapchain */
VK_FUNC(vkAcquireNextImageKHR);
VK_FUNC(vkCreateSwapchainKHR);
VK_FUNC(vkDestroySwapchainKHR);
VK_FUNC(vkGetSwapchainImagesKHR);

#if defined(_WIN32)
	/* VK_KHR_external_memory_win32  */
	VK_FUNC(vkGetMemoryWin32HandleKHR);
	/* VK_KHR_external_semaphore_win32 */
	VK_FUNC(vkGetSemaphoreWin32HandleKHR);
	VK_FUNC(vkImportSemaphoreWin32HandleKHR);
#else
	/* VK_KHR_external_memory_fd */
	VK_FUNC(vkGetMemoryFdKHR);
	/* VK_KHR_external_semaphore_fd */
	VK_FUNC(vkGetSemaphoreFdKHR);
#endif

#define VK_LOAD_FUNC(I, Name) Name = (PFN_##Name)vkGetInstanceProcAddr(I, #Name)
#define VK_LOAD_AND_CHECK(I, Name) Name = (PFN_##Name)vkGetInstanceProcAddr(I, #Name); if(!Name) return false;

static bool VK_LoadFunctions(void)
{
	VK_LOAD_AND_CHECK(0, vkCreateInstance);
	VK_LOAD_AND_CHECK(0, vkEnumerateInstanceExtensionProperties);
	VK_LOAD_AND_CHECK(0, vkEnumerateInstanceLayerProperties);

	return true;
}

static bool VK_LoadInstanceFunctions(VkInstance Instance)
{
	/* 1.0 */
	VK_LOAD_AND_CHECK(Instance, vkDestroyInstance);
	VK_LOAD_AND_CHECK(Instance, vkEnumeratePhysicalDevices);
	VK_LOAD_AND_CHECK(Instance, vkGetPhysicalDeviceProperties);
	VK_LOAD_AND_CHECK(Instance, vkGetPhysicalDeviceFeatures);
	VK_LOAD_AND_CHECK(Instance, vkGetPhysicalDeviceQueueFamilyProperties);
	VK_LOAD_AND_CHECK(Instance, vkGetPhysicalDeviceMemoryProperties);
	VK_LOAD_AND_CHECK(Instance, vkGetPhysicalDeviceFormatProperties);
	VK_LOAD_AND_CHECK(Instance, vkCreateDevice);
	VK_LOAD_AND_CHECK(Instance, vkDestroyDevice);
	VK_LOAD_AND_CHECK(Instance, vkGetDeviceQueue);
	VK_LOAD_AND_CHECK(Instance, vkEnumerateDeviceExtensionProperties);
	VK_LOAD_AND_CHECK(Instance, vkCreateImage);
	VK_LOAD_AND_CHECK(Instance, vkDestroyImage);
	VK_LOAD_AND_CHECK(Instance, vkCreateImageView);
	VK_LOAD_AND_CHECK(Instance, vkDestroyImageView);
	VK_LOAD_AND_CHECK(Instance, vkCreateShaderModule);
	VK_LOAD_AND_CHECK(Instance, vkDestroyShaderModule);
	VK_LOAD_AND_CHECK(Instance, vkCreatePipelineLayout);
	VK_LOAD_AND_CHECK(Instance, vkDestroyPipelineLayout);
	VK_LOAD_AND_CHECK(Instance, vkCreateRenderPass);
	VK_LOAD_AND_CHECK(Instance, vkDestroyRenderPass);
	VK_LOAD_AND_CHECK(Instance, vkCreateGraphicsPipelines);
	VK_LOAD_AND_CHECK(Instance, vkDestroyPipeline);
	VK_LOAD_AND_CHECK(Instance, vkCreateFramebuffer);
	VK_LOAD_AND_CHECK(Instance, vkDestroyFramebuffer);
	VK_LOAD_AND_CHECK(Instance, vkDestroyCommandPool);
	VK_LOAD_AND_CHECK(Instance, vkCreateCommandPool);
	VK_LOAD_AND_CHECK(Instance, vkAllocateCommandBuffers);
	VK_LOAD_AND_CHECK(Instance, vkFreeCommandBuffers);
	VK_LOAD_AND_CHECK(Instance, vkBeginCommandBuffer);
	VK_LOAD_AND_CHECK(Instance, vkEndCommandBuffer);
	VK_LOAD_AND_CHECK(Instance, vkResetCommandBuffer);
	VK_LOAD_AND_CHECK(Instance, vkCmdExecuteCommands);
	VK_LOAD_AND_CHECK(Instance, vkCmdBeginRenderPass);
	VK_LOAD_AND_CHECK(Instance, vkCmdBindPipeline);
	VK_LOAD_AND_CHECK(Instance, vkCmdBindVertexBuffers);
	VK_LOAD_AND_CHECK(Instance, vkCmdDraw);
	VK_LOAD_AND_CHECK(Instance, vkCmdEndRenderPass);
	VK_LOAD_AND_CHECK(Instance, vkCmdBlitImage);
	VK_LOAD_AND_CHECK(Instance, vkCmdResolveImage);
	VK_LOAD_AND_CHECK(Instance, vkCmdCopyBuffer);
	VK_LOAD_AND_CHECK(Instance, vkCmdCopyBufferToImage);
	VK_LOAD_AND_CHECK(Instance, vkCmdPipelineBarrier);
	VK_LOAD_AND_CHECK(Instance, vkCmdBindDescriptorSets);
	VK_LOAD_AND_CHECK(Instance, vkCmdPushConstants);
	VK_LOAD_AND_CHECK(Instance, vkCmdNextSubpass);
	VK_LOAD_AND_CHECK(Instance, vkCmdSetViewport);
	VK_LOAD_AND_CHECK(Instance, vkCmdSetScissor);
	VK_LOAD_AND_CHECK(Instance, vkCreateSemaphore);
	VK_LOAD_AND_CHECK(Instance, vkDestroySemaphore);
	VK_LOAD_AND_CHECK(Instance, vkCreateFence);
	VK_LOAD_AND_CHECK(Instance, vkDestroyFence);
	VK_LOAD_AND_CHECK(Instance, vkWaitForFences);
	VK_LOAD_AND_CHECK(Instance, vkResetFences);
	VK_LOAD_AND_CHECK(Instance, vkQueueSubmit);
	VK_LOAD_AND_CHECK(Instance, vkQueueWaitIdle);
	VK_LOAD_AND_CHECK(Instance, vkQueuePresentKHR);
	VK_LOAD_AND_CHECK(Instance, vkDeviceWaitIdle);
	VK_LOAD_AND_CHECK(Instance, vkGetImageMemoryRequirements);
	VK_LOAD_AND_CHECK(Instance, vkGetBufferMemoryRequirements);
	VK_LOAD_AND_CHECK(Instance, vkGetDeviceImageMemoryRequirements);
	VK_LOAD_AND_CHECK(Instance, vkGetDeviceBufferMemoryRequirements);
	VK_LOAD_AND_CHECK(Instance, vkAllocateMemory);
	VK_LOAD_AND_CHECK(Instance, vkBindImageMemory);
	VK_LOAD_AND_CHECK(Instance, vkBindBufferMemory);
	VK_LOAD_AND_CHECK(Instance, vkFreeMemory);
	VK_LOAD_AND_CHECK(Instance, vkCreateBuffer);
	VK_LOAD_AND_CHECK(Instance, vkDestroyBuffer);
	VK_LOAD_AND_CHECK(Instance, vkMapMemory);
	VK_LOAD_AND_CHECK(Instance, vkUnmapMemory);
	VK_LOAD_AND_CHECK(Instance, vkCreateDescriptorSetLayout);
	VK_LOAD_AND_CHECK(Instance, vkDestroyDescriptorSetLayout);
	VK_LOAD_AND_CHECK(Instance, vkCreateDescriptorPool);
	VK_LOAD_AND_CHECK(Instance, vkDestroyDescriptorPool);
	VK_LOAD_AND_CHECK(Instance, vkAllocateDescriptorSets);
	VK_LOAD_AND_CHECK(Instance, vkFreeDescriptorSets);
	VK_LOAD_AND_CHECK(Instance, vkUpdateDescriptorSets);
	VK_LOAD_AND_CHECK(Instance, vkCreateSampler);
	VK_LOAD_AND_CHECK(Instance, vkDestroySampler);

#if defined(_DEBUG)
	VK_LOAD_FUNC(Instance, vkCreateDebugReportCallbackEXT);
	VK_LOAD_FUNC(Instance, vkDestroyDebugReportCallbackEXT);
#endif

/* VK_KHR_surface */
VK_LOAD_FUNC(Instance, vkDestroySurfaceKHR);
VK_LOAD_FUNC(Instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
VK_LOAD_FUNC(Instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
VK_LOAD_FUNC(Instance, vkGetPhysicalDeviceSurfacePresentModesKHR);
VK_LOAD_FUNC(Instance, vkGetPhysicalDeviceSurfaceSupportKHR);

/* VK_KHR_swapchain */
VK_LOAD_FUNC(Instance, vkAcquireNextImageKHR);
VK_LOAD_FUNC(Instance, vkCreateSwapchainKHR);
VK_LOAD_FUNC(Instance, vkDestroySwapchainKHR);
VK_LOAD_FUNC(Instance, vkGetSwapchainImagesKHR);

#if defined(_WIN32)
	/* VK_KHR_external_memory_win32  */
	VK_LOAD_FUNC(Instance, vkGetMemoryWin32HandleKHR);
	/* VK_KHR_external_semaphore_win32 */
	VK_LOAD_FUNC(Instance, vkGetSemaphoreWin32HandleKHR);
	VK_LOAD_FUNC(Instance, vkImportSemaphoreWin32HandleKHR);
#else
	/* VK_KHR_external_memory_fd */
	VK_LOAD_FUNC(Instance, vkGetMemoryFdKHR);
	/* VK_KHR_external_semaphore_fd */
	VK_LOAD_FUNC(Instance, vkGetSemaphoreFdKHR);
#endif

	return true;
}

static void VK_DeleteFunctions(void)
{
	/* 1.0 */
	vkDestroyInstance = 0;
	vkEnumeratePhysicalDevices = 0;
	vkGetPhysicalDeviceProperties = 0;
	vkGetPhysicalDeviceFeatures = 0;
	vkGetPhysicalDeviceQueueFamilyProperties = 0;
	vkGetPhysicalDeviceMemoryProperties = 0;
	vkGetPhysicalDeviceFormatProperties = 0;
	vkCreateDevice = 0;
	vkDestroyDevice = 0;
	vkGetDeviceQueue = 0;
	vkDestroySurfaceKHR = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR = 0;
	vkEnumerateDeviceExtensionProperties = 0;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR = 0;
	vkCreateSwapchainKHR = 0;
	vkDestroySwapchainKHR = 0;
	vkGetSwapchainImagesKHR = 0;
	vkAcquireNextImageKHR = 0;
	vkCreateImage = 0;
	vkDestroyImage = 0;
	vkCreateImageView = 0;
	vkDestroyImageView = 0;
	vkCreateShaderModule = 0;
	vkDestroyShaderModule = 0;
	vkCreatePipelineLayout = 0;
	vkDestroyPipelineLayout = 0;
	vkCreateRenderPass = 0;
	vkDestroyRenderPass = 0;
	vkCreateGraphicsPipelines = 0;
	vkDestroyPipeline = 0;
	vkCreateFramebuffer = 0;
	vkDestroyFramebuffer = 0;
	vkDestroyCommandPool = 0;
	vkCreateCommandPool = 0;
	vkAllocateCommandBuffers = 0;
	vkFreeCommandBuffers = 0;
	vkBeginCommandBuffer = 0;
	vkEndCommandBuffer = 0;
	vkResetCommandBuffer = 0;
	vkCmdExecuteCommands = 0;
	vkCmdBeginRenderPass = 0;
	vkCmdBindPipeline = 0;
	vkCmdBindVertexBuffers = 0;
	vkCmdDraw = 0;
	vkCmdEndRenderPass = 0;
	vkCmdBlitImage = 0;
	vkCmdResolveImage = 0;
	vkCmdCopyBuffer = 0;
	vkCmdCopyBufferToImage = 0;
	vkCmdPipelineBarrier = 0;
	vkCmdBindDescriptorSets = 0;
	vkCmdPushConstants = 0;
	vkCmdNextSubpass = 0;
	vkCmdSetViewport = 0;
	vkCmdSetScissor = 0;
	vkCreateSemaphore = 0;
	vkDestroySemaphore = 0;
	vkCreateFence = 0;
	vkDestroyFence = 0;
	vkWaitForFences = 0;
	vkResetFences = 0;
	vkQueueSubmit = 0;
	vkQueueWaitIdle = 0;
	vkQueuePresentKHR = 0;
	vkDeviceWaitIdle = 0;
	vkGetImageMemoryRequirements = 0;
	vkGetBufferMemoryRequirements = 0;
	vkGetDeviceImageMemoryRequirements = 0;
	vkGetDeviceBufferMemoryRequirements = 0;
	vkAllocateMemory = 0;
	vkBindImageMemory = 0;
	vkBindBufferMemory = 0;
	vkFreeMemory = 0;
	vkCreateBuffer = 0;
	vkDestroyBuffer = 0;
	vkMapMemory = 0;
	vkUnmapMemory = 0;
	vkCreateDescriptorSetLayout = 0;
	vkDestroyDescriptorSetLayout = 0;
	vkCreateDescriptorPool = 0;
	vkDestroyDescriptorPool = 0;
	vkAllocateDescriptorSets = 0;
	vkFreeDescriptorSets = 0;
	vkUpdateDescriptorSets = 0;
	vkCreateSampler = 0;
	vkDestroySampler = 0;

#if defined(_DEBUG)
	vkCreateDebugReportCallbackEXT = 0;
	vkDestroyDebugReportCallbackEXT = 0;
#endif

vkDestroySurfaceKHR = 0;
vkGetPhysicalDeviceSurfaceCapabilitiesKHR = 0;
vkGetPhysicalDeviceSurfaceFormatsKHR = 0;
vkGetPhysicalDeviceSurfacePresentModesKHR = 0;
vkGetPhysicalDeviceSurfaceSupportKHR = 0;

vkAcquireNextImageKHR = 0;
vkCreateSwapchainKHR = 0;
vkDestroySwapchainKHR = 0;
vkGetSwapchainImagesKHR = 0;

#if defined(_WIN32)
	vkGetMemoryWin32HandleKHR = 0;
	vkGetSemaphoreWin32HandleKHR = 0;
	vkImportSemaphoreWin32HandleKHR = 0;
#else
	vkGetMemoryFdKHR = 0;
	vkGetSemaphoreFdKHR = 0;
#endif
}

#undef VK_FUNC
#undef VK_LOAD_AND_CHECK
