// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/Core_Common.h"

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
    struct DX12_Device
    {
        ID3D12Device10* m_device;
        IDXGIFactory6*  m_factory;

        uint32_t        cbv_srv_uav_size;
        uint32_t        sampler_size;
        uint32_t        rtv_size;
        uint32_t        dsv_size;

        ID3D12RootSignature* m_global_root_signature; // bindless
    };

    struct DX12_Fence
    {
        ID3D12Fence* m_fence;
        uint64_t     value;
        HANDLE       event;
    };

    struct DX12_CommandQueue
    {
        ID3D12CommandQueue* m_queue;
    };

    struct DX12_CommandList
    {
        ID3D12CommandAllocator*     m_allocator;
        ID3D12GraphicsCommandList7* m_list;
        bool is_open;
    };

    struct DX12_Descriptor
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        uint64_t m_index;
    };

    struct DX12_DescriptorHeap
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

    struct DX12_SwapChain
    {
        IDXGISwapChain3* m_swap_chain;
        ID3D12Resource*  m_resources[3];
        DX12_Descriptor  m_descriptors[3];
        uint32_t         num_frames;
        uint32_t         current_frame_index;
    };

    struct DX12_BufferDesc
    {
        uint64_t     size;
        RHI_HeapKind heap_kind;
    };

    struct DX12_Buffer
    {
        ID3D12Resource* m_resource;
        D3D12_GPU_VIRTUAL_ADDRESS m_gpu_address;
        uint64_t m_size;
        D3D12_RESOURCE_STATES m_state;
    };

    struct DX12_TextureDesc
    {
        uint32_t    width;
        uint32_t    height;
        uint16_t    depth;
        uint16_t    mip_levels;
        DXGI_FORMAT format;
    };

    struct DX12_Texture
    {
        ID3D12Resource*       m_resource;
        DX12_TextureDesc      m_desc;
        D3D12_RESOURCE_STATES m_state;
    };

    struct DX12_Pipeline
    {
        ID3D12PipelineState* m_pso;
    };





    ENGINE_API bool DX12_InitDevice(DX12_Device* device, bool use_debug_layer);
    ENGINE_API void DX12_DeinitDevice(DX12_Device* device);


    ENGINE_API bool DX12_InitCommandQueue(DX12_Device*device, DX12_CommandQueue* cmd_queue);
    ENGINE_API void DX12_DeinitCommandQueue(DX12_CommandQueue* cmd_queue);


    ENGINE_API bool DX12_InitFence(DX12_Device* device, DX12_Fence* fence);
    ENGINE_API void DX12_DeinitFence(DX12_Fence* fence);
    ENGINE_API void DX12_PlaceFence(DX12_CommandQueue* cmd_queue, DX12_Fence* fence);
    ENGINE_API void DX12_WaitForFence(DX12_Fence* fence);


    ENGINE_API bool DX12_InitDescriptorHeap(DX12_Device* device, DX12_DescriptorHeap* descriptor_heap, uint32_t max_descriptors, RHI_DescriptorKind kind);
    ENGINE_API void DX12_DeinitDescriptorHeap(DX12_DescriptorHeap* descriptor_heap);
    ENGINE_API DX12_Descriptor DX12_AllocDescriptor(DX12_DescriptorHeap* descriptor_heap);


    ENGINE_API bool DX12_InitSwapChain(DX12_Device* device, DX12_SwapChain* swap_chain, DX12_DescriptorHeap* rtv_descriptor_heap, DX12_CommandQueue* cmd_queue, HWND hwnd, uint width, uint height, uint num_frames);
    ENGINE_API void DX12_DeinitSwapChain(DX12_SwapChain* swap_chain);
    ENGINE_API void DX12_Present(DX12_SwapChain* swap_chain);


    ENGINE_API bool DX12_InitCommandList(DX12_Device *device, DX12_CommandList *cmd_list, D3D12_COMMAND_LIST_TYPE type);
    ENGINE_API void DX12_DeinitCommandList(DX12_CommandList *cmd_list);
    ENGINE_API void DX12_BeginCommandList(DX12_CommandList* cmd_list);
    ENGINE_API void DX12_EndCommandList(DX12_CommandList* cmd_list);
    ENGINE_API void DX12_ExecuteCommandList(DX12_CommandQueue* cmd_queue, DX12_CommandList* cmd_list);

    ENGINE_API void DX12_CMD_SetViewport(DX12_CommandList* cmd_list, int top_left_x, int top_left_y, int width, int height);
    ENGINE_API void DX12_CMD_SetScissor(DX12_CommandList* cmd_list, int top_left_x, int top_left_y, int width, int height);
    ENGINE_API void DX12_CMD_ClearRTV(DX12_CommandList* cmd_list, DX12_Descriptor rtv, float r, float g, float b, float a);
    ENGINE_API void DX12_CMD_SetRenderTarget(DX12_CommandList* cmd_list, DX12_Descriptor rtv);
    ENGINE_API void DX12_CMD_Copy(DX12_CommandList* cmd_list, DX12_Buffer& dst, DX12_Buffer& src, uint64_t size);
    ENGINE_API void DX12_CMD_TransitionBarrier(DX12_CommandList* cmd_list, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
    ENGINE_API D3D12_RESOURCE_STATES DX12_CMD_TransitionBarrier(DX12_CommandList* cmd_list, DX12_Buffer* buffer, D3D12_RESOURCE_STATES state);
    ENGINE_API void DX12_CMD_Draw(DX12_CommandList* cmd_list, uint32_t num_vertices, uint32_t num_instances, uint32_t first_vertex, uint32_t first_instance);
    ENGINE_API void DX12_CMD_SetGraphicsConstants(DX12_CommandList* cmd_list, void* data, uint64_t size);
    ENGINE_API void DX12_CMD_SetIndexBuffer(DX12_CommandList* cmd_list, DX12_Buffer* buffer);


    ENGINE_API DX12_Buffer DX12_Malloc(DX12_Device* device, DX12_BufferDesc desc);
    ENGINE_API void DX12_Free(DX12_Buffer buffer);
    ENGINE_API void* DX12_Map(DX12_Buffer buffer);
    ENGINE_API void DX12_Unmap(DX12_Buffer buffer);

    ENGINE_API DX12_Texture DX12_AllocTexture(DX12_Device* device, DX12_TextureDesc tex_desc);
    ENGINE_API void DX12_ReleaseTexture(DX12_Texture tex);

    ENGINE_API uint64 DX12_GetRequiredIntermediateSize(DX12_Device* device, DX12_Texture tex);

    ENGINE_API bool DX12_InitPipeline(DX12_Device* device, DX12_Pipeline* pipeline, DXIL::CompiledShader* vs, DXIL::CompiledShader* ps, ID3D12RootSignature* root_signature, D3D12_INPUT_ELEMENT_DESC* input_element_descs, uint32_t num_input_element_descs);
    ENGINE_API void DX12_DeinitPipeline(DX12_Pipeline* pipeline);
}
