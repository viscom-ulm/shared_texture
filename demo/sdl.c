// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_vulkan.h>

#include "vulkan.c"
#include "opengl.c"

static void SDL2_DestroyWindow(SDL_Window *Window);
static void SDL2_UpdateWindow(SDL_Window *Window, float Delta);
static SDL_Window *SDL2_CreateOpenGLWindow(int32_t Width, int32_t Height);
static SDL_Window *SDL2_CreateVulkanWindow(int32_t Width, int32_t Height);
static bool SDL2_CollectEvents(void);
static bool SDL2_Init(void);

//
//
//

typedef struct sdl2_gl
{
    SDL_GLContext GLContext;
    gl_state GL;
} sdl2_gl;

typedef struct sdl2_vk
{
    vk_state VK;
} sdl2_vk;

static void SDL2_DestroyWindow(SDL_Window *Window)
{
    if (!Window) return;

    uint32_t Flags = SDL_GetWindowFlags(Window);
    
    if (Flags & SDL_WINDOW_OPENGL)
    {
        sdl2_gl *GL = SDL_GetWindowData(Window, "opengl");
        SDL_GL_DeleteContext(GL->GLContext);
        free(GL);
    }
    if (Flags & SDL_WINDOW_VULKAN)
    {
        sdl2_vk *VK = SDL_GetWindowData(Window, "vulkan");
        Vulkan_Destroy(&VK->VK);
        Vulkan_DestroyInstance(&VK->VK);
        free(VK);
    }

    SDL_DestroyWindow(Window);
}

static void SDL2_UpdateWindow(SDL_Window *Window, float Delta)
{
    if (!Window) return;

    uint32_t Flags = SDL_GetWindowFlags(Window);
    int32_t Width, Height;
    SDL_GetWindowSize(Window, &Width, &Height);

    if (Flags & SDL_WINDOW_OPENGL)
    {
        sdl2_gl *GL = SDL_GetWindowData(Window, "opengl");
        SDL_GL_MakeCurrent(Window, GL->GLContext);
        OpenGL_Update(&GL->GL, Width, Height, Delta);
        SDL_GL_SwapWindow(Window);
    }
    if (Flags & SDL_WINDOW_VULKAN)
    {
        sdl2_vk *VK = SDL_GetWindowData(Window, "vulkan");
        Vulkan_Update(&VK->VK, Width, Height, Delta);
    }
}

static void SDL2_LogFunction(const char *Message)
{
    printf("%s\n", Message);
    SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "%s", Message);
}

static SDL_Window *SDL2_CreateOpenGLWindow(int32_t Width, int32_t Height)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

    uint32_t WindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;
    SDL_Window *Window = SDL_CreateWindow("OpenGL Window",
                                          SDL_WINDOWPOS_UNDEFINED, 
                                          SDL_WINDOWPOS_UNDEFINED,
                                          Width, 
                                          Height,
                                          WindowFlags);
    if (!Window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create Window!");
        return 0;
    }

    sdl2_gl *GL = malloc(sizeof(sdl2_gl));
    GL->GLContext = SDL_GL_CreateContext(Window);
    if (!GL->GLContext)
    {
        free(GL);
        SDL_DestroyWindow(Window);
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create OpenGL Context!");
        return 0;
    }

    if (!OpenGL_Init(&GL->GL, SDL_GL_GetProcAddress, SDL2_LogFunction))
    {
        free(GL);
        SDL_DestroyWindow(Window);
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create OpenGL!");
        return 0;        
    }

    SDL_SetWindowData(Window, "opengl", GL);
    SDL_ShowWindow(Window);
    SDL_RaiseWindow(Window);
    return Window;
}

static SDL_Window *SDL2_CreateVulkanWindow(int32_t Width, int32_t Height)
{
    uint32_t WindowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN;
    SDL_Window *Window = SDL_CreateWindow("Vulkan Window",
                                          SDL_WINDOWPOS_UNDEFINED, 
                                          SDL_WINDOWPOS_UNDEFINED,
                                          Width, 
                                          Height,
                                          WindowFlags);
    if (!Window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create Window!");
        return 0;
    }

    uint32_t ExtCount;
    if (!SDL_Vulkan_GetInstanceExtensions(Window, &ExtCount, 0))
    {
        SDL_DestroyWindow(Window);
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't query Vulkan required extensions!");
        return 0;
    }
    char** ExtNames = malloc(sizeof(char *) * (ExtCount + 1));
    if (!SDL_Vulkan_GetInstanceExtensions(Window, &ExtCount, (const char **)ExtNames))
    {
        free(ExtNames);
        SDL_DestroyWindow(Window);
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't query Vulkan extension names!");
        return 0;
    }
#if defined(_DEBUG)
    ExtNames[ExtCount++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
#endif
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
    VkInstance Instance = Vulkan_CreateInstance(ExtCount, (const char **)ExtNames);
    free(ExtNames);
    if (Instance == VK_NULL_HANDLE)
    {
        SDL_DestroyWindow(Window);
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create Vulkan instance!");
        return 0;
    }

    VkSurfaceKHR Surface;
    if (!SDL_Vulkan_CreateSurface(Window, Instance, &Surface))
    {
        SDL2_DestroyWindow(Window);
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't create Vulkan surface!");
        return 0;
    }

    sdl2_vk *VK = malloc(sizeof(sdl2_vk));
    if (!Vulkan_InitInstance(&VK->VK, Instance, Surface, SDL2_LogFunction))
    {
        free(VK);
        SDL2_DestroyWindow(Window);
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't initialize Vulkan instance!");
        return 0;
    }

    if (!Vulkan_Init(&VK->VK))
    {
        free(VK);
        SDL2_DestroyWindow(Window);
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't initialize Vulkan!");
        return 0;        
    }

    SDL_SetWindowData(Window, "vulkan", VK);
    SDL_ShowWindow(Window);
    SDL_RaiseWindow(Window);
    return Window;
}

static bool SDL2_CollectEvents(void)
{
    SDL_Event Event;
    while (SDL_PollEvent(&Event))
    {
        /* PROCESS EVENTS */
        switch (Event.type)
        {
            /* QUIT EVENT */
            case SDL_QUIT:
            {
                return false;
            } break;

            case SDL_WINDOWEVENT:
            {
                switch (Event.window.event)
                {
                    case SDL_WINDOWEVENT_CLOSE:
                    {
                        SDL2_DestroyWindow(SDL_GetWindowFromID(Event.window.windowID));
                    }
                }
            }
        }
    }

    return true;
}

static bool SDL2_Init(void)
{
    /* INIT */
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Couldn't initialize SDL2!");
        return 0;
    }

    /* OPENGL */
    SDL_GL_LoadLibrary(0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 
                        SDL_GL_CONTEXT_PROFILE_CORE);

    /* VULKAN */
    SDL_Vulkan_LoadLibrary(0);

    return 1;
}

void SDL2_Destroy(void)
{
    SDL_GL_UnloadLibrary();
    SDL_Vulkan_UnloadLibrary();
}
