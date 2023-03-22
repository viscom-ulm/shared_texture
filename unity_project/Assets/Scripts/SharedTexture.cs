// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.InteropServices;
using System;

public class SharedTexture
{
    public enum SharedTextureFormat
    {
        SHARED_TEXTURE_NONE = 0,
        SHARED_TEXTURE_RGBA8,
        SHARED_TEXTURE_DEPTH,
    };

    [StructLayout(LayoutKind.Sequential)]
    public readonly struct SharedTextureStruct
    {
        public readonly IntPtr nativeTex;
        public readonly uint format;
        public readonly int width, height;
    }

    [DllImport("shared_texture")]
    private static extern SharedTextureStruct CreateSharedTexture(string Name, int width, int height, SharedTextureFormat format);

    [DllImport("shared_texture")]
    private static extern void DestroySharedTexture(SharedTextureStruct sharedTexture);

    [RuntimeInitializeOnLoadMethod]
    private static void Initialize()
    {        
        // GL.IssuePluginEvent()
    }

private SharedTextureStruct sharedTexture;
    public Texture2D texture;

    private TextureFormat ToUnityTextureFormat(SharedTextureFormat format)
    {
        switch (format)
        {
            case SharedTextureFormat.SHARED_TEXTURE_RGBA8: return TextureFormat.RGBA32;
            case SharedTextureFormat.SHARED_TEXTURE_DEPTH: return TextureFormat.RFloat;
        }
        return 0;
    }

    private SharedTextureFormat FromUnityTextureFormat(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat.RGBA32: return SharedTextureFormat.SHARED_TEXTURE_RGBA8;
            case TextureFormat.RFloat: return SharedTextureFormat.SHARED_TEXTURE_DEPTH;
        }
        return SharedTextureFormat.SHARED_TEXTURE_NONE;
    }

    public SharedTexture(string Name, int width, int height, TextureFormat format)
    {
        SharedTextureFormat sharedFormat = FromUnityTextureFormat(format);
        sharedTexture = CreateSharedTexture(Name, width, height, sharedFormat);
        TextureFormat textureFormat = ToUnityTextureFormat((SharedTextureFormat)sharedTexture.format);
        texture = Texture2D.CreateExternalTexture(sharedTexture.width, sharedTexture.height, textureFormat, false, false, sharedTexture.nativeTex);
    }

    ~SharedTexture()
    {
        DestroySharedTexture(sharedTexture);
    }
}
