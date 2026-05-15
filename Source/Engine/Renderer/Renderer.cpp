// Copyright Seong Woo Lee. All Rights Reserved.

#include "Renderer.h"

namespace Engine
{
    void Resource_State::clear()
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

    void Resource_State::alloc_resource(const String& name, DX12_Device* device, DX12_Resource_Desc desc)
    {
        if (!resource_table.contains(name)) {
            auto* resource = dx12_alloc_resource(device, desc);
            resource_table[name] = resource;
        } else {
            CORE_ASSERT(0, "Name already used.");
        }
    }

    void IPass::begin(Resource_State* resource_state, DX12_Command_List* cmd_list)
    {
        // Set shader.
        cmd_list->set_pipeline_state(pipeline_state);

        if (pipeline_state->type == DX12_PIPELINE_TYPE_GRAPHICS) {
            // Transition resources.
            {
                for (const String& name : inputs) {
                    cmd_list->transition_barrier(resource_state->get_resource(name), 0, D3D12_RESOURCE_STATE_COMMON);
                }
                for (const String& name : outputs) {
                    cmd_list->transition_barrier(resource_state->get_resource(name), 0, D3D12_RESOURCE_STATE_RENDER_TARGET);
                }
                if (has_depth()) {
                    cmd_list->transition_barrier(resource_state->get_resource(depth_target), 0, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                }
            }

            // Set RTV and DSV
            {
                Array <DX12_Descriptor> rtvs;
                for (const String& name : outputs) {
                    auto res = resource_state->get_pass_resource(name);
                    rtvs.push_back(res.rtv);
                }

                // @Cleanup
                DX12_Descriptor* pdsv = nullptr;
                if (has_depth()) { 
                    auto dsv = resource_state->get_pass_resource(depth_target);
                    pdsv = &dsv.dsv;
                }
                cmd_list->set_render_target(rtvs.size(), rtvs.data(), pdsv);

                if (pdsv) {
                    // @Temporary
                    cmd_list->clear_dsv(pdsv, 1.0f, 0, 0, viewport_width, viewport_height);
                }
            }

            // Set viewport, scissor and topology.
            {
                cmd_list->set_viewport(0, 0, viewport_width, viewport_height);
                cmd_list->set_scissor(0, 0, scissor_width, scissor_height);
                cmd_list->set_topology(topology);
            }
        } else if (pipeline_state->type == DX12_PIPELINE_TYPE_COMPUTE) {
            CORE_ASSERT(!"Unhandled pipeline type.");
        } else {
            CORE_ASSERT(!"Unhandled pipeline type.");
        }

    }

    DX12_Resource* Resource_State::get_resource(const String& name) {
        CORE_ASSERT(resource_table.contains(name));
        return resource_table[name];
    }

    void Resource_State::alloc_pass_resource(const String& name, DX12_Device* device, DX12_Descriptor_Heap* srv_heap, DX12_Descriptor_Heap* uav_heap, DX12_Descriptor_Heap* rtv_heap, DX12_Descriptor_Heap* dsv_heap)
    {
        Pass_Resource result = {};

        auto* resource = get_resource(name);
        CORE_ASSERT(resource);

        if (srv_heap) {
            result.flags |= Pass_Resource::FLAG_HAS_SRV;
            result.srv = dx12_alloc_descriptor(srv_heap);
            dx12_create_srv(device, resource, &result.srv, resource->desc.texture.format);
        }

        if (uav_heap) {
            result.flags |= Pass_Resource::FLAG_HAS_UAV;
            result.uav = dx12_alloc_descriptor(uav_heap);
            dx12_create_uav(device, resource, &result.uav, resource->desc.texture.format);
        }

        if (rtv_heap) {
            result.flags |= Pass_Resource::FLAG_HAS_RTV;
            result.rtv = dx12_alloc_descriptor(rtv_heap);
            dx12_create_rtv(device, resource, &result.rtv, { .Format = resource->desc.texture.format, .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D, .Texture2D = { .MipSlice = 0, .PlaneSlice = 0 } });
        }

        if (dsv_heap) {
            result.flags |= Pass_Resource::FLAG_HAS_DSV;
            result.dsv = dx12_alloc_descriptor(dsv_heap);
            dx12_create_dsv(device, resource, &result.dsv, resource->desc.texture.format);
        }

        CORE_ASSERT(!pass_resource_table.contains(name));
        pass_resource_table[name] = result;
    }

    Pass_Resource Resource_State::get_pass_resource(const String& name) {
        CORE_ASSERT(pass_resource_table.contains(name));
        return pass_resource_table[name];
    }
}
