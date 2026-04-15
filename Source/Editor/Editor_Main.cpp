// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core/Core_Array.h"

#include "Window/Window.h"
#include "OS/OS_Main.h"
#include "IO/IO.h"
#include "Asset/Asset.h"

#include "RHI/DX12/RHI_DX12.h"
#include "Shader/DXIL/DXIL_Compiler.h"

#include "ThirdParty/DirectX/Include/d3d12.h"
#include "ThirdParty/DirectX/Include/d3dx12/d3dx12.h"
#include "ThirdParty/DXC/Include/d3d12shader.h"


using namespace Engine;
using namespace DXIL;

int ENGINE_MAIN(int argc, const char** argv)
{
    (void)argc;
    (void)argv;

    Window* window = Window::Create("Hello", 1920, 1080);

    DX12_Device* device = new DX12_Device;
    DX12_InitDevice(device, true);

    DX12_CommandQueue* cmd_queue = new DX12_CommandQueue;
    DX12_InitCommandQueue(device, cmd_queue);

    DX12_CommandList* cmd_list = new DX12_CommandList;
    DX12_InitCommandList(device, cmd_list, D3D12_COMMAND_LIST_TYPE_DIRECT);

    DX12_DescriptorHeap* rtv_heap = new DX12_DescriptorHeap;
    DX12_InitDescriptorHeap(device, rtv_heap, 53, RHI_DESCRIPTOR_KIND_RTV);

    DX12_DescriptorHeap* cbv_srv_uav_heap = new DX12_DescriptorHeap;
    DX12_InitDescriptorHeap(device, cbv_srv_uav_heap, 256, Engine::RHI_DESCRIPTOR_KIND_CBV_SRV_UAV);

    DX12_DescriptorHeap* sampler_heap = new DX12_DescriptorHeap;
    DX12_InitDescriptorHeap(device, sampler_heap, 256, Engine::RHI_DESCRIPTOR_KIND_SAMPLER);

    HWND hwnd = (HWND)window->GetPlatformWindow();
    uint width  = 1920;
    uint height = 1080;
    uint num_frames = 3;
    DX12_SwapChain* swap_chain = new DX12_SwapChain;
    DX12_InitSwapChain(device, swap_chain, rtv_heap, cmd_queue, hwnd, width, height, num_frames);

    DX12_Fence* fence = new DX12_Fence;
    DX12_InitFence(device, fence);


    Compiler* compiler = new Compiler;
    InitCompiler(compiler);

    bool is_debug = false;
#if BUILD_DEBUG
    is_debug = true;
#endif

    // Load HLSL asset.
    //
    u8* hlsl = nullptr;
    u64 hlsl_sz = 0;
    {
        const String hlsl_path = "C:/dev/swl/Untitled/Data/Shader/HLSL/Triangle.hlsl"; // @Temporary
        hlsl_sz = IO::ReadEntireFile(hlsl_path, nullptr);
        hlsl = new u8[hlsl_sz];
        IO::ReadEntireFile(hlsl_path, hlsl);
    }

    // @Todo: How should I deal with 'entry point' and 'target profile'?
    // The general pipeline follows:
    //      Artist writes shader with arbitrary entry point names in it.
    //      -> The engine must know the name of the entry point to compile the shader.
    //
    //      So, I guess I got two options:
    //
    //          1. Artists feed compiler the string manually.
    //          2. Make sort of a wrapper shader language that has builtin entry point names.
    //
    //          Both will go through DXC anyway, so to lighten the load of artrists, the later is more reasonable???
    //
    CompiledShader vs_module = CompileShader(compiler, is_debug, hlsl, hlsl_sz, "VS_Main", "vs_6_6");
    CompiledShader ps_module = CompileShader(compiler, is_debug, hlsl, hlsl_sz, "PS_Main", "ps_6_6");

    


    // @Temporary
    // @Temporary
    // @Temporary
    // @Temporary
    // @Temporary
    //
    DX12_Pipeline* pipeline = new DX12_Pipeline;

    {
        D3D12_SHADER_DESC shader_desc;
        auto r = ps_module.reflection;
        r->GetDesc(&shader_desc);

        // I need to gather root parameters to create a root signature.
        Array<D3D12_ROOT_PARAMETER1> root_parameters;

        // Create input element layout.
        //
        uint num_input_parameters = shader_desc.InputParameters;
        Array<D3D12_INPUT_ELEMENT_DESC> input_element_descs(num_input_parameters);

        for (uint i = 0; i < num_input_parameters; ++i)
        {
            D3D12_SIGNATURE_PARAMETER_DESC desc;
            r->GetInputParameterDesc(i, &desc);

            D3D12_INPUT_ELEMENT_DESC input_element_desc = {
                .SemanticName      = desc.SemanticName,
                .SemanticIndex     = desc.SemanticIndex,
                .Format            = ToDXGIFormat(desc.ComponentType, desc.Mask),
                .InputSlot         = 0u, // @Todo: wtf is this.
                .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass    = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,

                // @Todo: This might be an issue when doing instanced rendering.
                .InstanceDataStepRate = 0
            };

            input_element_descs[i] = input_element_desc;
        }

        // Create shader bound resource state..
        //
        for (uint i = 0; i < shader_desc.BoundResources; ++i)
        {
            D3D12_SHADER_INPUT_BIND_DESC shader_input_bind_desc;
            r->GetResourceBindingDesc(i, &shader_input_bind_desc);

            const D3D_SHADER_INPUT_TYPE type = shader_input_bind_desc.Type;

            if (type == D3D_SIT_CBUFFER)
            {
                //ID3D12ShaderReflectionConstantBuffer* shader_reflection_constant_buffer = r->GetConstantBufferByIndex(i);
                //D3D12_SHADER_BUFFER_DESC desc;
                //shader_reflection_constant_buffer->GetDesc(&desc);

                D3D12_ROOT_PARAMETER1 root_param = {
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                    .Descriptor = {
                        .ShaderRegister = shader_input_bind_desc.BindPoint,
                        .RegisterSpace = shader_input_bind_desc.Space,
                        .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
                    },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                };

                root_parameters.push_back(root_param);
            } 
            else if (type == D3D_SIT_TEXTURE)
            {
            }
            else
            {
                CORE_ASSERT(!"Unhandled shader input type.");
            }
        }

        // Create PSO.
        DX12_InitPipeline(device, pipeline, &vs_module, &ps_module, device->m_global_root_signature, input_element_descs.data(), input_element_descs.size());
    }
    // @Temporary
    // @Temporary
    // @Temporary
    // @Temporary
    // @Temporary


    // @Temporary: Create vertex buffer and descriptor pointing it.
    //
    DX12_Buffer vertex_buffer;
    DX12_Descriptor vertex_buffer_descriptor = DX12_AllocDescriptor(cbv_srv_uav_heap);
    {
        // Create buffer.
        //
        Vertex vertices[] = {
            {{-0.5f, -0.5f,  0.0f}, { 0.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.0f}, { 1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.0f}, { 0.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.0f}, { 1.0f, 0.0f}}
        };
        u64 num_vertices = ARRAY_COUNT(vertices);
        u64 sz = sizeof(vertices);
        u64 stride = sizeof(vertices[0]);

        vertex_buffer = DX12_Malloc(device, {.size = sz, .heap_kind = RHI_HEAP_KIND_DEFAULT});

        u64 staging_buffer_size = GetRequiredIntermediateSize(vertex_buffer.m_resource, 0, 1);
        auto staging_buffer = DX12_Malloc(device, { .size = staging_buffer_size, .heap_kind = RHI_HEAP_KIND_UPLOAD });

        void* ptr = DX12_Map(staging_buffer);
        memcpy(ptr, vertices, sz);
        DX12_Unmap(staging_buffer);

        DX12_BeginCommandList(cmd_list);
        {
            auto prev1 = DX12_CMD_TransitionBarrier(cmd_list, &vertex_buffer, D3D12_RESOURCE_STATE_COPY_DEST);
            auto prev2 = DX12_CMD_TransitionBarrier(cmd_list, &staging_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
            DX12_CMD_Copy(cmd_list, vertex_buffer, staging_buffer, sz);
            DX12_CMD_TransitionBarrier(cmd_list, &vertex_buffer, prev1);
            DX12_CMD_TransitionBarrier(cmd_list, &staging_buffer, prev2);
        }
        DX12_EndCommandList(cmd_list);
        DX12_ExecuteCommandList(cmd_queue, cmd_list);

        DX12_PlaceFence(cmd_queue, fence);
        DX12_WaitForFence(fence);


        // Create srv.
        //
        CD3DX12_SHADER_RESOURCE_VIEW_DESC desc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer(num_vertices, stride);
        device->m_device->CreateShaderResourceView(vertex_buffer.m_resource, &desc, vertex_buffer_descriptor.cpu_handle);

        // Cleanup
        //
        DX12_Free(staging_buffer);
    }

    // Create index buffer.
    //
    DX12_Buffer index_buffer;
    {
        u32 indices[] = {
            0, 1, 2,
            3, 2, 1
        };
        u64 num_indices = ARRAY_COUNT(indices);
        u64 sz = sizeof(indices);

        index_buffer = DX12_Malloc(device, {.size = sz, .heap_kind = RHI_HEAP_KIND_DEFAULT});

        u64 staging_buffer_size = GetRequiredIntermediateSize(vertex_buffer.m_resource, 0, 1);
        auto staging_buffer = DX12_Malloc(device, { .size = staging_buffer_size, .heap_kind = RHI_HEAP_KIND_UPLOAD });

        void* ptr = DX12_Map(staging_buffer);
        memcpy(ptr, indices, sz);
        DX12_Unmap(staging_buffer);

        DX12_BeginCommandList(cmd_list);
        {
            auto prev1 = DX12_CMD_TransitionBarrier(cmd_list, &index_buffer, D3D12_RESOURCE_STATE_COPY_DEST);
            auto prev2 = DX12_CMD_TransitionBarrier(cmd_list, &staging_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
            DX12_CMD_Copy(cmd_list, index_buffer, staging_buffer, sz);
            DX12_CMD_TransitionBarrier(cmd_list, &index_buffer, prev1);
            DX12_CMD_TransitionBarrier(cmd_list, &staging_buffer, prev2);
        }
        DX12_EndCommandList(cmd_list);
        DX12_ExecuteCommandList(cmd_queue, cmd_list);

        DX12_PlaceFence(cmd_queue, fence);
        DX12_WaitForFence(fence);
    }

    // @Temporary: Create texture and view.
    //
    DX12_Texture tex = {};
    DX12_Descriptor tex_srv = DX12_AllocDescriptor(cbv_srv_uav_heap);
    u32 tex_width      = 1024;
    u32 tex_height     = 1024;
    u32 tex_pitch      = tex_width * 4;
    u16 tex_depth      = 1;
    u16 tex_mip_levels = 1;
    DXGI_FORMAT tex_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    u32* tex_data = new u32[tex_width * tex_height];
    for (u32 r = 0; r < tex_height; ++r) {
        for (u32 c = 0; c < tex_width; ++c) {
            u32* texel = &tex_data[r * tex_width + c];
            int x = 64;
            if ((r/x + c/x) % 2 == 0) {
                *texel = 0xff000000;
            } else {
                *texel = 0xffffffff;
            }
        }
    }

    {
        // Alloc buffer for texture in GPU.
        //
        tex = DX12_AllocTexture(device, {.width = tex_width, .height = tex_height, .depth = tex_depth, .mip_levels = tex_mip_levels, .format = tex_format});

        u64 upload_buffer_size = DX12_GetRequiredIntermediateSize(device, tex);
        auto upload_buffer = DX12_Malloc(device, {.size = upload_buffer_size, .heap_kind = RHI_HEAP_KIND_UPLOAD});

        DX12_BeginCommandList(cmd_list);
        {
            // @Temporary: study copying subresourecs...
            D3D12_SUBRESOURCE_DATA data = {
                .pData      = tex_data,
                .RowPitch   = tex_pitch,
                .SlicePitch = tex_pitch * tex_height
            };
            UpdateSubresources(cmd_list->m_list, tex.m_resource, upload_buffer.m_resource, 0, 0, 1, &data);
            DX12_CMD_TransitionBarrier(cmd_list, tex.m_resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
        DX12_EndCommandList(cmd_list);
        DX12_ExecuteCommandList(cmd_queue, cmd_list);

        DX12_PlaceFence(cmd_queue, fence);
        DX12_WaitForFence(fence);

        DX12_Free(upload_buffer);


        // Create a view.
        //
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
            .Format = tex.m_desc.format,
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D = {
                .MostDetailedMip = 0,
                .MipLevels = tex_mip_levels,
                .PlaneSlice = 0,
                .ResourceMinLODClamp = 0.f
            }
        };
        device->m_device->CreateShaderResourceView(tex.m_resource, &srv_desc, tex_srv.cpu_handle);
    }

    // @Temporary: Create a sampler.
    //
    DX12_Descriptor sampler = DX12_AllocDescriptor(sampler_heap);
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
        device->m_device->CreateSampler(&sampler_desc, sampler.cpu_handle);
    }




    // @Todo: Is there a way to reflect PushConstant from shader to CPP side?
    //
    struct PushConstant
    {
        u32 vertex_buffer_index;
        u32 texture_index;
        u32 sampler_index;
    };
    PushConstant bindless_buffer = {
        .vertex_buffer_index = (u32)vertex_buffer_descriptor.m_index,
        .texture_index       = (u32)tex_srv.m_index,
        .sampler_index       = (u32)sampler.m_index,
    };


    while (window->IsOpen()) 
    {
        window->PollEvents();

        DX12_BeginCommandList(cmd_list);
        {
            // @Temporary: Update current frame index!
            DX12_CMD_TransitionBarrier(cmd_list, swap_chain->m_resources[swap_chain->current_frame_index], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            DX12_CMD_SetRenderTarget(cmd_list, swap_chain->m_descriptors[swap_chain->current_frame_index]);
            DX12_CMD_ClearRTV(cmd_list, swap_chain->m_descriptors[swap_chain->current_frame_index], 0.0f, 0.2f, 0.4f, 1.0f);

            {
                DX12_CMD_SetViewport(cmd_list, 0, 0, width, height);
                DX12_CMD_SetScissor(cmd_list, 0, 0, width, height);

                ID3D12DescriptorHeap* heaps[] = {
                    cbv_srv_uav_heap->m_descriptor_heap,
                    sampler_heap->m_descriptor_heap
                };
                cmd_list->m_list->SetDescriptorHeaps(ARRAY_COUNT(heaps), heaps);
                cmd_list->m_list->SetGraphicsRootSignature(device->m_global_root_signature);

                DX12_CMD_SetGraphicsConstants(cmd_list, &bindless_buffer, sizeof(bindless_buffer));

                cmd_list->m_list->SetPipelineState(pipeline->m_pso);

                cmd_list->m_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                DX12_CMD_SetIndexBuffer(cmd_list, index_buffer);
                DX12_CMD_DrawIndexed(cmd_list, 6, 1); // @Temporary
            }

            DX12_CMD_TransitionBarrier(cmd_list, swap_chain->m_resources[swap_chain->current_frame_index], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        }
        DX12_EndCommandList(cmd_list);
        DX12_ExecuteCommandList(cmd_queue, cmd_list);

        DX12_PlaceFence(cmd_queue, fence);
        DX12_Present(swap_chain);
        DX12_WaitForFence(fence);

        swap_chain->current_frame_index = swap_chain->m_swap_chain->GetCurrentBackBufferIndex();
    }

    // Cleanup
    //
    Window::Destroy(window);

    DX12_DeinitPipeline(pipeline);

    DeinitCompiler(compiler);

    DX12_DeinitFence(fence);
    DX12_DeinitSwapChain(swap_chain);
    DX12_DeinitDescriptorHeap(cbv_srv_uav_heap);
    DX12_DeinitDescriptorHeap(rtv_heap);
    DX12_DeinitCommandList(cmd_list);
    DX12_DeinitCommandQueue(cmd_queue);
    DX12_DeinitDevice(device);

    return 0;
}
