// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core/SE_Array.h"

#include "Window/Window.h"
#include "OS/OS_Main.h"
#include "IO/IO.h"
#include "Asset/SE_Asset.h"

#include "RHI/DX12/RHI_DX12.h"
#include "GFX/gfx.h"
#include "Shader/DXIL/DXIL_Compiler.h"
#include "Renderer/Renderer.h"

#include "ThirdParty/DirectX/Include/d3d12.h"
#include "ThirdParty/DirectX/Include/d3dx12/d3dx12.h"
#include "ThirdParty/DXC/Include/d3d12shader.h"


using namespace Engine;
using namespace DXIL;
using namespace DX12;
using namespace Render;


Array<D3D12_INPUT_ELEMENT_DESC> 
dx12ReflectInputParameters(ID3D12ShaderReflection* reflection)
{
    D3D12_SHADER_DESC desc;
    reflection->GetDesc(&desc);

    uint num_input_params = desc.InputParameters;
    Array <D3D12_INPUT_ELEMENT_DESC> input_element_descs(num_input_params);

    for (uint i = 0; i < num_input_params; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC desc;
        reflection->GetInputParameterDesc(i, &desc);

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

    return input_element_descs;
}

Intermediate_Pipeline_State
dx12CreateIntermediatePipelineState(DXIL::Compiler* compiler, String& path, bool is_debug)
{
    Intermediate_Pipeline_State result = {};

    u8* src = nullptr;
    u64 sz = 0;

    sz = IO::ReadEntireFile(path, nullptr);
    src = new u8[sz];
    IO::ReadEntireFile(path, src);

    CompiledShader vs_module = CompileShader(compiler, is_debug, src, sz, "VS_Main", "vs_6_6");
    CompiledShader ps_module = CompileShader(compiler, is_debug, src, sz, "PS_Main", "ps_6_6");

    result.vs_module        = vs_module;
    result.ps_module        = ps_module;
    result.input_parameters = dx12ReflectInputParameters(vs_module.reflection);

    return result;
}

int ENGINE_MAIN(int argc, const char** argv)
{
    (void)argc;
    (void)argv;

    Window* window = Window::Create("Hello", 1920, 1080);

    Device* device = new Device;
    InitDevice(device, true);

    CommandQueue* cmd_queue = new CommandQueue;
    InitCommandQueue(device, cmd_queue);

    CommandList* cmd_list = new CommandList;
    InitCommandList(device, cmd_list, D3D12_COMMAND_LIST_TYPE_DIRECT);

    DescriptorHeap* rtv_heap = new DescriptorHeap;
    InitDescriptorHeap(device, rtv_heap, 53, RHI_DESCRIPTOR_KIND_RTV);

    DescriptorHeap* cbv_srv_uav_heap = new DescriptorHeap;
    InitDescriptorHeap(device, cbv_srv_uav_heap, 256, RHI_DESCRIPTOR_KIND_CBV_SRV_UAV);

    DescriptorHeap* sampler_heap = new DescriptorHeap;
    InitDescriptorHeap(device, sampler_heap, 256, RHI_DESCRIPTOR_KIND_SAMPLER);

    HWND hwnd = (HWND)window->GetPlatformWindow();
    uint width  = 1920;
    uint height = 1080;
    uint num_frames = 3;
    SwapChain* swap_chain = new SwapChain;
    InitSwapChain(device, swap_chain, rtv_heap, cmd_queue, hwnd, width, height, num_frames);

    Fence* fence = new Fence;
    InitFence(device, fence);

    GFX::State* gfx_state = new GFX::State;
    GFX::Init(gfx_state, device, cmd_list, cmd_queue, fence, cbv_srv_uav_heap);

    Compiler* compiler = new Compiler;
    InitCompiler(compiler);

    bool is_debug = false;
#if BUILD_DEBUG
    is_debug = true;
#endif

    // @Temporary: Root
    //
    Scene_Component* root_component = new Scene_Component;

    // @Temporary: Camera
    //
    Camera* camera = new Camera;
    {
        vec3 position    = vec3(0.f, 3.f, 3.f);
        vec3 look_at     = vec3(0.f, 0.f, 0.f);
        f32 aspect_ratio = 9.f / 16.f;

        camera->view      = m4x4::LookAtLH(position, look_at, vec3(0.f, 1.f, 0.f));
        camera->proj      = m4x4::PerspectiveLH(DegreeToRadian(120), aspect_ratio, 0.1f, 1000.f);
        camera->view_proj = camera->proj * camera->view;
        camera->position  = vec4(position, 1.f);
    }

    // @Temporary: Load model
    //
    Scene_Component* sponza = LoadGLTF(gfx_state, "C:/dev/swl/Untitled/Data/Model/Cube/Cube.gltf");
    CORE_ASSERT(sponza);
    root_component->children.push_back(sponza);

    // @Temporary: Create PSO
    //
    Pipeline_State* pipeline_state = new Pipeline_State;
    {
        String path = "C:/dev/swl/Untitled/Data/Shader/HLSL/Triangle.hlsl";
        auto intermediate_pso = dx12CreateIntermediatePipelineState(compiler, path, is_debug);
        InitPipelineState(device, &intermediate_pso, pipeline_state);
    }

    // @Temporary: Create texture and view.
    //
    Texture tex = {};
    Descriptor tex_srv = AllocDescriptor(cbv_srv_uav_heap);
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
        tex = AllocTexture(device, {.width = tex_width, .height = tex_height, .depth = tex_depth, .mip_levels = tex_mip_levels, .format = tex_format});

        u64 upload_buffer_size = GetRequiredIntermediateSize(device, tex);
        auto upload_buffer = Malloc(device, {.size = upload_buffer_size, .heap_kind = RHI_HEAP_KIND_UPLOAD});

        BeginCommandList(cmd_list);
        {
            // @Temporary: study copying subresourecs...
            D3D12_SUBRESOURCE_DATA data = {
                .pData      = tex_data,
                .RowPitch   = tex_pitch,
                .SlicePitch = tex_pitch * tex_height
            };
            UpdateSubresources(cmd_list->m_list, tex.m_resource, upload_buffer.m_resource, 0, 0, 1, &data);
            CMD_TransitionBarrier(cmd_list, tex.m_resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
        EndCommandList(cmd_list);
        ExecuteCommandList(cmd_queue, cmd_list);

        PlaceFence(cmd_queue, fence);
        WaitForFence(fence);

        Free(upload_buffer);


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
    Descriptor sampler = AllocDescriptor(sampler_heap);
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

    Descriptor camera_desc = AllocDescriptor(cbv_srv_uav_heap);
    Buffer camera_buffer;
    {
        // Create buffer.
        //
        uint alignment = 256;
        uint size = sizeof(Camera);
        uint aligned_size = AlignUp(size, alignment);
        camera_buffer = Malloc(device, {.size = aligned_size, .heap_kind = RHI_HEAP_KIND_DEFAULT});
        camera_buffer.m_resource->SetName(L"Camera");

        // Create srv.
        //
        D3D12_CONSTANT_BUFFER_VIEW_DESC view_desc = {
            .BufferLocation = camera_buffer.m_gpu_address,
            .SizeInBytes    = aligned_size
        };
        device->m_device->CreateConstantBufferView(&view_desc, camera_desc.cpu_handle);
    }

    {
        uint size = sizeof(Camera);

        u64 staging_buffer_size = GetRequiredIntermediateSize(camera_buffer.m_resource, 0, 1);
        auto staging_buffer = Malloc(device, { .size = staging_buffer_size, .heap_kind = RHI_HEAP_KIND_UPLOAD });

        void* ptr = Map(staging_buffer);
        memcpy(ptr, camera, size);
        Unmap(staging_buffer);

        BeginCommandList(cmd_list);
        {
            auto prev1 = CMD_TransitionBarrier(cmd_list, &camera_buffer, D3D12_RESOURCE_STATE_COPY_DEST);
            auto prev2 = CMD_TransitionBarrier(cmd_list, &staging_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
            CMD_Copy(cmd_list, camera_buffer, staging_buffer, size);
            CMD_TransitionBarrier(cmd_list, &camera_buffer, prev1);
            CMD_TransitionBarrier(cmd_list, &staging_buffer, prev2);
        }
        EndCommandList(cmd_list);
        ExecuteCommandList(cmd_queue, cmd_list);

        PlaceFence(cmd_queue, fence);
        WaitForFence(fence);

        Free(staging_buffer);
    }




    // @Todo: Is there a way to reflect PushConstant from shader to CPP side?
    //
    struct PushConstant
    {
        u32 vertex_buffer_id;
        u32 texture_id;
        u32 sampler_id;

        u32 camera_id;
    };

    PushConstant push_constants = {
        .vertex_buffer_id  = 0,
        .texture_id        = (u32)tex_srv.m_index,
        .sampler_id        = (u32)sampler.m_index,

        .camera_id         = (u32)camera_desc.m_index
    };


    f32 time = 0.f;

    while (window->IsOpen()) 
    {
        window->PollEvents();

        // Update
        //
        {
            f32 time_elapsed = 0.017f;
            constexpr f32 dt = 1.f / 60.f;
            for (;time_elapsed >= dt; time_elapsed -= dt)
            {
                time += dt;
                camera->position  = vec4(vec3(cosf(time)*3.f, 3.f, sinf(time)*3.f), 1.0f);
                camera->view      = m4x4::LookAtLH(camera->position.xyz, vec3(0.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f));
                camera->proj      = m4x4::PerspectiveLH(DegreeToRadian(120), 9.f / 16.f, 0.1f, 1000.f);
                camera->view_proj = camera->proj * camera->view;
            }
            
            {
                uint size = sizeof(Camera);

                u64 staging_buffer_size = GetRequiredIntermediateSize(camera_buffer.m_resource, 0, 1);
                auto staging_buffer = Malloc(device, { .size = staging_buffer_size, .heap_kind = RHI_HEAP_KIND_UPLOAD });

                void* ptr = Map(staging_buffer);
                memcpy(ptr, camera, size);
                Unmap(staging_buffer);

                BeginCommandList(cmd_list);
                {
                    auto prev1 = CMD_TransitionBarrier(cmd_list, &camera_buffer, D3D12_RESOURCE_STATE_COPY_DEST);
                    auto prev2 = CMD_TransitionBarrier(cmd_list, &staging_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
                    CMD_Copy(cmd_list, camera_buffer, staging_buffer, size);
                    CMD_TransitionBarrier(cmd_list, &camera_buffer, prev1);
                    CMD_TransitionBarrier(cmd_list, &staging_buffer, prev2);
                }
                EndCommandList(cmd_list);
                ExecuteCommandList(cmd_queue, cmd_list);

                PlaceFence(cmd_queue, fence);
                WaitForFence(fence);

                Free(staging_buffer);
            }
        }

        BeginCommandList(cmd_list);
        {
            // @Temporary: Update current frame index!
            CMD_TransitionBarrier(cmd_list, swap_chain->m_resources[swap_chain->current_frame_index], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            CMD_SetRenderTarget(cmd_list, swap_chain->m_descriptors[swap_chain->current_frame_index]);
            CMD_ClearRTV(cmd_list, swap_chain->m_descriptors[swap_chain->current_frame_index], 0.0f, 0.2f, 0.4f, 1.0f);

            {
                CMD_SetViewport(cmd_list, 0, 0, width, height);
                CMD_SetScissor(cmd_list, 0, 0, width, height);

                ID3D12DescriptorHeap* heaps[] = {
                    cbv_srv_uav_heap->m_descriptor_heap,
                    sampler_heap->m_descriptor_heap
                };
                cmd_list->m_list->SetDescriptorHeaps(ARRAY_COUNT(heaps), heaps);
                cmd_list->m_list->SetGraphicsRootSignature(device->m_global_root_signature);

                cmd_list->m_list->SetPipelineState(pipeline_state->m_pso);

                cmd_list->m_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


                Stack<Scene_Component*> dfs;
                dfs.push(root_component);

                while (!dfs.empty())
                {
                    auto* node = dfs.top();
                    dfs.pop();

                    for (auto& mesh : node->meshes)
                    {
                        push_constants.vertex_buffer_id = mesh->vertex_buffer_descriptor.m_index;
                        CMD_SetGraphicsConstants(cmd_list, &push_constants, sizeof(push_constants));

                        CMD_SetIndexBuffer(cmd_list, mesh->index_buffer);
                        CMD_DrawIndexed(cmd_list, mesh->indices.size(), 1);
                    }

                    for (Scene_Component* child : node->children)
                    {
                        dfs.push(child);
                    }
                }
            }

            CMD_TransitionBarrier(cmd_list, swap_chain->m_resources[swap_chain->current_frame_index], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        }
        EndCommandList(cmd_list);
        ExecuteCommandList(cmd_queue, cmd_list);

        PlaceFence(cmd_queue, fence);
        Present(swap_chain);
        WaitForFence(fence);

        swap_chain->current_frame_index = swap_chain->m_swap_chain->GetCurrentBackBufferIndex();
    }

    // Cleanup
    //
    Window::Destroy(window);

    DeinitPipeline(pipeline_state);

    DeinitCompiler(compiler);

    DeinitFence(fence);
    DeinitSwapChain(swap_chain);
    DeinitDescriptorHeap(cbv_srv_uav_heap);
    DeinitDescriptorHeap(rtv_heap);
    DeinitCommandList(cmd_list);
    DeinitCommandQueue(cmd_queue);
    DeinitDevice(device);

    return 0;
}
