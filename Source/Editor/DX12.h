// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Basics.h"

#include <ThirdParty/DirectX/Include/d3d12.h>
#include <ThirdParty/DirectX/Include/d3dx12/d3dx12.h>

#include <dxgi1_6.h>

#if BUILD_DEBUG
#  include <dxgidebug.h>
#endif


namespace Engine
{
    struct DX12_Device {
        ID3D12Device10* native_device;
        IDXGIFactory6*  factory;
    };

    struct DX12_Fence {
        ID3D12Fence* native_fence;
        uint64_t     value;
        HANDLE       event;
    };

    struct DX12_Descriptor_Heap {
        ID3D12DescriptorHeap*         native_heap;
        D3D12_DESCRIPTOR_HEAP_TYPE    type;
        uint64_t                      descriptor_size;
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        uint32_t                      max;
        uint32_t                      num;
        uint64_t*                     free_list; // bit fields.
        uint32_t                      num_nodes;
    };

    struct DX12_Descriptor {
        DX12_Descriptor_Heap*         my_heap;
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        int                           index;
    };

    struct DX12_Resource {
        ID3D12Resource* native_resource;
    };

    struct ENGINE_API DX12_Command_List {
        ID3D12GraphicsCommandList7* native_cmd_list;
        ID3D12CommandAllocator*     native_cmd_allocator;

        void begin();
        void end();
        void clear_rtv(DX12_Descriptor* descriptor, float r, float g, float b, float a);
        void transition_barrier(ID3D12Resource* resource, uint32_t subresource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
    };

    struct DX12_Command_Queue {
        ID3D12CommandQueue* native_cmd_queue;
    };

    struct ENGINE_API DX12_Swap_Chain {
        IDXGISwapChain3* native_swap_chain;
        uint32_t         num_frames;
        uint32_t         current_frame_index;

        ID3D12Resource** resources;
        DX12_Descriptor* rtvs;

        void present();
        ID3D12Resource* get_current_resource();
        DX12_Descriptor* get_current_rtv();
    };

    ENGINE_API DX12_Device* dx12_create_device();
    ENGINE_API void dx12_destroy_device(DX12_Device* device);

    ENGINE_API DX12_Swap_Chain* dx12_create_swap_chain(DX12_Device* device, DX12_Command_Queue* cmd_queue, DX12_Descriptor_Heap* rtv_heap, HWND hwnd, u32 width, u32 height, u32 num_frames);
    ENGINE_API void dx12_destroy_swap_chain(DX12_Swap_Chain* swap_chain);

    ENGINE_API DX12_Command_Queue* dx12_create_command_queue(DX12_Device* device, D3D12_COMMAND_LIST_TYPE type);
    ENGINE_API void dx12_destroy_command_queue(DX12_Command_Queue* cmd_queue);

    ENGINE_API DX12_Command_List* dx12_create_command_list(DX12_Device* device, D3D12_COMMAND_LIST_TYPE type);
    ENGINE_API void dx12_destroy_command_list(DX12_Command_List* cmd_list);

    ENGINE_API DX12_Fence* dx12_create_fence(DX12_Device* device);
    ENGINE_API void dx12_destroy_fence(DX12_Fence* fence);

    ENGINE_API DX12_Descriptor_Heap* dx12_create_descriptor_heap(DX12_Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 max_descriptors);
    ENGINE_API void dx12_destroy_descriptor_heap(DX12_Descriptor_Heap* heap);

    ENGINE_API DX12_Descriptor dx12_alloc_descriptor(DX12_Descriptor_Heap* heap);
    ENGINE_API void DX12_release_descriptor(const DX12_Descriptor& descriptor);

    ENGINE_API void dx12_execute_command_list(DX12_Command_Queue* cmd_queue, DX12_Command_List* cmd_list);

    ENGINE_API void dx12_fence_signal(DX12_Command_Queue* cmd_queue, DX12_Fence* fence);
    ENGINE_API void dx12_fence_wait(DX12_Fence* fence);
}
