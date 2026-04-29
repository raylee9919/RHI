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

    struct Scene_Component
    {
        m4x4 local_transform = m4x4::Identity();
        Array <Scene_Component*> children;
        Array <Mesh*> meshes;
    };


    [[nodiscard]] ENGINE_API Scene_Component* LoadGLTF(const String& path);
}
