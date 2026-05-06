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

    auto* rtv_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,          32);
    auto* dsv_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV,          32);
    auto* scu_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512);
    auto* sampler_heap = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,      32);

    HWND hwnd = (HWND)window->get_platform_window();
    u32 tex_width = 1920;
    u32 tex_height = 1080;
    u32 num_frames = 3;
    auto* swap_chain = dx12_create_swap_chain(device, cmd_queue, rtv_heap, hwnd, tex_width, tex_height, num_frames);

    auto* color_texture_resource = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D,
                                                       .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                       .texture = { .format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                       .width = tex_width, .height = tex_height,
                                                       .mip_levels = 1, .depth = 1, .num_samples = 1 } });

    auto* depth_texture_resource = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D,
                                                       .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                                                       .texture = { .format = DXGI_FORMAT_D24_UNORM_S8_UINT,
                                                       .width = tex_width, .height = tex_height,
                                                       .mip_levels = 1, .depth = 1, .num_samples = 1 } });

    // Main loop
    //
    while (window->is_running) {
        while (window->poll_events()) {}

        cmd_list->begin();
        {
            cmd_list->transition_barrier(swap_chain->get_current_resource(), 0, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmd_list->clear_rtv(swap_chain->get_current_rtv(), 0.4f, 0.2f, 0.2f, 1.0f);
            cmd_list->transition_barrier(swap_chain->get_current_resource(), 0, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        }
        cmd_list->end();

        dx12_execute_command_list(cmd_queue, cmd_list);

        dx12_signal_fence(cmd_queue, fence);
        dx12_wait_fence(fence);

        swap_chain->present();
    }

    // Cleanups
    //
    dx12_signal_fence(cmd_queue, fence);
    dx12_wait_fence(fence);

    dx12_dealloc_resource(color_texture_resource);
    dx12_dealloc_resource(depth_texture_resource);

    dx12_destroy_swap_chain(swap_chain);
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
