// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

#pragma once

typedef struct unity_shared_texture
{
    void *NativeTex;
    uint32_t Format;
    int32_t Width, Height;
} unity_shared_texture;

unity_shared_texture UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API CreateSharedTexture(const char *Name, int32_t Width, int32_t Height, uint32_t Format);
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API DestroySharedTexture(unity_shared_texture SharedTexture);
