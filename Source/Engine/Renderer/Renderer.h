// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Math.h"
#include "RHI/DX12/DX12.h"
#include "Asset/SE_Asset.h"

namespace Engine
{
    struct Index_Buffer {
        DX12_Resource* resource;
        u32 num_indices;
    };

    struct Mesh_Slice_Resource {
        DX12_Resource*  vertex_buffer;
        DX12_Descriptor vertex_buffer_descriptor;
        u32 num_vertices;

        Index_Buffer index_buffer;

        String material_name;
    };

    struct Mesh_Resource {
        String name;
        Array <Mesh_Slice_Resource> slices;
    };

    struct Resource_State {
        Hash_Table <String, Material> materials;
        Hash_Table <String, Mesh_Resource> meshes;

        void alloc_material(Material m) { materials[m.name] = m; }
        void clear()
        {
            Array <String> material_names;
            for (auto& it : materials) {
                dx12_dealloc_resource(it.second.resource);
                dx12_dealloc_descriptor(it.second.srv);
                for (auto* tex : it.second.resources) {
                    dx12_dealloc_resource(tex);
                }
                material_names.push_back(it.first);
            }
            for (String& name : material_names) { materials.erase(name); }


            Array <String> mesh_names;
            for (auto& mesh : meshes) {
                for (auto& it : mesh.second.slices) {
                    dx12_dealloc_resource(it.vertex_buffer);
                    dx12_dealloc_descriptor(it.vertex_buffer_descriptor);
                    dx12_dealloc_resource(it.index_buffer.resource);
                }
                mesh_names.push_back(mesh.first);
            }
            for (String& name : mesh_names) { meshes.erase(name); }
        }
    };


    struct ENGINE_API Pass_Resource {
        DX12_Resource* resource;
        DX12_Descriptor srv;
        DX12_Descriptor rtv;
        DX12_Descriptor dsv;

        FORCE_INLINE u32 get_srv_index() { return srv.index; }
    };

    struct ENGINE_API IPass {
        DX12_Pipeline_State* pipeline_state = nullptr;

        Array <Pass_Resource*> inputs;
        Array <Pass_Resource*> outputs;
        Pass_Resource* depth_target = nullptr;

        u32 viewport_width = 0;
        u32 viewport_height = 0;
        u32 scissor_width = 0;
        u32 scissor_height = 0;

        D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        void begin(DX12_Command_List* cmd_list);
        virtual void draw(DX12_Command_List* cmd_list, void* param) = 0;
    };

    ENGINE_API Pass_Resource* create_pass_resource(DX12_Device* device, DX12_Descriptor_Heap* srv_heap, DX12_Descriptor_Heap* rtv_heap, DX12_Descriptor_Heap* dsv_heap, DXGI_FORMAT format, u32 width, u32 height);
}
