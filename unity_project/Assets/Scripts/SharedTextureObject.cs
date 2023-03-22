// Copyright 2023 Visual Computing Group, Ulm University
// Author: Jan Eric Haßler

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SharedTextureObject : MonoBehaviour
{
    SharedTexture sharedTexture;

    private void Start()
    {
        sharedTexture = new SharedTexture("demo", 1920, 1080, TextureFormat.RGBA32);
        gameObject.GetComponent<Renderer>().material.SetTexture("_MainTex", sharedTexture.texture);
    }

    void OnEnable()
    {
    }

    private void OnDisable()
    {
    }

    void Update()
    {

    }
}
