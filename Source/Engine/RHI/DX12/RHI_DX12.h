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
        IDXGIFactory6* m_factory;
    };

    struct DX12_CommandQueue
    {
        ID3D12CommandQueue* m_queue;
    };

    struct DX12_SwapChain
    {
        IDXGISwapChain1* m_swap_chain;
    };

    struct DX12_DescriptorHeap
    {
        ID3D12DescriptorHeap* m_descriptor_heap;
    };



    ENGINE_API bool DX12_InitDevice(DX12_Device* device, bool use_debug_layer);
    ENGINE_API void DX12_DeinitDevice(DX12_Device* device);

    ENGINE_API bool DX12_InitCommandQueue(DX12_Device*device, DX12_CommandQueue* cmd_queue);
    ENGINE_API void DX12_DeinitCommandQueue(DX12_CommandQueue* cmd_queue);

    ENGINE_API bool DX12_InitDescriptorHeap(DX12_Device* device, DX12_DescriptorHeap* descriptor_heap, uint num_descriptors, RHI_DescriptorKind kind);
    ENGINE_API void DX12_DeinitDescriptorHeap(DX12_DescriptorHeap* descriptor_heap);

    ENGINE_API bool DX12_InitSwapchain(DX12_Device* device, DX12_SwapChain* swap_chain, DX12_CommandQueue* cmd_queue, HWND hwnd, uint width, uint height, uint num_frames);
    ENGINE_API void DX12_DeinitSwapchain(DX12_SwapChain* swap_chain);
}
