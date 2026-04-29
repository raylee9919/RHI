// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Math.h"
#include "Core/SE_Array.h"
#include "Core/SE_String.h"

#include "RHI/DX12/RHI_DX12.h"
using namespace Engine::DX12;

#include "GFX/gfx.h"

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

    struct Mesh
    {
        Array<Vertex> vertices;
        Array<uint32> indices;

        Buffer     vertex_buffer;
        Descriptor vertex_buffer_descriptor;

        Buffer     index_buffer;

        AABB aabb;
    };

    struct Scene_Component
    {
        String name;

        m4x4 local_transform = m4x4::Identity();
        Array <Scene_Component*> children;
        Array <Mesh*> meshes;
    };

    [[nodiscard]] ENGINE_API Scene_Component* LoadGLTF(GFX::State* gfx, const String& path);
}
