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
    };

    struct DX12_Command_List {
        ID3D12GraphicsCommandList7* native_cmd_list;
        ID3D12CommandAllocator*     native_cmd_allocator;
    };

    struct DX12_Command_Queue {
        ID3D12CommandQueue* native_cmd_queue;
    };

    struct DX12_Fence {
        ID3D12Fence* native_fence;
        u64          value;
        HANDLE       event;
    };

    struct DX12_Descriptor_Heap {
        ID3D12DescriptorHeap*         native_heap;
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        u32 max;
        u32 num;
    };

    struct DX12_Resource {
        ID3D12Resource* native_resource;
    };

    ENGINE_API DX12_Device* dx12_create_device();
    ENGINE_API void dx12_destroy_device(DX12_Device* device);

    ENGINE_API DX12_Command_Queue* dx12_create_command_queue(DX12_Device* device, D3D12_COMMAND_LIST_TYPE type);
    ENGINE_API void dx12_destroy_command_queue(DX12_Command_Queue* cmd_queue);

    ENGINE_API DX12_Command_List* dx12_create_command_list(DX12_Device* device, D3D12_COMMAND_LIST_TYPE type);
    ENGINE_API void dx12_destroy_command_list(DX12_Command_List* cmd_list);

    ENGINE_API DX12_Fence* dx12_create_fence(DX12_Device* device);
    ENGINE_API void dx12_destroy_fence(DX12_Fence* fence);

    ENGINE_API DX12_Descriptor_Heap* dx12_create_descriptor_heap(DX12_Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 max_descriptors);
    ENGINE_API void dx12_destroy_descriptor_heap(DX12_Descriptor_Heap* heap);
}
