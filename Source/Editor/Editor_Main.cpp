// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core/SE_Basics.h"
#include "Core/SE_String.h"
#include "Window/Window.h"
#include "DX12.h"

using namespace Engine;

int main()
{
    String title = "This is a window";
    int window_width  = 1920;
    int window_height = 1080;

    Window* window = create_window(title, window_width, window_height);

    auto* device    = dx12_create_device();
    auto* cmd_queue = dx12_create_command_queue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto* cmd_list  = dx12_create_command_list(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto* fence     = dx12_create_fence(device);

    auto* rtv_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,          64);
    auto* dsv_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV,          64);
    auto* scu_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256);
    auto* sampler_heap = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,      64);

    u32 tex_width = 1920, tex_height = 1080;

    ID3D12Resource* color_resource = nullptr;
    {
        u32 num_samples = 1;
        u32 alignment = 0;
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
        D3D12_HEAP_PROPERTIES heap_prop = {
            .Type                 = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask     = 1,
            .VisibleNodeMask      = 1
        };
        D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
        D3D12_RESOURCE_FLAGS resource_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_RESOURCE_DESC desc = {
            .Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment        = alignment,
            .Width            = tex_width,
            .Height           = tex_height,
            .DepthOrArraySize = 1,
            .MipLevels        = 1,
            .Format           = format,
            .SampleDesc       = {
                .Count   = num_samples,
                .Quality = 0
            },
            .Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags            = resource_flags,
        };
        D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_COMMON;
        D3D12_CLEAR_VALUE clear_value = {
            .Format = format,
            .Color  = { 1.0f, 0.2f, 1.0f, 1.0f }
        };

        if (FAILED(device->native_device->CreateCommittedResource(&heap_prop, heap_flags, &desc, init_state, &clear_value, IID_PPV_ARGS(&color_resource)))) {
            assert(0);
        }
    }

    ID3D12Resource* depth_resource = nullptr;
    {
        u32 num_samples = 1;
        u32 alignment = 0;
        DXGI_FORMAT format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        D3D12_HEAP_PROPERTIES heap_prop = {
            .Type                 = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask     = 1,
            .VisibleNodeMask      = 1
        };
        D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;
        D3D12_RESOURCE_FLAGS resource_flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_RESOURCE_DESC desc = {
            .Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment        = alignment,
            .Width            = tex_width,
            .Height           = tex_height,
            .DepthOrArraySize = 1,
            .MipLevels        = 1,
            .Format           = format,
            .SampleDesc       = {
                .Count   = num_samples,
                .Quality = 0
            },
            .Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags            = resource_flags,
        };
        D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_COMMON;
        D3D12_CLEAR_VALUE clear_value = {
            .Format = format,
            .DepthStencil = {
                .Depth = 1.0f, .Stencil = 0u
            }
        };

        if (FAILED(device->native_device->CreateCommittedResource(&heap_prop, heap_flags, &desc, init_state, &clear_value, IID_PPV_ARGS(&depth_resource)))) {
            assert(0);
        }
    }



    while (window->is_running) {
        while (window->poll_events()) {}
    }

    // Cleanups
    //
    depth_resource->Release();
    color_resource->Release();

    dx12_destroy_descriptor_heap(rtv_heap);
    dx12_destroy_descriptor_heap(dsv_heap);
    dx12_destroy_descriptor_heap(scu_heap);
    dx12_destroy_descriptor_heap(sampler_heap);
    dx12_destroy_fence(fence);
    dx12_destroy_command_list(cmd_list);
    dx12_destroy_command_queue(cmd_queue);
    dx12_destroy_device(device);

    destroy_window(window);

    return 0;
}
