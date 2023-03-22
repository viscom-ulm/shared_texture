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
        public readonly int id;
        public readonly uint format;
        public readonly int width, height;
    }

    [DllImport("shared_texture")]
    private static extern SharedTextureStruct CreateSharedTexture(string Name, int width, int height, SharedTextureFormat format);

    [DllImport("shared_texture")]
    private static extern IntPtr GetAttachSharedTextureFunc();

    [DllImport("shared_texture")]
    private static extern IntPtr GetDestroySharedTextureFunc();

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

    public SharedTexture(string name, int width, int height, TextureFormat format)
    {
        texture = new Texture2D(width, height, format, false);
        SharedTextureFormat sharedFormat = FromUnityTextureFormat(texture.format);
        sharedTexture = CreateSharedTexture(name, texture.width, texture.height, sharedFormat);
        GL.IssuePluginEvent(GetAttachSharedTextureFunc(), sharedTexture.id);
    }

    ~SharedTexture()
    {
        GL.IssuePluginEvent(GetDestroySharedTextureFunc(), sharedTexture.id);
    }
}
