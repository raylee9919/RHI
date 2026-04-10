// Copyright Seong Woo Lee. All Rights Reserved.

#include <cstdlib>

#include "Core/Core_Common.h"
#include "Core/Core_Log.h"

#include "RHI_DX12.h"

extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

    __declspec(dllexport) extern const u32 D3D12SDKVersion = 619;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\.";
}

namespace Engine
{
    INTERNAL D3D12_DESCRIPTOR_HEAP_TYPE ToDX12DescriptorHeapType(RHI_DescriptorKind kind)
    {
        switch (kind)
        {
            default: CORE_ASSERT(!"invalid default case."); return {};
            case RHI_DESCRIPTOR_KIND_CBV_SRV_UAV: return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            case RHI_DESCRIPTOR_KIND_SAMPLER:     return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            case RHI_DESCRIPTOR_KIND_RTV:         return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            case RHI_DESCRIPTOR_KIND_DSV:         return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        }
    }

    ENGINE_API bool DX12_InitDevice(DX12_Device* device, bool use_debug_layer)
    {
        DWORD dxgi_factory_flags = 0;

        if (use_debug_layer)
        {
            ID3D12Debug* debug_interface = nullptr;

            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface))))
            {
                debug_interface->EnableDebugLayer();

                ID3D12Debug5* debug_interface5 = nullptr;
                if (SUCCEEDED(debug_interface->QueryInterface(IID_PPV_ARGS(&debug_interface5))))
                {
                    debug_interface5->SetEnableGPUBasedValidation(true);
                    debug_interface5->SetEnableSynchronizedCommandQueueValidation(true);
                    debug_interface5->Release();
                }
            }
            else
            {
                CORE_ASSERT(!"Unable to use debug layer.");
            }

            // @Todo: DRED?

            IDXGIInfoQueue* dxgi_info_queue = nullptr;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue))))
            {
                dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;

                dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
                dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] = {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };

                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgi_info_queue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
            }
            else
            {
                CORE_ASSERT(!"Failed to get IDXGIInfoQueue.");
            }
        }

        CORE_ASSERT(SUCCEEDED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&device->m_factory))), "Failed to create IDXGIFactory.");


        IDXGIAdapter1* adapter = nullptr;

        ID3D12Device* temp_device = nullptr;


        u64 max_vram_size = 0;

        D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_12_2; // @Temporary

        for (u32 index = 0; device->m_factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND; ++index)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            // @Todo: RT Support
            // {
            // }

            if (desc.DedicatedVideoMemory < max_vram_size)
            {
                continue;
            }
            max_vram_size = desc.DedicatedVideoMemory;

            // Put it at the last.
            if (FAILED(D3D12CreateDevice(adapter, feature_level, IID_PPV_ARGS(&temp_device))))
            {
                SafeReleaseCOM(&temp_device);
                continue;
            }

            char buf[1024] = {};
            std::wcstombs(buf, desc.Description, 1024);
            Log("Selected GPU: %s (%u MB)", buf, desc.DedicatedVideoMemory >> 20);
        }

        if (!temp_device)
        {
            Log("Unable to find capable device.");
            return false;
        }

        temp_device->QueryInterface(IID_PPV_ARGS(&device->m_device));

        // Get descriptor sizes
        device->cbv_srv_uav_size = device->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        device->sampler_size     = device->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        device->rtv_size         = device->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        device->dsv_size         = device->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        // Cleanup
        temp_device->Release();

        
        Log("Initted DX12 device.");
        return true;
    }

    ENGINE_API void DX12_DeinitDevice(DX12_Device* device)
    {
        if (device)
        {
            SafeReleaseCOM(&device->m_device);
            Log("Deinitted DX12 device.");
        }
    }

    ENGINE_API bool DX12_InitCommandQueue(DX12_Device*device, DX12_CommandQueue* cmd_queue)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT, // @Temporary
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0, // Single GPU
        };

        device->m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&cmd_queue->m_queue));

        Log("Initted DX12 command queue.");
        return true;
    }

    ENGINE_API void DX12_DeinitCommandQueue(DX12_CommandQueue* cmd_queue)
    {
        if (cmd_queue)
        {
            SafeReleaseCOM(&cmd_queue->m_queue);
            Log("Deinitted DX12 command queue.");
        }
    }

    ENGINE_API bool DX12_InitFence(DX12_Device* device, DX12_Fence* fence)
    {
        UINT64 init_value = 0;
        device->m_device->CreateFence(init_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence->m_fence));
        fence->value = init_value;
        fence->event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        Log("Initted DX12 fence.");
        return true;
    }

    ENGINE_API void DX12_DeinitFence(DX12_Fence* fence)
    {
        SafeReleaseCOM(&fence->m_fence);
        CloseHandle(fence->event);
    }

    ENGINE_API void DX12_PushFence(DX12_CommandQueue* cmd_queue, DX12_Fence* fence)
    {
        uint64_t value_to_set = fence->value;
        fence->value += 1;
        cmd_queue->m_queue->Signal(fence->m_fence, value_to_set);
    }

    ENGINE_API void DX12_WaitForFence(DX12_Fence* fence)
    {
        uint64_t value = fence->value - 1;
        if (value > fence->m_fence->GetCompletedValue())
        {
            fence->m_fence->SetEventOnCompletion(value, fence->event);
            WaitForSingleObject(fence->event, INFINITE);
        }
    }

    ENGINE_API bool DX12_InitSwapChain(DX12_Device* device, DX12_SwapChain* swap_chain,
                                       DX12_DescriptorHeap* rtv_descriptor_heap,
                                       DX12_CommandQueue* cmd_queue, HWND hwnd,
                                       uint width, uint height, uint num_frames)
    {
        // Resource array's length is hard-coded to 3.
        CORE_ASSERT(num_frames <= 3);
        
        if (device && swap_chain && cmd_queue)
        {
            DXGI_SWAP_CHAIN_DESC1 desc = {
                .Width = width,
                .Height = height,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, // @Temporary
                .Stereo = FALSE, // For some VR voodoo?
                .SampleDesc = {
                    .Count = 1, // @Todo: Something related to MSAA?
                    .Quality = 0,
                },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = num_frames,
                .Scaling = DXGI_SCALING_STRETCH, // @Temporary: Sure?
                .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
                .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
                .Flags = 0, // @Temporary: Tearing...?
            };

            // Those are how miniengine did.
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
                .Windowed = TRUE
            };

            IDXGISwapChain1* swap_chain_1 = nullptr;

            if (FAILED(device->m_factory->CreateSwapChainForHwnd(cmd_queue->m_queue, hwnd, &desc, &fullscreen_desc, nullptr, &swap_chain_1)))
            {
                return false;
            }

            swap_chain_1->QueryInterface(IID_PPV_ARGS(&swap_chain->m_swap_chain));

            UINT current_frame_index = swap_chain->m_swap_chain->GetCurrentBackBufferIndex();
            swap_chain->current_frame_index = current_frame_index;

            for (uint i = 0; i < num_frames; ++i) {
                swap_chain->m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&swap_chain->m_resources[i]));

                DX12_Descriptor rtv = DX12_AllocDescriptor(rtv_descriptor_heap);

                swap_chain->m_descriptors[i] = rtv;

                device->m_device->CreateRenderTargetView(swap_chain->m_resources[i], nullptr, rtv.cpu_handle);
            }

            swap_chain->num_frames = num_frames;

            return true;
        }
        else
        {
            Log("Null detected. Abort initting DX12 swap chain.");
            return false;
        }
    }

    ENGINE_API void DX12_DeinitSwapChain(DX12_SwapChain* swap_chain)
    {
        if (swap_chain)
        {
            SafeReleaseCOM(&swap_chain->m_swap_chain);
            for (uint i = 0; i < swap_chain->num_frames; ++i) {
                SafeReleaseCOM(&swap_chain->m_resources[i]);
            }

            Log("Deinitted DX12 swap chain.");
        }
    }

    ENGINE_API void DX12_Present(DX12_SwapChain* swap_chain)
    {
        // @Todo: vsync, tearing flag?
        swap_chain->m_swap_chain->Present(1, 0);
    }

    ENGINE_API bool DX12_InitDescriptorHeap(DX12_Device* device,
                                            DX12_DescriptorHeap* descriptor_heap,
                                            uint32_t max_descriptors,
                                            RHI_DescriptorKind kind)
    {
        if (device && descriptor_heap)
        {
            u32 free_list_node_size_in_bits = sizeof(descriptor_heap->free_list[0]) << 3;
            u32 num_alloc = AlignUp(max_descriptors, free_list_node_size_in_bits); // = num_descriptors_to_alloc
            u32 num_free_list_nodes = num_alloc / free_list_node_size_in_bits;
            descriptor_heap->free_list = new u32[num_free_list_nodes];
            MemorySet(descriptor_heap->free_list, 0xff, sizeof(descriptor_heap->free_list[0]) * num_free_list_nodes);

            const bool is_shader_visible = kind == RHI_DESCRIPTOR_KIND_CBV_SRV_UAV || kind == RHI_DESCRIPTOR_KIND_SAMPLER;


            // Set a creation descriptor.
            //
            D3D12_DESCRIPTOR_HEAP_DESC desc = {
                .Type = ToDX12DescriptorHeapType(kind),
                .NumDescriptors = num_alloc,
                .NodeMask = 0
            };

            if (is_shader_visible)
            {
                desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            }

            // Create a heap.
            //
            if (FAILED(device->m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap->m_descriptor_heap))))
            {
                Log("CreateDescriptorHeap() failed.");
                return false;
            }

            // Get gpu handle if it's shader visible.
            //
            D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = {};

            if (is_shader_visible)
            {
                gpu_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(descriptor_heap->m_descriptor_heap->GetGPUDescriptorHandleForHeapStart());
            }

            // Set data
            //
            {
                descriptor_heap->kind            = kind;
                descriptor_heap->max_descriptors = num_alloc;
                descriptor_heap->num_descriptors = 0;
                descriptor_heap->cpu_handle      = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptor_heap->m_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
                descriptor_heap->gpu_handle      = gpu_handle;

                uint64_t descriptor_size;
                switch (kind)
                {
                    case RHI_DESCRIPTOR_KIND_CBV_SRV_UAV:   descriptor_size = device->cbv_srv_uav_size; break;
                    case RHI_DESCRIPTOR_KIND_SAMPLER:       descriptor_size = device->sampler_size; break;
                    case RHI_DESCRIPTOR_KIND_RTV:           descriptor_size = device->rtv_size; break;
                    case RHI_DESCRIPTOR_KIND_DSV:           descriptor_size = device->dsv_size; break;
                                                            INVALID_DEFAULT_CASE;
                }
                descriptor_heap->descriptor_size = descriptor_size;
            }


            Log("Initted DX12 descriptor heap.");
            return true;
        }
        else
        {
            Log("Null detected. Failed initting DX12 descriptor heap.");
            return false;
        }
    }

    ENGINE_API void DX12_DeinitDescriptorHeap(DX12_DescriptorHeap* descriptor_heap)
    {
        if (descriptor_heap)
        {
            SafeReleaseCOM(&descriptor_heap->m_descriptor_heap);

            if (descriptor_heap->free_list)
            {
                delete [] descriptor_heap->free_list;
            }
        }
        else
        {
            Log("Null detected. Failed deinitting DX12 descriptor heap.");
        }
    }

    ENGINE_API DX12_Descriptor DX12_AllocDescriptor(DX12_DescriptorHeap* descriptor_heap)
    {
        u32 num_bits = (sizeof(descriptor_heap->free_list[0]) << 3);
        u32 num_nodes = descriptor_heap->max_descriptors / num_bits;
        for (u32 i = 0; i < num_nodes; ++i)
        {
            u32& node = descriptor_heap->free_list[i];

            // No free slot.
            if (node == 0x0) {
                continue;
            }

            u32 bit = BitScanFromLSB(node);
            if (bit < 32) // found
            {
                node ^= (1 << bit);

                uint32_t index = i * num_bits  + bit;
                int32_t offset = index * descriptor_heap->descriptor_size;

                DX12_Descriptor result = {
                    .cpu_handle = descriptor_heap->cpu_handle.Offset(offset),
                };

                auto kind = descriptor_heap->kind;
                if (kind == RHI_DESCRIPTOR_KIND_CBV_SRV_UAV || kind == RHI_DESCRIPTOR_KIND_SAMPLER) 
                {
                    result.gpu_handle = descriptor_heap->gpu_handle.Offset(offset);
                }
                else if (kind != RHI_DESCRIPTOR_KIND_RTV && kind != RHI_DESCRIPTOR_KIND_DSV) 
                {
                    CORE_ASSERT(!"invalide code path");
                }

                return result;
            }
        }

        CORE_ASSERT(!"Descriptor heap is full!");
        return {};
    }

    ENGINE_API bool DX12_InitCommandList(DX12_Device *device, DX12_CommandList *cmd_list, D3D12_COMMAND_LIST_TYPE type)
    {
        CORE_ASSERT(SUCCEEDED(device->m_device->CreateCommandAllocator(type, IID_PPV_ARGS(&cmd_list->m_allocator))));
        CORE_ASSERT(SUCCEEDED(device->m_device->CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&cmd_list->m_list)))); // creates with closed state.

        cmd_list->is_open = false;

        Log("Initted DX12 command list.");
        return true;
    }

    ENGINE_API void DX12_DeinitCommandList(DX12_Device *device, DX12_CommandList *cmd_list)
    {
        SafeReleaseCOM(&cmd_list->m_list);
        SafeReleaseCOM(&cmd_list->m_allocator);
        
        Log("Deinitted DX12 command list.");
    }

    ENGINE_API void DX12_BeginCommandList(DX12_CommandList* cmd_list)
    {
        if (!cmd_list->is_open)
        {
            // Reset only when the associated command lists are finished. Use fence.
            cmd_list->m_allocator->Reset();

            cmd_list->m_list->Reset(cmd_list->m_allocator, nullptr);
            cmd_list->is_open = true;
        }
        else
        {
            CORE_ASSERT(!"Already open.");
        }
    }

    ENGINE_API void DX12_EndCommandList(DX12_CommandList* cmd_list)
    {
        if (cmd_list->is_open)
        {
            cmd_list->m_list->Close();
            cmd_list->is_open = false;
        }
        else
        {
            CORE_ASSERT(!"Already closed.");
        }
    }

    ENGINE_API void DX12_ExecuteCommandList(DX12_CommandQueue* cmd_queue, DX12_CommandList* cmd_list)
    {
        ID3D12CommandList* list[] = { cmd_list->m_list };
        cmd_queue->m_queue->ExecuteCommandLists(1, list);
    }

    ENGINE_API void DX12_SetViewport(DX12_CommandList* cmd_list, int top_left_x, int top_left_y, int width, int height)
    {
        D3D12_VIEWPORT viewport = {
            .TopLeftX = (FLOAT)top_left_x,
            .TopLeftY = (FLOAT)top_left_y,
            .Width    = (FLOAT)width,
            .Height   = (FLOAT)height,
            .MinDepth = D3D12_MIN_DEPTH,
            .MaxDepth = D3D12_MAX_DEPTH
        };
        cmd_list->m_list->RSSetViewports(1, &viewport);
    }

    ENGINE_API void DX12_SetScissor(DX12_CommandList* cmd_list, int top_left_x, int top_left_y, int width, int height)
    {
        D3D12_RECT rect = {
            .left = top_left_x,
            .top  = top_left_y,
            .right = top_left_x + width,
            .bottom = top_left_y + height,
        };
        cmd_list->m_list->RSSetScissorRects(1, &rect);
    }

    ENGINE_API void DX12_ClearRTV(DX12_CommandList* cmd_list, DX12_Descriptor rtv, float r, float g, float b, float a)
    {
        FLOAT color[] = { r, g, b, a };
        cmd_list->m_list->ClearRenderTargetView(rtv.cpu_handle, color, 0, nullptr);
    }

    ENGINE_API void DX12_TransitionBarrier(DX12_CommandList* cmd_list, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
        cmd_list->m_list->ResourceBarrier(1, &barrier);
    }
}
