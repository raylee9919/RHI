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

    struct ENGINE_API Pass_Resource {
        String name; // Bound to resource table in resource state.
        u16 flags;

        DX12_Descriptor srv;
        DX12_Descriptor uav;
        DX12_Descriptor rtv;
        DX12_Descriptor dsv;

        enum Flags : u16 {
            FLAG_HAS_SRV = 0x1,
            FLAG_HAS_UAV = 0x2,
            FLAG_HAS_RTV = 0x4,
            FLAG_HAS_DSV = 0x8,
        };

        FORCE_INLINE bool has_srv() { return flags & FLAG_HAS_SRV; }
        FORCE_INLINE bool has_uav() { return flags & FLAG_HAS_UAV; }
        FORCE_INLINE bool has_rtv() { return flags & FLAG_HAS_RTV; }
        FORCE_INLINE bool has_dsv() { return flags & FLAG_HAS_DSV; }
        FORCE_INLINE u32 get_srv_index() { CORE_ASSERT(has_srv()); return srv.index; }
        FORCE_INLINE u32 get_urv_index() { CORE_ASSERT(has_uav()); return uav.index; }
        FORCE_INLINE u32 get_rtv_index() { CORE_ASSERT(has_rtv()); return rtv.index; }
        FORCE_INLINE u32 get_dsv_index() { CORE_ASSERT(has_dsv()); return dsv.index; }
    };

    struct ENGINE_API Resource_State {
        // @Cleanup
        public:
            Hash_Table <String, Material> materials;
            Hash_Table <String, Mesh_Resource> meshes;

        private:
            Hash_Table <String, DX12_Resource*> resource_table;
            Hash_Table <String, Pass_Resource>  pass_resource_table;


        public:
            void clear();

            FORCE_INLINE void alloc_material(Material m) { materials[m.name] = m; }

            void alloc_resource(const String& name, DX12_Device* device, DX12_Resource_Desc);
            DX12_Resource* get_resource(const String& name);

            void alloc_pass_resource(const String& name, DX12_Device* device, DX12_Descriptor_Heap* srv_heap, DX12_Descriptor_Heap* uav_heap, DX12_Descriptor_Heap* rtv_heap, DX12_Descriptor_Heap* dsv_heap);
            Pass_Resource get_pass_resource(const String& name);
    };

    struct ENGINE_API IPass {
        DX12_Pipeline_State* pipeline_state = nullptr;

        Array <String> inputs;
        Array <String> outputs;
        String depth_target;

        u32 viewport_width  = 0;
        u32 viewport_height = 0;
        u32 scissor_width   = 0;
        u32 scissor_height  = 0;

        D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        FORCE_INLINE bool has_depth() { return depth_target != ""; } // @Robustness
        void push_input(Resource_State* resource_state, const String& name);
        void begin(Resource_State* resource_state, DX12_Command_List* cmd_list);
    };

    ENGINE_API Pass_Resource make_pass_resource(Resource_State* resource_state, const String& resource_name, DX12_Device* device, DX12_Descriptor_Heap* srv_heap, DX12_Descriptor_Heap* uav_heap, DX12_Descriptor_Heap* rtv_heap, DX12_Descriptor_Heap* dsv_heap);
}
