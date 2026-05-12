// Copyright Seong Woo Lee. All Rights Reserved.

#include "Renderer.h"

namespace Engine
{
    void IPass::begin(DX12_Command_List* cmd_list)
    {
        // Set shader.
        cmd_list->set_pipeline_state(pipeline_state);

        // Transition resources.
        for (auto* res : inputs) {
            cmd_list->transition_barrier(res->resource, 0, D3D12_RESOURCE_STATE_COMMON);
        }
        for (auto* res : outputs) {
            cmd_list->transition_barrier(res->resource, 0, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
        if (depth_target) {
            cmd_list->transition_barrier(depth_target->resource, 0, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        }

        // Set render targets.
        Array <DX12_Descriptor> rtvs;
        for (auto* res : outputs) {
            rtvs.push_back(res->rtv);
        }
        DX12_Descriptor* dsv = depth_target ? &depth_target->dsv : nullptr;
        cmd_list->set_render_target(rtvs.size(), rtvs.data(), dsv);

        // Set viewport, scissor and topology.
        cmd_list->set_viewport(0, 0, viewport_width, viewport_height);
        cmd_list->set_scissor(0, 0, scissor_width, scissor_height);
        cmd_list->set_topology(topology);
    }

    ENGINE_API Pass_Resource* create_pass_resource(DX12_Device* device, DX12_Descriptor_Heap* srv_heap, DX12_Descriptor_Heap* rtv_heap, DX12_Descriptor_Heap* dsv_heap, 
                                                   DXGI_FORMAT format, u32 width, u32 height) 
    {
        Pass_Resource* result = new Pass_Resource;

        D3D12_RESOURCE_FLAGS flags = dsv_heap ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        DX12_Resource_Desc desc = { .type = DX12_RESOURCE_TYPE_TEXTURE_2D, 
            .resource_flags = flags,
            .texture = { .format = format, .width = width, .height = height,
                .mip_levels = 1, .depth = 1, .num_samples = 1 }, };
        if (dsv_heap) {
            desc.do_clear = true;
            desc.clear_value = {
                .Format = format,
                .DepthStencil = { .Depth = 1.0f, .Stencil = 0u }
            };
        }
        result->resource = dx12_alloc_resource(device, desc);

        if (srv_heap) {
            result->srv = dx12_alloc_descriptor(srv_heap);
            dx12_create_srv(device, result->resource, &result->srv, result->resource->desc.texture.format);
        }

        if (rtv_heap) {
            result->rtv = dx12_alloc_descriptor(rtv_heap);
            dx12_create_rtv(device, result->resource, &result->rtv, { .Format = format, .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D, .Texture2D = { .MipSlice = 0, .PlaneSlice = 0 } });
        }

        if (dsv_heap) {
            result->dsv = dx12_alloc_descriptor(dsv_heap);
            dx12_create_dsv(device, result->resource, &result->dsv, format);
        }

        return result;
    }
}
