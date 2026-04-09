// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/Core_Common.h"

#include "RHI/RHI.h"

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

        uint64_t        cbv_srv_uav_size;
        uint64_t        sampler_size;
        uint64_t        rtv_size;
        uint64_t        dsv_size;
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
    };

    struct DX12_DescriptorHeap
    {
        RHI_DescriptorKind            kind;
        ID3D12DescriptorHeap*         m_descriptor_heap;
        uint32_t*                     free_list;
        uint32_t                      num_descriptors;
        uint32_t                      max_descriptors;
        uint64_t                      descriptor_size;
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
    };

    struct DX12_SwapChain
    {
        IDXGISwapChain3* m_swap_chain;
        ID3D12Resource*  m_resources[3];
        DX12_Descriptor  m_descriptors[3];
        uint8_t          num_frames;
        uint8_t          current_frame_index;
    };





    ENGINE_API bool DX12_InitDevice(DX12_Device* device, bool use_debug_layer);
    ENGINE_API void DX12_DeinitDevice(DX12_Device* device);


    ENGINE_API bool DX12_InitCommandQueue(DX12_Device*device, DX12_CommandQueue* cmd_queue);
    ENGINE_API void DX12_DeinitCommandQueue(DX12_CommandQueue* cmd_queue);


    ENGINE_API bool DX12_InitFence(DX12_Device* device, DX12_Fence* fence);
    ENGINE_API void DX12_DeinitFence(DX12_Fence* fence);
    ENGINE_API void DX12_PushFence(DX12_CommandQueue* cmd_queue, DX12_Fence* fence);
    ENGINE_API void DX12_WaitForFence(DX12_Fence* fence);


    ENGINE_API bool DX12_InitDescriptorHeap(DX12_Device* device, DX12_DescriptorHeap* descriptor_heap, uint32_t max_descriptors, RHI_DescriptorKind kind);
    ENGINE_API void DX12_DeinitDescriptorHeap(DX12_DescriptorHeap* descriptor_heap);
    ENGINE_API DX12_Descriptor DX12_AllocDescriptor(DX12_DescriptorHeap* descriptor_heap);


    ENGINE_API bool DX12_InitSwapChain(DX12_Device* device, DX12_SwapChain* swap_chain, DX12_DescriptorHeap* rtv_descriptor_heap, DX12_CommandQueue* cmd_queue, HWND hwnd, uint width, uint height, uint num_frames);
    ENGINE_API void DX12_DeinitSwapChain(DX12_SwapChain* swap_chain);
    ENGINE_API void DX12_Present(DX12_SwapChain* swap_chain);


    ENGINE_API bool DX12_InitCommandList(DX12_Device *device, DX12_CommandList *cmd_list, D3D12_COMMAND_LIST_TYPE type);
    ENGINE_API void DX12_DeinitCommandList(DX12_Device *device, DX12_CommandList *cmd_list);
    ENGINE_API void DX12_BeginCommandList(DX12_CommandList* cmd_list);
    ENGINE_API void DX12_EndCommandList(DX12_CommandList* cmd_list);
    ENGINE_API void DX12_ExecuteCommandList(DX12_CommandQueue* cmd_queue, DX12_CommandList* cmd_list);

    ENGINE_API void DX12_SetViewport(DX12_CommandList* cmd_list, int top_left_x, int top_left_y, int width, int height);
    ENGINE_API void DX12_SetScissor(DX12_CommandList* cmd_list, int top_left_x, int top_left_y, int width, int height);

    ENGINE_API void DX12_ClearRTV(DX12_CommandList* cmd_list, DX12_Descriptor rtv, float r, float g, float b, float a);

    ENGINE_API void DX12_TransitionBarrier(DX12_CommandList* cmd_list, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
}
