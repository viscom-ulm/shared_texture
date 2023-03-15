// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#pragma once

#if defined(_WIN32)
  #define VC_EXTRALEAN
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>

typedef enum shared_texture_format
{
    SHARED_TEXTURE_NONE = 0,
    SHARED_TEXTURE_RGBA8,
    SHARED_TEXTURE_DEPTH,
} shared_texture_format;

typedef struct shared_texture
{
    uint32_t Format;
    int32_t Width, Height;
    uint64_t Size;
#if defined(_WIN32)
    struct
    {
        HANDLE MemoryHandle;
        HANDLE SemaphoreHandle;
    } Win32;
#else
    struct
    {
        int MemoryHandle;
        int SemaphoreHandle;
    } Posix;
#endif
} shared_texture;

#define SHARED_TEXTURE_EXPORT __declspec(dllexport) __cdecl

#ifdef __cplusplus
extern "C" {
#endif

bool SHARED_TEXTURE_EXPORT SharedTexture_Init(void);
shared_texture SHARED_TEXTURE_EXPORT SharedTexture_Open(const char *Name);
shared_texture SHARED_TEXTURE_EXPORT SharedTexture_Create(const char *Name, int32_t Width, int32_t Height, uint32_t Format);
shared_texture SHARED_TEXTURE_EXPORT SharedTexture_OpenOrCreate(const char *Name, int32_t Width, int32_t Height, uint32_t Format);
void SHARED_TEXTURE_EXPORT SharedTexture_Close(shared_texture SharedTexture);

#ifdef __cplusplus
}
#endif

#if defined(SHARED_TEXTURE_OPENGL)

#include <gl/gl.h>
#include <gl/glext.h>

typedef struct gl_shared_texture
{
    GLuint Texture;
    GLuint Memory;
    GLuint Semaphore;
} gl_shared_texture;

static gl_shared_texture SharedTexture_ToOpenGL(shared_texture SharedTexture);
static bool SharedTexture_OpenGLWait(gl_shared_texture SharedTexture);
static void SharedTexture_OpenGLSignal(gl_shared_texture SharedTexture);
static GLuint SharedTexture_ToOpenGLFormat(shared_texture_format Format);

#endif // defined(SHARED_TEXTURE_OPENGL)

#if defined(SHARED_TEXTURE_VULKAN)

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#if defined(_WIN32)
  #include <vulkan/vulkan_win32.h>
#endif

typedef struct vk_shared_texture
{
    VkImage Image;
    VkDeviceMemory Memory;
    VkSemaphore Semaphore;
} vk_shared_texture;

static vk_shared_texture SharedTexture_ToVulkan(shared_texture SharedTexture, VkDevice Device, VkPhysicalDevice PhysicalDevice);
static void SharedTexture_DestroyVulkanTexture(vk_shared_texture SharedTexture, VkDevice Device);
static VkFormat SharedTexture_ToVulkanFormat(shared_texture_format Format);

#endif // defined(SHARED_TEXTURE_VULKAN)

////////////////////////
//                    //
//   IMPLEMENTATION   //
//                    //
////////////////////////

#if defined(SHARED_TEXTURE_OPENGL)

PFNGLCREATEMEMORYOBJECTSEXTPROC glCreateMemoryObjectsEXT;
PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC glImportMemoryWin32HandleEXT;
PFNGLIMPORTMEMORYFDEXTPROC glImportMemoryFdEXT;
PFNGLCREATETEXTURESPROC glCreateTextures;
PFNGLTEXTUREPARAMETERIPROC glTextureParameteri;
PFNGLTEXTURESTORAGEMEM2DEXTPROC glTextureStorageMem2DEXT;
PFNGLGENSEMAPHORESEXTPROC glGenSemaphoresEXT;
PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC glImportSemaphoreWin32HandleEXT;
PFNGLIMPORTSEMAPHOREFDEXTPROC glImportSemaphoreFdEXT;
PFNGLDELETEMEMORYOBJECTSEXTPROC glDeleteMemoryObjectsEXT;
PFNGLDELETESEMAPHORESEXTPROC glDeleteSemaphoresEXT;
PFNGLWAITSEMAPHOREEXTPROC glWaitSemaphoreEXT;
PFNGLSIGNALSEMAPHOREEXTPROC glSignalSemaphoreEXT;

static GLuint SharedTexture_ToOpenGLFormat(shared_texture_format Format)
{
    switch (Format)
    {
        case SHARED_TEXTURE_RGBA8: return GL_RGBA8;
        case SHARED_TEXTURE_DEPTH: return GL_DEPTH_COMPONENT32F;
    }
    return VK_FORMAT_UNDEFINED;
}

static gl_shared_texture SharedTexture_ToOpenGL(shared_texture SharedTexture)
{
    GLuint Format = SharedTexture_ToOpenGLFormat(SharedTexture.Format);

    GLuint Memory;
    glCreateMemoryObjectsEXT(1, &Memory);
#if defined(_WIN32)
    glImportMemoryWin32HandleEXT(Memory, SharedTexture.Size, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, SharedTexture.Win32.MemoryHandle);
#else
    glImportMemoryFdEXT(Memory, SharedTexture.Size, GL_HANDLE_TYPE_OPAQUE_FD_EXT, SharedTexture.Posix.MemoryHandle);
#endif

    GLuint Texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &Texture);
    glTextureParameteri(Texture, GL_TEXTURE_TILING_EXT, GL_OPTIMAL_TILING_EXT);
    glTextureStorageMem2DEXT(Texture, 1, Format, SharedTexture.Width, SharedTexture.Height, Memory, 0);

    GLuint Semaphore;
    glGenSemaphoresEXT(1, &Semaphore);
#if defined(_WIN32)
    glImportSemaphoreWin32HandleEXT(Semaphore, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, SharedTexture.Win32.SemaphoreHandle);
#else
    glImportSemaphoreFdEXT(Semaphore, GL_HANDLE_TYPE_OPAQUE_FD_EXT, SharedTexture.Posix.SemaphoreHandle);
#endif

    gl_shared_texture GLSharedTexture;
    GLSharedTexture.Texture = Texture;
    GLSharedTexture.Memory = Memory;
    GLSharedTexture.Semaphore = Semaphore;
    return GLSharedTexture;
}

static void SharedTexture_DestroyOpenGLTexture(gl_shared_texture GLSharedTexture)
{
    glDeleteTextures(1, &GLSharedTexture.Texture);
    glDeleteMemoryObjectsEXT(1, &GLSharedTexture.Memory);
    glDeleteSemaphoresEXT(1, &GLSharedTexture.Semaphore);
}

static bool SharedTexture_OpenGLWait(gl_shared_texture GLSharedTexture)
{
    glWaitSemaphoreEXT(GLSharedTexture.Semaphore, 0, 0, 1, &GLSharedTexture.Texture, (GLenum[]){ GL_LAYOUT_SHADER_READ_ONLY_EXT });
    return glGetError() == GL_NO_ERROR;
}

static void SharedTexture_OpenGLSignal(gl_shared_texture GLSharedTexture)
{
    glSignalSemaphoreEXT(GLSharedTexture.Semaphore, 0, 0, 1, &GLSharedTexture.Texture, (GLenum[]){ GL_LAYOUT_SHADER_READ_ONLY_EXT });
}

#endif // defined(SHARED_TEXTURE_OPENGL)

//
// VULKAN
//

#if defined(SHARED_TEXTURE_VULKAN)

#if defined(_WIN32)
  #define VULKAN_EXTERNAL_MEMORY_HANDLE_TYPE VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
  #define VULKAN_EXTERNAL_SEMAPHORE_HANDLE_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
#else
  #define VULKAN_EXTERNAL_MEMORY_HANDLE_TYPE VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
  #define VULKAN_EXTERNAL_SEMAPHORE_HANDLE_TYPE VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
#endif

PFN_vkCreateImage vkCreateImage;
PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
PFN_vkAllocateMemory vkAllocateMemory;
PFN_vkBindImageMemory vkBindImageMemory;
PFN_vkCreateSemaphore vkCreateSemaphore;
PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR;
PFN_vkFreeMemory vkFreeMemory;
PFN_vkDestroyImage vkDestroyImage;
PFN_vkDestroySemaphore vkDestroySemaphore;

static VkFormat SharedTexture_ToVulkanFormat(shared_texture_format Format)
{
    switch (Format)
    {
        case SHARED_TEXTURE_RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
        case SHARED_TEXTURE_DEPTH: return VK_FORMAT_D32_SFLOAT;
    }
    return VK_FORMAT_UNDEFINED;
}

static vk_shared_texture SharedTexture_ToVulkan(shared_texture SharedTexture, VkDevice Device, VkPhysicalDevice PhysicalDevice)
{
    VkFormat Format = SharedTexture_ToVulkanFormat(SharedTexture.Format);

    // IMAGE
    VkImage Image;
    VkExternalMemoryImageCreateInfo ExternalMemoryImageCreateInfo;
    ExternalMemoryImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    ExternalMemoryImageCreateInfo.pNext = 0;
    ExternalMemoryImageCreateInfo.handleTypes = VULKAN_EXTERNAL_MEMORY_HANDLE_TYPE;
    VkImageCreateInfo ImageCreateInfo;
    ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    ImageCreateInfo.pNext = &ExternalMemoryImageCreateInfo;
    ImageCreateInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.format = Format;
    ImageCreateInfo.extent.width = SharedTexture.Width;
    ImageCreateInfo.extent.height = SharedTexture.Height;
    ImageCreateInfo.extent.depth = 1;
    ImageCreateInfo.mipLevels = 1;
    ImageCreateInfo.arrayLayers = 1;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.pQueueFamilyIndices = 0;
    ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(Device, &ImageCreateInfo, 0, &Image);

    // MEMORY
    VkDeviceMemory Memory;
    VkMemoryRequirements MemReqs;
    vkGetImageMemoryRequirements(Device, Image, &MemReqs);
    uint32_t MemoryTypeIndex = Vulkan_FindPhysicalDeviceMemoryIndex(PhysicalDevice,
        MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkImportMemoryWin32HandleInfoKHR ImportMemoryWin32HandleInfoKHR;
    ImportMemoryWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
    ImportMemoryWin32HandleInfoKHR.pNext = 0;
    ImportMemoryWin32HandleInfoKHR.handleType = VULKAN_EXTERNAL_MEMORY_HANDLE_TYPE;
    ImportMemoryWin32HandleInfoKHR.handle = SharedTexture.Win32.MemoryHandle;
    ImportMemoryWin32HandleInfoKHR.name = 0;
    VkMemoryAllocateInfo MemoryAllocateInfo;
    MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocateInfo.pNext = &ImportMemoryWin32HandleInfoKHR;
    MemoryAllocateInfo.allocationSize = MemReqs.size;
    MemoryAllocateInfo.memoryTypeIndex = MemoryTypeIndex;
    vkAllocateMemory(Device, &MemoryAllocateInfo, 0, &Memory);
    vkBindImageMemory(Device, Image, Memory, 0);
    
    // SEMAPHORE
    VkSemaphore Semaphore;
    VkSemaphoreCreateInfo SemaphoreCreateInfo;
    SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    SemaphoreCreateInfo.pNext = 0;
    SemaphoreCreateInfo.flags = 0;
    vkCreateSemaphore(Device, &SemaphoreCreateInfo, 0, &Semaphore);

#if _WIN32
    VkImportSemaphoreWin32HandleInfoKHR ImportSemaphoreWin32HandleInfoKHR;
    ImportSemaphoreWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
    ImportSemaphoreWin32HandleInfoKHR.pNext = 0;
    ImportSemaphoreWin32HandleInfoKHR.semaphore = Semaphore;
    ImportSemaphoreWin32HandleInfoKHR.flags = 0;
    ImportSemaphoreWin32HandleInfoKHR.handleType = VULKAN_EXTERNAL_SEMAPHORE_HANDLE_TYPE;
    ImportSemaphoreWin32HandleInfoKHR.handle = SharedTexture.Win32.SemaphoreHandle;
    ImportSemaphoreWin32HandleInfoKHR.name = 0;
    VkResult Result = vkImportSemaphoreWin32HandleKHR(Device, &ImportSemaphoreWin32HandleInfoKHR);
#else
  #error TODO Implement Posix
#endif

    vk_shared_texture VKSharedTexture;
    VKSharedTexture.Image = Image;
    VKSharedTexture.Memory = Memory;
    VKSharedTexture.Semaphore = Semaphore;
    return VKSharedTexture;
}

static void SharedTexture_DestroyVulkanTexture(vk_shared_texture VKSharedTexture, VkDevice Device)
{
    if (VKSharedTexture.Memory)
        vkFreeMemory(Device, VKSharedTexture.Memory, 0);

    if (VKSharedTexture.Image)
        vkDestroyImage(Device, VKSharedTexture.Image, 0);

    if (VKSharedTexture.Semaphore)
        vkDestroySemaphore(Device, VKSharedTexture.Semaphore, 0);
}

#endif // defined(SHARED_TEXTURE_VULKAN)
