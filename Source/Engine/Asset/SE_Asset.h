// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Math.h"
#include "Core/SE_String.h"

#include "RHI/DX12/DX12.h"

#include "ThirdParty/json/Include/json.h"

namespace Engine
{
    struct Asset_State {
        Set <String> used_names;
        int suffix = 0;

        FORCE_INLINE void reset_suffix() { suffix = 0; }
        FORCE_INLINE String make_name(const String& base_name) {
            String name;
            for (;;) {
                name = base_name + std::to_string(suffix++);
                if (!used_names.contains(name)) { break; }
            }
            return name;
        }

    };

    struct Shader_Material {
        u32 albedo_id;
        u32 normal_id;
        u32 orm_id;
        u32 emissive_id;
    };

    struct Material {
        String name;

        u32 id;
        DX12_Resource*  resource;
        DX12_Descriptor srv;

        Shader_Material shader_material;
        Array <DX12_Resource*>  resources;
        Array <DX12_Descriptor> srvs;
    };

    struct Vertex {
        vec3 position;
        vec3 normal;
        vec2 uv;
        vec4 tangent;
    };

    struct Mesh_Slice {
        Array<Vertex> vertices;
        Array<u32>    indices;
        String        material_name;
    };

    struct Mesh {
        String name;
        Array <Mesh_Slice*> meshes;
    };


    struct GLTF_Load_Result {
        nlohmann::json materials; // array of materials jsons
        Array <Mesh*> meshes;
    };

    ENGINE_API GLTF_Load_Result load_gltf(Asset_State* state, const String& path);
}
