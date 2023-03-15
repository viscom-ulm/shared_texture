// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#define SHARED_TEXTURE_OPENGL
#define SHARED_TEXTURE_VULKAN
#include "share.h"

#if _WIN32

#if _DLL
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}
#endif

#else
  #error posix not implemented
#endif

#define PIPE_PREFIX "\\\\.\\pipe\\shared_texture_"

#if defined(_WIN32)

typedef struct send_thread_data
{
    HANDLE Pipe;
    shared_texture SharedTexture;
} send_thread_data;

static DWORD WINAPI SharedTexture_SendThreadProc(LPVOID lpParam)
{
    send_thread_data *Data = lpParam;
    
    ConnectNamedPipe(Data->Pipe, NULL);
    WriteFile(Data->Pipe, &Data->SharedTexture, sizeof(shared_texture), NULL, NULL);
    CloseHandle(Data->Pipe);
    HeapFree(GetProcessHeap(), 0, Data);

    return 0;
}

static bool SharedTexture_Send(shared_texture SharedTexture, const char *Name)
{
    char PipeName[MAX_PATH] = { 0 };
    strncpy(PipeName, PIPE_PREFIX, MAX_PATH - 1);
    strncpy(PipeName + sizeof(PIPE_PREFIX), Name, MAX_PATH - 1 - sizeof(PIPE_PREFIX));
    HANDLE Pipe = CreateNamedPipeA(PipeName,
                                   PIPE_ACCESS_OUTBOUND,
                                   PIPE_TYPE_BYTE | PIPE_WAIT,
                                   1, sizeof(shared_texture),
                                   sizeof(shared_texture), 0, NULL);
    if (Pipe == INVALID_HANDLE_VALUE) return false;

    send_thread_data *SendData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(send_thread_data));
    SendData->Pipe = Pipe;
    SendData->SharedTexture = SharedTexture;
    HANDLE Thread = CreateThread(NULL, 0, SharedTexture_SendThreadProc, SendData, 0, NULL);
    return true;
}

static bool SharedTexture_Receive(shared_texture *SharedTexture, const char *Name)
{
    char PipeName[MAX_PATH] = { 0 };
    strncpy(PipeName, PIPE_PREFIX, MAX_PATH - 1);
    strncpy(PipeName + sizeof(PIPE_PREFIX), Name, MAX_PATH - 1 - sizeof(PIPE_PREFIX));
    HANDLE Pipe = CreateFile(PipeName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (Pipe == INVALID_HANDLE_VALUE) return false;
    
    ReadFile(Pipe, SharedTexture, sizeof(shared_texture), NULL, NULL);
    ULONG ServerProcessId;
    GetNamedPipeServerProcessId(Pipe, &ServerProcessId);
    HANDLE ServerProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ServerProcessId);
    DuplicateHandle(ServerProcess, SharedTexture->Win32.MemoryHandle, GetCurrentProcess(), &SharedTexture->Win32.MemoryHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(ServerProcess, SharedTexture->Win32.SemaphoreHandle, GetCurrentProcess(), &SharedTexture->Win32.SemaphoreHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
    CloseHandle(Pipe);

    return true;
}
#else
#error posix not implemented
static void SharedTexture_Send(shared_texture SharedTexture) { }
static bool SharedTexture_Receive(shared_texture *SharedTexture) { return true; }
#endif

//
// VULKAN
//

#include "vk_funcs.h"
#include "vk_utils.h"

struct
{
    VkInstance Instance;
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
} VK;

bool SHARED_TEXTURE_EXPORT SharedTexture_Init(void)
{
#if _WIN32
    HMODULE VulkanDLL = LoadLibraryA("vulkan-1.dll");
    if (!VulkanDLL) return false;
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(VulkanDLL, "vkGetInstanceProcAddr");
#endif

    if (!VK_LoadFunctions())
        return false;

    VK.Instance = VK_NULL_HANDLE;
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
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = 0,
            .enabledExtensionCount = 0,
            .ppEnabledExtensionNames = 0
        },
        0, &VK.Instance
    );
    if (Result != VK_SUCCESS)
        return false;

    if (!VK_LoadInstanceFunctions(VK.Instance))
        return false;

    const char* ExtNames[] = {
#if defined(_WIN32)
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME
#else
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME
#endif
    };
    const uint32_t ExtCount = sizeof(ExtNames) / sizeof(ExtNames[0]);

    VK.PhysicalDevice = Vulkan_FindPhysicalDevice(VK.Instance, VK_NULL_HANDLE, ExtCount, ExtNames);
    if (VK.PhysicalDevice == VK_NULL_HANDLE) return false;
    uint32_t QueueIndex = Vulkan_DefaultQueueFamilyIndex(VK.PhysicalDevice, VK_NULL_HANDLE);

    Result = vkCreateDevice(VK.PhysicalDevice,
        &(VkDeviceCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pQueueCreateInfos = (VkDeviceQueueCreateInfo[]) {
                    [0] = {
                        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                        .queueFamilyIndex = QueueIndex,
                        .queueCount = 1,
                        .pQueuePriorities = (float[]){ 1.0f }
                    },
                },
            .queueCreateInfoCount = 1,
            .enabledExtensionCount = ExtCount,
            .ppEnabledExtensionNames = ExtNames,
            .pEnabledFeatures = 0,
        },
        0, &VK.Device
    );
    if (Result != VK_SUCCESS)
        return false;

    return true;
}

shared_texture SHARED_TEXTURE_EXPORT SharedTexture_Open(const char *Name)
{
    shared_texture SharedTexture = { 0 };
    if(SharedTexture_Receive(&SharedTexture, Name))
        return SharedTexture;

    return (shared_texture) { .Format = SHARED_TEXTURE_NONE };
}

shared_texture SHARED_TEXTURE_EXPORT SharedTexture_Create(const char *Name, int32_t Width, int32_t Height, shared_texture_format Format)
{
    // IMAGE
    VkImage Image;
    vkCreateImage(VK.Device,
        &(VkImageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = &(VkExternalMemoryImageCreateInfo){
                .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
                .handleTypes = VULKAN_EXTERNAL_MEMORY_HANDLE_TYPE
            },
            .flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = SharedTexture_ToVulkanFormat(Format),
            .extent = {
                .width = Width,
                .height = Height,
                .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = 0,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
        0, &Image
    );

    // MEMORY
    VkDeviceMemory Memory;
    VkMemoryRequirements MemReqs;
    vkGetImageMemoryRequirements(VK.Device, Image, &MemReqs);
    uint32_t MemoryTypeIndex = Vulkan_FindPhysicalDeviceMemoryIndex(VK.PhysicalDevice,
        MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(VK.Device,
        &(VkMemoryAllocateInfo) {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = &(VkExportMemoryAllocateInfo){
                .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
                .handleTypes = VULKAN_EXTERNAL_MEMORY_HANDLE_TYPE
            },
            .allocationSize = MemReqs.size,
            .memoryTypeIndex = MemoryTypeIndex,
        },
        0, &Memory
    );

    // SEMAPHORE
    VkSemaphore Semaphore;
    vkCreateSemaphore(VK.Device,
        &(VkSemaphoreCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &(VkExportSemaphoreCreateInfo){
                .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
                .handleTypes = VULKAN_EXTERNAL_SEMAPHORE_HANDLE_TYPE
            },
        },
        0, &Semaphore
    );

#if defined(_WIN32)
    HANDLE Win32MemoryHandle;
    vkGetMemoryWin32HandleKHR(VK.Device,
        &(VkMemoryGetWin32HandleInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
            .memory = Memory,
            .handleType = VULKAN_EXTERNAL_MEMORY_HANDLE_TYPE 
        }, &Win32MemoryHandle
    );

    HANDLE Win32SemaphoreHandle;
    vkGetSemaphoreWin32HandleKHR(VK.Device,
        &(VkSemaphoreGetWin32HandleInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR,
            .semaphore = Semaphore,
            .handleType = VULKAN_EXTERNAL_SEMAPHORE_HANDLE_TYPE,
        }, &Win32SemaphoreHandle
    );
#else
    int PosixMemoryHandle;
    vkGetMemoryFdKHR(VK.Device,
        &(VkMemoryGetFdInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
            .memory = Memory,
            .handleType = VULKAN_EXTERNAL_MEMORY_HANDLE_TYPE 
        }, &PosixMemoryHandle
    );

    int PosixSemaphoreHandle;
    vkGetSemaphoreFdKHR(VK.Device,
        &(VkSemaphoreGetFdInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR,
            .semaphore = Semaphore,
            .handleType = VULKAN_EXTERNAL_SEMAPHORE_HANDLE_TYPE,
        }, &PosixSemaphoreHandle
    );
#endif

    vkFreeMemory(VK.Device, Memory, 0);
    vkDestroyImage(VK.Device, Image, 0);
    vkDestroySemaphore(VK.Device, Semaphore, 0);

    shared_texture SharedTexture = {
        .Format = Format,
        .Width = Width,
        .Height = Height,
        .Size = MemReqs.size,
    #if defined(_WIN32)
        .Win32 = {
            .MemoryHandle = Win32MemoryHandle,
            .SemaphoreHandle = Win32SemaphoreHandle,
        }
    #else
        .Posix = {
            .MemoryHandle = PosixMemoryHandle,
            .SemaphoreHandle = PosixSemaphoreHandle,
        }
    #endif
    };

    if (SharedTexture_Send(SharedTexture, Name))
        return SharedTexture;

    SharedTexture_Close(SharedTexture);
    return (shared_texture) { .Format = SHARED_TEXTURE_NONE };    
}

void SHARED_TEXTURE_EXPORT SharedTexture_Close(shared_texture SharedTexture)
{
#if _WIN32
    CloseHandle(SharedTexture.Win32.MemoryHandle);
    CloseHandle(SharedTexture.Win32.SemaphoreHandle);
#else
  #error posix not implemented
#endif
}

shared_texture SharedTexture_OpenOrCreate(const char *Name, int32_t Width, int32_t Height, uint32_t Format)
{
    shared_texture SharedTexture = SharedTexture_Open(Name);
    if (SharedTexture.Format == SHARED_TEXTURE_NONE)
        SharedTexture = SharedTexture_Create(Name, Width, Height, Format);
    return SharedTexture;
}

#include "unity.c"
