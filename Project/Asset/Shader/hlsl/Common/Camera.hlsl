// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef CAMERA_HLSL
#define CAMERA_HLSL

struct Camera
{
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;

    float4 position;
}

#endif
