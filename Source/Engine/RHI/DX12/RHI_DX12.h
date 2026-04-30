// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Basics.h"
#include "Core/SE_Array.h"
#include "Core/SE_Math.h"

#include "RHI/RHI.h"

#include "Shader/DXIL/DXIL_Compiler.h"

#include <ThirdParty/DirectX/Include/d3d12.h>
#include <ThirdParty/DirectX/Include/d3dx12/d3dx12.h>

#include <dxgi1_6.h>

#if BUILD_DEBUG
#  include <dxgidebug.h>
#endif

// Why the wrapper? Becasue I might want extra payload?
//

namespace Engine
{
    namespace DX12
    {
        struct Device
        {
            ID3D12Device10* m_device;
            IDXGIFactory6*  m_factory;

            uint32_t        cbv_srv_uav_size;
            uint32_t        sampler_size;
            uint32_t        rtv_size;
            uint32_t        dsv_size;

            ID3D12RootSignature* m_global_root_signature; // bindless
        };

        struct Fence
        {
            ID3D12Fence* m_fence;
            uint64_t     value;
            HANDLE       event;
        };

        struct CommandQueue
        {
            ID3D12CommandQueue* m_queue;
        };

        struct CommandList
        {
            ID3D12CommandAllocator*     m_allocator;
            ID3D12GraphicsCommandList7* m_list;
            bool is_open;
        };

        struct Descriptor
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
            CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
            uint64_t m_index;
        };

        struct DescriptorHeap
        {
            RHI_DescriptorKind            kind;
            ID3D12DescriptorHeap*         m_descriptor_heap;
            uint32_t*                     free_list;
            uint32_t                      num_descriptors;
            uint32_t                      max_descriptors;
            uint32_t                      descriptor_size;
            CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
            CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        };

        struct SwapChain
        {
            IDXGISwapChain3* m_swap_chain;
            ID3D12Resource*  m_resources[3];
            Descriptor       m_descriptors[3];
            uint32_t         num_frames;
            uint32_t         current_frame_index;
        };

        struct BufferDesc
        {
            uint64_t     size;
            RHI_HeapKind heap_kind;
        };

        struct Buffer
        {
            ID3D12Resource* m_resource;
            D3D12_GPU_VIRTUAL_ADDRESS m_gpu_address;
            uint64_t m_size;
            D3D12_RESOURCE_STATES m_state;
        };

        struct TextureDesc
        {
            uint32_t    width;
            uint32_t    height;
            uint16_t    depth;
            uint16_t    mip_levels;
            DXGI_FORMAT format;
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

            bool do_clear = false;
            D3D12_CLEAR_VALUE clear_value;

            D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_COPY_DEST;
        };

        struct Texture
        {
            ID3D12Resource*       m_resource;
            TextureDesc           m_desc;
            D3D12_RESOURCE_STATES m_state;
        };

        struct Intermediate_Pipeline_State
        {
            DXIL::CompiledShader vs_module;
            DXIL::CompiledShader ps_module;

            Array<D3D12_INPUT_ELEMENT_DESC> input_parameters;
        };

        struct Pipeline_State
        {
            ID3D12PipelineState* m_pso;
        };





        ENGINE_API bool InitDevice(Device* device, bool use_debug_layer);
        ENGINE_API void DeinitDevice(Device* device);


        ENGINE_API bool InitCommandQueue(Device*device, CommandQueue* cmd_queue);
        ENGINE_API void DeinitCommandQueue(CommandQueue* cmd_queue);


        ENGINE_API bool InitFence(Device* device, Fence* fence);
        ENGINE_API void DeinitFence(Fence* fence);
        ENGINE_API void PlaceFence(CommandQueue* cmd_queue, Fence* fence);
        ENGINE_API void WaitForFence(Fence* fence);


        ENGINE_API bool InitDescriptorHeap(Device* device, DescriptorHeap* descriptor_heap, uint32_t max_descriptors, RHI_DescriptorKind kind);
        ENGINE_API void DeinitDescriptorHeap(DescriptorHeap* descriptor_heap);
        ENGINE_API Descriptor AllocDescriptor(DescriptorHeap* descriptor_heap);


        ENGINE_API bool InitSwapChain(Device* device, SwapChain* swap_chain, DescriptorHeap* rtv_descriptor_heap, CommandQueue* cmd_queue, HWND hwnd, uint width, uint height, uint num_frames);
        ENGINE_API void DeinitSwapChain(SwapChain* swap_chain);
        ENGINE_API void Present(SwapChain* swap_chain);


        ENGINE_API bool InitCommandList(Device *device, CommandList *cmd_list, D3D12_COMMAND_LIST_TYPE type);
        ENGINE_API void DeinitCommandList(CommandList *cmd_list);
        ENGINE_API void BeginCommandList(CommandList* cmd_list);
        ENGINE_API void EndCommandList(CommandList* cmd_list);
        ENGINE_API void ExecuteCommandList(CommandQueue* cmd_queue, CommandList* cmd_list);

        ENGINE_API void CmdSetViewport(CommandList* cmd_list, int top_left_x, int top_left_y, int width, int height);
        ENGINE_API void CmdSetScissor(CommandList* cmd_list, int top_left_x, int top_left_y, int width, int height);
        ENGINE_API void CmdClearRTV(CommandList* cmd_list, Descriptor rtv, float r, float g, float b, float a);
        ENGINE_API void CmdClearDSV(CommandList* cmd_list, Descriptor& dsv, f32 depth, u8 stencil, u32 width, u32 height);
        ENGINE_API void CmdSetRenderTarget(CommandList* cmd_list, Descriptor* rtv, Descriptor* dsv);
        ENGINE_API void CmdCopy(CommandList* cmd_list, Buffer& dst, Buffer& src, uint64_t size);
        ENGINE_API void CmdTransitionBarrier(CommandList* cmd_list, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
        ENGINE_API D3D12_RESOURCE_STATES CmdTransitionBarrier(CommandList* cmd_list, Buffer* buffer, D3D12_RESOURCE_STATES state);
        ENGINE_API void CmdDraw(CommandList* cmd_list, uint32_t num_vertices, uint32_t num_instances, uint32_t first_vertex, uint32_t first_instance);
        ENGINE_API void CmdDrawIndexed(CommandList* cmd_list, uint num_indices_per_instance, uint num_instances);
        ENGINE_API void CmdSetGraphicsConstants(CommandList* cmd_list, void* data, uint64_t size);
        ENGINE_API void CmdSetIndexBuffer(CommandList* cmd_list, Buffer buffer);


        ENGINE_API Buffer Malloc(Device* device, BufferDesc desc);
        ENGINE_API void Free(Buffer buffer);
        ENGINE_API void* Map(Buffer buffer);
        ENGINE_API void Unmap(Buffer buffer);

        ENGINE_API Texture AllocTexture(Device* device, TextureDesc tex_desc);
        ENGINE_API void ReleaseTexture(Texture tex);

        ENGINE_API uint64 GetRequiredIntermediateSize(Device* device, Texture tex);

        ENGINE_API bool InitPipelineState(Device* device, Intermediate_Pipeline_State* intermediate, Pipeline_State* pipeline);
        ENGINE_API void DeinitPipeline(Pipeline_State* pipeline);
    }
}
