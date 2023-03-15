// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#if defined(_WIN32)
  #define VC_EXTRALEAN
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 1280
#define HEIGHT 720

#define SHARED_TEXTURE_OPENGL
#define SHARED_TEXTURE_VULKAN
#include "share.h"
#include "sdl.c"

int main(int argc, char *argv[])
{
    SDL2_Init();
    SharedTexture_Init();

    if (argc >= 2)
    {        
        SDL_Window *Window = 0;
        if (argv[1][0] == '-' && argv[1][1] == 'v' && argv[1][2] == 'k')
        {
            Window = SDL2_CreateVulkanWindow(WIDTH, HEIGHT);
        }
        else if ((argv[1][0] == '-' && argv[1][1] == 'g' && argv[1][2] == 'l'))
        {
            Window = SDL2_CreateOpenGLWindow(WIDTH, HEIGHT);
        }

        while (SDL2_CollectEvents())
        {
            float Delta = 1.0f / 60.0f;
            SDL2_UpdateWindow(Window, Delta);
        }
    }

    SDL2_Destroy();

    return 0;
}
