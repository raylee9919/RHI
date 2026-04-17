// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Math.h"
#include "Core/SE_Array.h"
#include "Core/SE_String.h"


namespace Engine
{
    struct Vertex
    {
        vec3 position;
        vec3 normal;
        vec2 uv;
        vec4 tangent;
    };

    enum class TextureFormat : u16
    {
        RGBA8_UNORM
    };

    struct Texture
    {
        u8* data;
        u32 width;
        u32 height;
        u16 mips;
        TextureFormat format;
    };

    struct Material
    {
    };

    struct Mesh
    {
        Array<Vertex> vertices;
        Array<u32> indices;

        Material* material;

        AABB aabb;
    };

    struct SceneComponent
    {
        m4x4 local_transform = m4x4::Identity();
        Array <SceneComponent*> children;
        Array <Mesh*> meshes;
    };




    [[nodiscard]] ENGINE_API SceneComponent* LoadGLTF(const String& path);
}
