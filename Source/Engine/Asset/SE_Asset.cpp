// Copyright Seong Woo Lee. All Rights Reserved.

#include "SE_Asset.h"

#include "Core/SE_Log.h"

#include "IO/IO.h"
#include "File/FileSystem.h"

#include "ThirdParty/cgltf/Include/cgltf.h"

#include "ThirdParty/json/Include/json.h"

#include "ThirdParty/stb/Include/stb_image.h"

namespace Engine
{
    using namespace Render;

    INTERNAL cgltf_accessor* cgltfGetAccessorFromType(cgltf_primitive* prim, cgltf_attribute_type type)
    {
        for (cgltf_size i = 0; i < prim->attributes_count; ++i) 
        {
            if (prim->attributes[i].type == type)
            {
                return prim->attributes[i].data;
            }
        }
        return nullptr;
    }

    INTERNAL vec2 cgltfReadVec2FromAccessor(cgltf_accessor* acc, uint index)
    {
        vec2 v;
        float f[2];
        cgltf_accessor_read_float(acc, index, f, 2);
        v.x = f[0];
        v.y = f[1];
        return v;
    }

    INTERNAL vec3 cgltfReadVec3FromAccessor(cgltf_accessor* acc, uint index)
    {
        vec3 v;
        float f[3];
        cgltf_accessor_read_float(acc, index, f, 3);
        v.x = f[0];
        v.y = f[1];
        v.z = f[2];
        return v;
    }

    INTERNAL vec4 cgltfReadVec4FromAccessor(cgltf_accessor* acc, uint index)
    {
        vec4 v;
        float f[4];
        cgltf_accessor_read_float(acc, index, f, 4);
        v = vec4(f[0], f[1], f[2], f[3]);
        return v;
    }

    INTERNAL Scene_Component* cgltfParseNode(GFX::State* gfx, cgltf_data* data, cgltf_node* node, Array<s32>& material_ids)
    {
        if (!gfx) 
        {
            return nullptr;
        }

        cgltf_mesh* mesh = node->mesh;
        if (!mesh) 
        {
            return nullptr;
        }

        Scene_Component* result = new Scene_Component;

        uint num_primitives = (uint)mesh->primitives_count;
        for (uint pi = 0; pi < num_primitives; ++pi) 
        {
            Mesh* M = new Mesh;

            AABB aabb = AABB::RevInf();

            cgltf_primitive* prim = &mesh->primitives[pi];
            cgltf_primitive_type type = prim->type;

            CORE_ASSERT(type == cgltf_primitive_type_triangles);

            cgltf_accessor* pos_acc    = cgltfGetAccessorFromType(prim, cgltf_attribute_type_position);
            cgltf_accessor* normal_acc = cgltfGetAccessorFromType(prim, cgltf_attribute_type_normal);
            cgltf_accessor* uv_acc     = cgltfGetAccessorFromType(prim, cgltf_attribute_type_texcoord);
            cgltf_accessor* tan_acc    = cgltfGetAccessorFromType(prim, cgltf_attribute_type_tangent);

            CORE_ASSERT(pos_acc);

            uint32 num_verts = (uint32)pos_acc->count;

            Array<Vertex> verts(num_verts);

            // Fetch vertex attributes.
            //
            for (uint32 vi = 0; vi < num_verts; ++vi)
            {
                auto& vert = verts[vi];

                vert.position = cgltfReadVec3FromAccessor(pos_acc, vi);

                if (normal_acc) {
                    vert.normal = cgltfReadVec3FromAccessor(normal_acc, vi);
                } else {
                    vert.normal = vec3(0.f, 0.f, 1.f);
                }

                if (uv_acc) {
                    vert.uv = cgltfReadVec2FromAccessor(uv_acc, vi);
                } else {
                    vert.uv = vec2(0.f, 0.f);
                }

                if (tan_acc) {
                    vec4 tangent = cgltfReadVec4FromAccessor(tan_acc, vi);
                    vert.tangent = tangent;
                } else {
                    // @Todo
                    vert.tangent = vec4(1.f, 0.f, 0.f, 0.f);
                }

                aabb.Expand(vert.position);
            }

            // Fetch indices.
            //
            Array<u32> indices;
            if (prim->indices)
            {
                for (cgltf_size i = 0; i < prim->indices->count; ++i)
                {
                    indices.push_back((uint32)cgltf_accessor_read_index(prim->indices, i));
                }
            }
            else
            {
                // Make identity index buffer
                CORE_ASSERT(num_verts % 3 == 0);
                indices.resize(num_verts);
                for (uint32 i = 0; i < num_verts; ++i) 
                {
                    indices[i] = i;
                }
            }

            // Fetch material index.
            //
            u32 material_index = 0;
            if (prim->material)
            {
                for (cgltf_size mi = 0; mi < data->materials_count; ++mi)
                {
                    if (&data->materials[mi] == prim->material)
                    {
                        material_index = static_cast<u32>(mi);
                    }
                }
            }

            M->material_id = material_ids[material_index];
            M->vertices = std::move(verts);
            M->indices  = std::move(indices);
            M->aabb     = aabb;

            auto vb = GFX::AllocStructuredBuffer(gfx, M->vertices.data(), sizeof(M->vertices[0]), M->vertices.size());
            M->vertex_buffer = vb.first;
            M->vertex_buffer_descriptor = vb.second;

            M->index_buffer = GFX::AllocRawBuffer(gfx, M->indices.data(), sizeof(M->indices[0]) * M->indices.size());

            result->meshes.push_back(std::move(M));
        }

        if (node->has_matrix)
        {
            m4x4 m;
            {
                m._11 = node->matrix[0];
                m._21 = node->matrix[1];
                m._31 = node->matrix[2];
                m._41 = node->matrix[3];

                m._12 = node->matrix[4];
                m._22 = node->matrix[5];
                m._32 = node->matrix[6];
                m._42 = node->matrix[7];

                m._13 = node->matrix[8];
                m._23 = node->matrix[9];
                m._33 = node->matrix[10];
                m._43 = node->matrix[11];

                m._14 = node->matrix[12];
                m._24 = node->matrix[13];
                m._34 = node->matrix[14];
                m._44 = node->matrix[15];
            }
            result->local_transform = m;
        }
        else
        {
            vec3 translation = vec3(0, 0, 0);
            if (node->has_translation) {
                translation = vec3(node->translation[0], node->translation[1], node->translation[2]);
            }

            Quaternion rotation = Quaternion(1, 0, 0, 0);
            if (node->has_rotation) {
                rotation.x = node->rotation[0];
                rotation.y = node->rotation[1];
                rotation.z = node->rotation[2];
                rotation.w = node->rotation[3];
            }

            vec3 scale = vec3(1, 1, 1);
            if (node->has_scale) {
                scale = vec3(node->scale[0], node->scale[1], node->scale[2]);
            }

            result->local_transform = m4x4(Xform(translation, rotation, scale));
        }

        for (cgltf_size chi = 0; chi < node->children_count; ++chi)
        {
            cgltf_node* child_node =  node->children[chi];
            Scene_Component* child = cgltfParseNode(gfx, data, child_node, material_ids);
            result->children.push_back(child);
        }

        return result;
    }

    ENGINE_API Scene_Component* LoadGLTF(GFX::State* gfx, const String& path)
    {
        if (!gfx) {
            return nullptr;
        }

        cgltf_options opt = {};
        cgltf_data* data = nullptr;

        if (cgltf_parse_file(&opt, path.c_str(), &data) != cgltf_result_success) {
            Log("Failed to load gltf.");
            return nullptr;
        }

        if (cgltf_load_buffers(&opt, data, path.c_str()) != cgltf_result_success) {
            Log("Failed to load gltf buffer.");
            return nullptr;
        }

        uint num_scenes = (uint)data->scenes_count;
        if (num_scenes == 0) {
            return nullptr;
        }

        Scene_Component* result = new Scene_Component;


        // Material
        //
        String dir = FS::path(path).parent_path().string();
        Array<s32> material_ids(data->materials_count);
        for (cgltf_size mi = 0; mi < data->materials_count; ++mi)
        {
            cgltf_material* mat = &data->materials[mi];

            String albedo_path, normal_path, orm_path, emissive_path;

            auto GetTexturePath = [&](cgltf_texture_view& view) -> String
            {
                if (view.texture && view.texture->image && view.texture->image->uri)
                {
                    return (FS::path(dir) / view.texture->image->uri).lexically_normal().string();
                }
                return "";
            };

            if (mat->has_pbr_metallic_roughness)
            {
                albedo_path = GetTexturePath(mat->pbr_metallic_roughness.base_color_texture);
                orm_path    = GetTexturePath(mat->pbr_metallic_roughness.metallic_roughness_texture);
            }

            normal_path   = GetTexturePath(mat->normal_texture);
            emissive_path = GetTexturePath(mat->emissive_texture);

            nlohmann::json j;
            j["albedo"]   = albedo_path;
            j["orm"]      = orm_path;
            j["normal"]   = normal_path;
            j["emissive"] = emissive_path;

            auto CreateSRV = [&](String& path) -> s32
            {
                if (path == "")
                {
                    return -1;
                }
                
                int width, height;
                int forced_channels = 4;
                u8* data = stbi_load(path.c_str(), &width, &height, nullptr, forced_channels);
                u32 pitch = width * forced_channels;
                CORE_ASSERT(data);

                // @Temporary
                // @Temporary
                // @Temporary
                // @Temporary
                u16 mip_levels = 1;
                Texture tex = AllocTexture(gfx->device, {.width=(u32)width, .height=(u32)height, .depth=1, .mip_levels=mip_levels, .format=DXGI_FORMAT_R8G8B8A8_UNORM});
                Descriptor srv = AllocDescriptor(gfx->cbv_srv_uav_heap);

                {
                    u64 upload_buffer_size = GetRequiredIntermediateSize(gfx->device, tex);
                    auto upload_buffer = Malloc(gfx->device, {.size = upload_buffer_size, .heap_kind = RHI_HEAP_KIND_UPLOAD});

                    BeginCommandList(gfx->cmd_list);
                    {
                        // @Temporary: study copying subresourecs...
                        D3D12_SUBRESOURCE_DATA subresource_data = {
                            .pData      = data,
                            .RowPitch   = pitch,
                            .SlicePitch = pitch * height
                        };
                        UpdateSubresources(gfx->cmd_list->m_list, tex.m_resource, upload_buffer.m_resource, 0, 0, 1, &subresource_data);
                        CmdTransitionBarrier(gfx->cmd_list, tex.m_resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
                    }
                    EndCommandList(gfx->cmd_list);
                    ExecuteCommandList(gfx->cmd_queue, gfx->cmd_list);

                    PlaceFence(gfx->cmd_queue, gfx->fence);
                    WaitForFence(gfx->fence);

                    Free(upload_buffer);

                    // Create a view.
                    //
                    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
                        .Format = tex.m_desc.format,
                        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                        .Texture2D = {
                            .MostDetailedMip = 0,
                            .MipLevels = mip_levels,
                            .PlaneSlice = 0,
                            .ResourceMinLODClamp = 0.f
                        }
                    };
                    gfx->device->m_device->CreateShaderResourceView(tex.m_resource, &srv_desc, srv.cpu_handle);
                }
                // @Temporary
                // @Temporary
                // @Temporary
                // @Temporary


                stbi_image_free(data);

                return srv.m_index;
            };

            auto CreateSampler = [&]() -> s32
            {
                Descriptor sampler = AllocDescriptor(gfx->sampler_heap);
                {
                    D3D12_SAMPLER_DESC sampler_desc = {
                        .Filter         = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
                        .AddressU       = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                        .AddressV       = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                        .AddressW       = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                        .MipLODBias     = 0.0f,
                        .MaxAnisotropy  = 1,
                        .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
                        .BorderColor    = {0.0f, 0.0f, 0.0f, 0.0f},
                        .MinLOD         = 0.0f,
                        .MaxLOD         = D3D12_FLOAT32_MAX
                    };
                    gfx->device->m_device->CreateSampler(&sampler_desc, sampler.cpu_handle);
                }
                return sampler.m_index;
            };

            Material material = {
                .albedo   = CreateSRV(albedo_path),
                .normal   = CreateSRV(normal_path),
                .orm      = CreateSRV(orm_path),
                .emissive = CreateSRV(emissive_path),
                .sampler  = CreateSampler()
            };

            auto buf_and_desc = GFX::AllocStructuredBuffer(gfx, &material, sizeof(material), 1);
            s32 mat_id = static_cast<s32>(buf_and_desc.second.m_index);
            material_ids[mi] = mat_id;
            result->materials.push_back(mat_id);
        }


        // Scenes
        //
        for (uint sci = 0; sci < num_scenes; ++sci) 
        {
            cgltf_scene* scene = &data->scenes[sci];

            uint num_nodes = (uint)scene->nodes_count;
            for (uint ni = 0; ni < num_nodes; ++ni) 
            {
                cgltf_node* node = scene->nodes[ni];
                Scene_Component* child = cgltfParseNode(gfx, data, node, material_ids);
                result->children.push_back(child);
            }
        }

        cgltf_free(data);

        return result;
    }
}
