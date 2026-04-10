// Copyright Seong Woo Lee. All Rights Reserved.

#include "Window/Window.h"
#include "OS/OS_Main.h"

#include "RHI/DX12/RHI_DX12.h"
#include "Shader/DXIL/DXIL_Compiler.h"



using namespace Engine;
using namespace DXIL;

void LoadAsset(DX12_Device* device)
{
    ID3D12RootSignature* root_signature = nullptr;

    // Create an empty root signature.
    {
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ID3DBlob* rs_blob    = nullptr;
        ID3DBlob* error_blob = nullptr;

        CORE_ASSERT(SUCCEEDED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &rs_blob, &error_blob)));
        CORE_ASSERT(SUCCEEDED(device->m_device->CreateRootSignature(0, rs_blob->GetBufferPointer(), rs_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature))));
    }

    // Create a pipeline state, which includes compiling and loading shader.
    {
        ID3DBlob* vs_blob = nullptr;
        ID3DBlob* ps_blob = nullptr;

    }
}

int ENGINE_MAIN(int argc, const char** argv)
{
    Window* window = Window::Create("Hello", 1920, 1080);

    DX12_Device* device = new DX12_Device;
    DX12_InitDevice(device, true);

    DX12_CommandQueue* cmd_queue = new DX12_CommandQueue;
    DX12_InitCommandQueue(device, cmd_queue);

    DX12_CommandList* cmd_list = new DX12_CommandList;
    DX12_InitCommandList(device, cmd_list, D3D12_COMMAND_LIST_TYPE_DIRECT);

    DX12_DescriptorHeap* rtv_heap = new DX12_DescriptorHeap;
    DX12_InitDescriptorHeap(device, rtv_heap, 53, RHI_DESCRIPTOR_KIND_RTV);

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

    const char* vs_source = R"(
        // Copyright Seong Woo Lee. All Rights Reserved.
        
        struct PS_Input
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };
        
        PS_Input VS_Main(float4 position : POSITION, float4 color : COLOR)
        {
            PS_Input result;
        
            result.position = position;
            result.color = color;
        
            return result;
        }

        float4 PS_Main(PS_Input input) : SV_TARGET
        {
            return input.color;
        }
    )";
    HLSL_Shader vs = {
        .source = (u8*)vs_source,
        .length = strlen(vs_source),
        .entry = "VS_Main",
        .target_profile = "vs_6_0"
    };
    DXIL_Bytecode vs_bytes = CompileShader(compiler, vs, is_debug);

    HLSL_Shader ps = {
        .source = (u8*)vs_source,
        .length = strlen(vs_source),
        .entry = "PS_Main",
        .target_profile = "ps_6_0"
    };
    DXIL_Bytecode ps_bytes = CompileShader(compiler, ps, is_debug);




    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
    };
    ID3D12PipelineState* pipeline_state;
    device->m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline_state));



    while (window->IsOpen()) 
    {
        window->PollEvents();

        DX12_BeginCommandList(cmd_list);
        {
            DX12_SetViewport(cmd_list, 0, 0, width, height);
            DX12_SetScissor(cmd_list, 0, 0, width, height);

            // @Temporary: Update current frame index!
            DX12_TransitionBarrier(cmd_list, swap_chain->m_resources[swap_chain->current_frame_index], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            // @Temporary: Update current frame index!
            DX12_ClearRTV(cmd_list, swap_chain->m_descriptors[swap_chain->current_frame_index], 0.0f, 0.2f, 0.4f, 1.0f);

            // @Temporary: Update current frame index!
            DX12_TransitionBarrier(cmd_list, swap_chain->m_resources[swap_chain->current_frame_index], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        }
        DX12_EndCommandList(cmd_list);
        DX12_ExecuteCommandList(cmd_queue, cmd_list);

        DX12_Present(swap_chain);

        DX12_PushFence(cmd_queue, fence);
        DX12_WaitForFence(fence);

        swap_chain->current_frame_index = swap_chain->m_swap_chain->GetCurrentBackBufferIndex();
    }

    // Cleanup
    //
    Window::Destroy(window);

    DeinitCompiler(compiler);

    DX12_DeinitFence(fence);
    DX12_DeinitSwapChain(swap_chain);
    DX12_DeinitDescriptorHeap(rtv_heap);
    DX12_DeinitCommandList(device, cmd_list);
    DX12_DeinitCommandQueue(cmd_queue);
    DX12_DeinitDevice(device);

    return 0;
}
