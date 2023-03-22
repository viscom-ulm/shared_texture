// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#pragma once

typedef struct unity_shared_texture
{
    int32_t Id;
    uint32_t Format;
    int32_t Width, Height;
} unity_shared_texture;

unity_shared_texture UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API CreateSharedTexture(const char *Name, int32_t Width, int32_t Height, uint32_t Format);
UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetLoadSharedTextureFunc();
UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetDestroySharedTextureFunc();
