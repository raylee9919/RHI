// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core/SE_Basics.h"
#include "Core/SE_String.h"
#include "Core/SE_Math.h"
#include "Window/Window.h"
#include "File/FileSystem.h"
#include "DX12.h"
#include "Shader/DXIL/DXIL_Compiler.h"

using namespace Engine;
using namespace DXIL;

int main()
{
    file_sys::path project_dir = "C:/dev/swl/RHI/Project";
    file_sys::path asset_dir   = project_dir / "Asset";

    String title = "This is a window";
    int window_width  = 1920;
    int window_height = 1080;

    Window* window = create_window(title, window_width, window_height);

    Shader_Compiler* compiler = new Shader_Compiler;
    init_shader_compiler(compiler);

    auto* device    = dx12_create_device();
    auto* cmd_queue = dx12_create_command_queue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto* cmd_list  = dx12_create_command_list(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto* fence     = dx12_create_fence(device);

    auto* rtv_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,          32);
    auto* dsv_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV,          32);
    auto* res_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512);
    auto* sampler_heap = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,      32);

    u32 tex_width  = 1920;
    u32 tex_height = 1080;
    u32 num_frames = 3;
    auto* swap_chain = dx12_create_swap_chain(device, cmd_queue, rtv_heap, (HWND)window->get_platform_window(), tex_width, tex_height, num_frames);

    DXGI_FORMAT rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT dsv_format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    auto* color_texture_resource = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D,
                                                       .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                       .texture = { .format = rtv_format,
                                                       .width = tex_width, .height = tex_height,
                                                       .mip_levels = 1, .depth = 1, .num_samples = 1 } });

    auto* depth_texture_resource = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D,
                                                       .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                                                       .do_clear = true,
                                                       .clear_value = { .Format = dsv_format, .DepthStencil = { .Depth = 1.0f, .Stencil = 0u }},
                                                       .texture = { .format = dsv_format,
                                                       .width = tex_width, .height = tex_height,
                                                       .mip_levels = 1, .depth = 1, .num_samples = 1 } });

    auto* bindless_root_signature = dx12_create_bindless_root_signature(device);

    auto* pso = new DX12_Pipeline_State;
    {
        // Reflection
        String path = (asset_dir / "Shader/HLSL/Triangle.shader").string();
        u64 length = read_entire_file(path, nullptr);
        u8* source = new u8[length];
        read_entire_file(path, source);

        auto compiled_vs = compiler->compile(true, source, length, "VS_Main", "vs_6_6");
        auto vs_reflection = compiler->reflect(compiled_vs.result);
        D3D12_INPUT_ELEMENT_DESC* vs_input_elements = new D3D12_INPUT_ELEMENT_DESC[vs_reflection.get_num_input_parameters()];
        vs_reflection.get_input_parameters(vs_input_elements);

        auto compiled_ps = compiler->compile(true, source, length, "PS_Main", "ps_6_6");


        // PSO creation
        DX12_Graphics_Pipeline_Desc pso_desc = {
            .root_signature     = bindless_root_signature,

            .num_input_elements = vs_reflection.get_num_input_parameters(),
            .input_elements     = vs_input_elements,

            .topology           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,

            .cull_mode          = D3D12_CULL_MODE_BACK,

            .depth_enabled = true,
            .num_render_targets = 1,
            .rtv_formats = {
                rtv_format
            },
            .dsv_format  = dsv_format,

            .vs_bytecode = compiled_vs.bytecode,
            .vs_length   = compiled_vs.length, 

            .ps_bytecode = compiled_ps.bytecode,
            .ps_length   = compiled_ps.length, 
        };
        *pso = dx12_create_graphics_pipeline_state(device, pso_desc);


        // Cleanup
        delete [] vs_input_elements;
        vs_reflection.release();
        compiled_vs.release();

        compiled_ps.release();

        delete [] source;
    }


    // Create DSV
    auto dsv = dx12_alloc_descriptor(dsv_heap);
    dx12_create_dsv(device, depth_texture_resource, &dsv, dsv_format);

    // Create vertex buffer
    vec3 vertices[] = {
        vec3(-0.5f, -0.5f, 0.0f),
        vec3( 0.5f, -0.5f, 0.0f),
        vec3(-0.5f,  0.5f, 0.0f),
        vec3( 0.5f,  0.5f, 0.0f),
    };
    u64 vertices_size = sizeof(vertices);
    const u32 num_vertices = count_of(vertices);
    const u32 vert_stride  = sizeof(vec3);

    auto* vertex_buffer = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_BUFFER, .heap_type = D3D12_HEAP_TYPE_DEFAULT, .buffer = { .size = vertices_size } });
    dx12_upload_buffer(device, cmd_queue, cmd_list, fence, vertex_buffer, vertices, vertices_size);

    DX12_Descriptor vertex_buffer_descriptor = dx12_alloc_descriptor(res_heap);
    dx12_create_srv(device, vertex_buffer, &vertex_buffer_descriptor, num_vertices, vert_stride);

    // Create index buffer
    u32 indices[] = {
        0, 1, 2,
        2, 1, 3
    };
    u32 num_indices = count_of(indices);
    u64 indices_size = sizeof(indices);

    auto* index_buffer = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_BUFFER, .heap_type = D3D12_HEAP_TYPE_DEFAULT, .buffer = { .size = indices_size } });
    dx12_upload_buffer(device, cmd_queue, cmd_list, fence, index_buffer, indices, indices_size);


    // Bindless push constants.
    struct Push_Constant {
        u32 vertex_buffer_id;
    } push_constants = {
        .vertex_buffer_id = vertex_buffer_descriptor.index
    };


    // Main loop
    //
    while (window->is_running) {
        while (window->poll_events()) {}

        cmd_list->begin();
        {
            cmd_list->set_viewport(0, 0, tex_width, tex_height);
            cmd_list->set_scissor(0, 0, tex_width, tex_height);

            cmd_list->transition_barrier(swap_chain->get_current_resource(), 0, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmd_list->clear_rtv(swap_chain->get_current_rtv(), 0.4f, 0.2f, 0.2f, 1.0f);

            cmd_list->transition_barrier(depth_texture_resource->native_resource, 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            cmd_list->clear_dsv(&dsv, 1.0f, 0, 0, tex_width, tex_height);

            cmd_list->set_resource_and_sampler_heap(res_heap, sampler_heap);

            cmd_list->set_graphics_root_signature(bindless_root_signature);
            cmd_list->set_pipeline_state(pso);

            cmd_list->set_graphics_root_constants(0u, sizeof(Push_Constant) / 4, &push_constants);

            cmd_list->set_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            cmd_list->set_index_buffer(index_buffer);

            DX12_Descriptor *rtvs[] = { swap_chain->get_current_rtv() };
            cmd_list->set_render_target(1, rtvs, &dsv);

            cmd_list->draw_indexed(num_indices, 1);

            cmd_list->transition_barrier(swap_chain->get_current_resource(), 0, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            cmd_list->transition_barrier(depth_texture_resource->native_resource, 0, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON);
        }
        cmd_list->end();

        dx12_execute_command_list(cmd_queue, cmd_list);

        dx12_signal_fence(cmd_queue, fence);
        dx12_wait_fence(fence);

        swap_chain->present();
    }

    // Cleanups
    //
    dx12_dealloc_resource(vertex_buffer);
    dx12_dealloc_resource(index_buffer);

    pso->release();

    dx12_signal_fence(cmd_queue, fence);
    dx12_wait_fence(fence);

    bindless_root_signature->Release();

    dx12_dealloc_resource(color_texture_resource);
    dx12_dealloc_resource(depth_texture_resource);

    dx12_destroy_swap_chain(swap_chain);
    dx12_destroy_descriptor_heap(rtv_heap);
    dx12_destroy_descriptor_heap(dsv_heap);
    dx12_destroy_descriptor_heap(res_heap);
    dx12_destroy_descriptor_heap(sampler_heap);
    dx12_destroy_fence(fence);
    dx12_destroy_command_list(cmd_list);
    dx12_destroy_command_queue(cmd_queue);
    dx12_destroy_device(device);

    deinit_shader_compiler(compiler);

    destroy_window(window);

    return 0;
}
