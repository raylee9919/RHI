// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core/Core_Array.h"

#include "Window/Window.h"
#include "OS/OS_Main.h"

#include "RHI/DX12/RHI_DX12.h"
#include "Shader/DXIL/DXIL_Compiler.h"

#include "ThirdParty/DirectX/Include/d3d12.h"
#include "ThirdParty/DirectX/Include/d3dx12/d3dx12.h"
#include "ThirdParty/DXC/Include/d3d12shader.h"


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
    auto vs_compilation = CompileShader(compiler, vs, is_debug);

    HLSL_Shader ps = {
        .source = (u8*)vs_source,
        .length = strlen(vs_source),
        .entry = "PS_Main",
        .target_profile = "ps_6_0"
    };
    auto ps_compilation = CompileShader(compiler, ps, is_debug);

    


    // @Temporary
    // @Temporary
    // @Temporary
    // @Temporary
    // @Temporary
    //

    {
        D3D12_SHADER_DESC shader_desc;
        auto r = vs_compilation.reflection;
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
                .SemanticName = desc.SemanticName,
                .SemanticIndex = desc.SemanticIndex,
                .Format = ToDXGIFormat(desc.ComponentType, desc.Mask),
                .InputSlot = 0u, // @Study: I don't know the impact of it.
                .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,

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

            if (shader_input_bind_desc.Type == D3D_SIT_CBUFFER)
            {
                ID3D12ShaderReflectionConstantBuffer* shader_reflection_constant_buffer = r->GetConstantBufferByIndex(i);

                D3D12_SHADER_BUFFER_DESC desc;
                shader_reflection_constant_buffer->GetDesc(&desc);

                D3D12_ROOT_PARAMETER1 cbv_root_param = {
                    .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
                    .Descriptor = {
                        .ShaderRegister = shader_input_bind_desc.BindPoint,
                        .RegisterSpace = shader_input_bind_desc.Space,
                        .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
                    },
                    .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
                };

                root_parameters.push_back(cbv_root_param);
            } 
            // @Todo: Texture
            else
            {
                CORE_ASSERT(!"Unhandled shader input type.");
            }
        }


        // Create root signature.
        //
        ID3D12RootSignature* root_signature = nullptr;
        {
            uint num_root_parameters = root_parameters.size();
            uint num_static_samplers = 0; // @Temporary

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc = {
                .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
                .Desc_1_1 = {
                    .NumParameters = num_root_parameters,
                    .pParameters = root_parameters.data(),
                    .NumStaticSamplers = num_static_samplers,
                    .pStaticSamplers = nullptr,
                    .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                }
            };

            ID3DBlob* root_signature_blob = nullptr;
            ID3DBlob* error_blob = nullptr;

            HRESULT hr = D3D12SerializeVersionedRootSignature(&root_signature_desc, &root_signature_blob, &error_blob);
            CORE_ASSERT(SUCCEEDED(hr));

            hr = device->m_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature));
            CORE_ASSERT(SUCCEEDED(hr));
        }


        // Create PSO.
        //
        ID3D12PipelineState* pso = nullptr;
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {
                .pRootSignature= root_signature,

                .VS = {
                    .pShaderBytecode = vs_compilation.bytes,
                    .BytecodeLength  = vs_compilation.length
                },
                .PS = {
                    .pShaderBytecode = ps_compilation.bytes,
                    .BytecodeLength  = ps_compilation.length
                },

                .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT), // @Temporary
                .SampleMask = 0xff,
                .RasterizerState = {
                    .FillMode = D3D12_FILL_MODE_SOLID,
                    .CullMode = D3D12_CULL_MODE_BACK,
                    .FrontCounterClockwise = TRUE,
                    .DepthBias = 0,
                    .DepthBiasClamp = 0.f, 
                    .SlopeScaledDepthBias = 0.f,
                    .DepthClipEnable = TRUE,
                    .MultisampleEnable = FALSE,
                    .AntialiasedLineEnable = FALSE,
                    .ForcedSampleCount = 0,
                    .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
                },
                .DepthStencilState = {
                    .DepthEnable   = FALSE,
                    //D3D12_DEPTH_WRITE_MASK DepthWriteMask;
                    //D3D12_COMPARISON_FUNC DepthFunc;
                    .StencilEnable = FALSE, 
                    //UINT8 StencilReadMask;
                    //UINT8 StencilWriteMask;
                    //D3D12_DEPTH_STENCILOP_DESC FrontFace;
                    //D3D12_DEPTH_STENCILOP_DESC BackFace;
                },
                .InputLayout = {
                    .pInputElementDescs = input_element_descs.data(),
                    .NumElements = (UINT)input_element_descs.size()
                },
                .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                
                .SampleDesc = {
                    .Count = 1,
                    .Quality = 0
                },

                // .CachedPSO;
                
                .Flags = D3D12_PIPELINE_STATE_FLAG_NONE // @Study: D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG
            };
            // @Temporary
            pso_desc.NumRenderTargets = 1;
            pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            //pso_desc.DSVFormat = ;

            HRESULT hr = device->m_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso));
            CORE_ASSERT(SUCCEEDED(hr));
        }
    }
    // @Temporary
    // @Temporary
    // @Temporary
    // @Temporary
    // @Temporary



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
