// Copyright Seong Woo Lee. All Rights Reserved.

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
    
    INTERNAL D3D12_HEAP_TYPE ToD3D12HeapType(RHI_HeapKind heap_kind)
    {
        switch (heap_kind)
        {
            INVALID_DEFAULT_CASE;
            case RHI_HEAP_KIND_DEFAULT:  return D3D12_HEAP_TYPE_DEFAULT;
            case RHI_HEAP_KIND_UPLOAD:   return D3D12_HEAP_TYPE_UPLOAD;
            case RHI_HEAP_KIND_READBACK: return D3D12_HEAP_TYPE_READBACK;
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


        // Create root signature for bindless framework.
        //
        // @Todo: D3D12_RESOURCE_BINDING_TIER_3 and D3D_SHADER_MODEL_6_6 support check
        ID3D12RootSignature* root_signature = nullptr;
        {
            D3D12_ROOT_PARAMETER1 root_params[1] = {};
            root_params[0] = {
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                .Constants = {
                    .ShaderRegister = 0,                  // b0
                    .RegisterSpace  = 0,                  // space0
                    .Num32BitValues = 144 / sizeof(uint), // 36
                },
                .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
            };

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc = {
                .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
                .Desc_1_1 = {
                    .NumParameters     = _countof(root_params),
                    .pParameters       = root_params,
                    .NumStaticSamplers = 0,
                    .pStaticSamplers   = nullptr,
                    .Flags = (D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | 
                              D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | 
                              D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED)
                }
            };

            ID3DBlob* root_signature_blob = nullptr;
            ID3DBlob* error_blob = nullptr;
            CORE_ASSERT(SUCCEEDED(D3D12SerializeVersionedRootSignature(&root_signature_desc, &root_signature_blob, &error_blob)));

            CORE_ASSERT(SUCCEEDED(device->m_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature))));
        }
        device->m_global_root_signature = root_signature;


        // Cleanup
        //
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

    ENGINE_API void DX12_PlaceFence(DX12_CommandQueue* cmd_queue, DX12_Fence* fence)
    {
        //uint64_t value_to_set = fence->value;
        //fence->value += 1;
        //cmd_queue->m_queue->Signal(fence->m_fence, value_to_set);
    fence->value += 1;
    cmd_queue->m_queue->Signal(fence->m_fence, fence->value);  // signals 1, 2, 3...
    }

    ENGINE_API void DX12_WaitForFence(DX12_Fence* fence)
    {
        //uint64_t value = fence->value - 1;
        //if (value > fence->m_fence->GetCompletedValue())
        //{
        //    fence->m_fence->SetEventOnCompletion(value, fence->event);
        //    WaitForSingleObject(fence->event, INFINITE);
        //}

    if (fence->value > fence->m_fence->GetCompletedValue())  // now actually blocks
    {
        fence->m_fence->SetEventOnCompletion(fence->value, fence->event);
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

                uint32_t descriptor_size = 0;
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

                DX12_Descriptor result = {};
                result.cpu_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptor_heap->cpu_handle, offset);
                    
                auto kind = descriptor_heap->kind;
                if (kind == RHI_DESCRIPTOR_KIND_CBV_SRV_UAV || kind == RHI_DESCRIPTOR_KIND_SAMPLER) 
                {
                    result.gpu_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(descriptor_heap->gpu_handle, offset);
                }
                else if (kind != RHI_DESCRIPTOR_KIND_RTV && kind != RHI_DESCRIPTOR_KIND_DSV) 
                {
                    CORE_ASSERT(!"invalide code path");
                }

                result.m_index = index;

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

    ENGINE_API void DX12_DeinitCommandList(DX12_CommandList *cmd_list)
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

    ENGINE_API void DX12_CMD_SetViewport(DX12_CommandList* cmd_list, int top_left_x, int top_left_y, int width, int height)
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

    ENGINE_API void DX12_CMD_SetScissor(DX12_CommandList* cmd_list, int top_left_x, int top_left_y, int width, int height)
    {
        D3D12_RECT rect = {
            .left = top_left_x,
            .top  = top_left_y,
            .right = top_left_x + width,
            .bottom = top_left_y + height,
        };
        cmd_list->m_list->RSSetScissorRects(1, &rect);
    }

    ENGINE_API void DX12_CMD_ClearRTV(DX12_CommandList* cmd_list, DX12_Descriptor rtv, float r, float g, float b, float a)
    {
        FLOAT color[] = { r, g, b, a };
        cmd_list->m_list->ClearRenderTargetView(rtv.cpu_handle, color, 0, nullptr);
    }

    ENGINE_API void DX12_CMD_SetRenderTarget(DX12_CommandList* cmd_list, DX12_Descriptor rtv)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE *dsv = nullptr;
        cmd_list->m_list->OMSetRenderTargets(1, &rtv.cpu_handle, FALSE, dsv);
    }

    ENGINE_API void DX12_CMD_Copy(DX12_CommandList* cmd_list, DX12_Buffer& dst, DX12_Buffer& src, uint64_t size)
    {
        cmd_list->m_list->CopyBufferRegion(dst.m_resource, 0, src.m_resource, 0, size);
    }

    ENGINE_API void DX12_CMD_TransitionBarrier(DX12_CommandList* cmd_list, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
        cmd_list->m_list->ResourceBarrier(1, &barrier);
    }

    ENGINE_API D3D12_RESOURCE_STATES DX12_CMD_TransitionBarrier(DX12_CommandList* cmd_list, DX12_Buffer* buffer, D3D12_RESOURCE_STATES state)
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffer->m_resource, buffer->m_state, state);
        cmd_list->m_list->ResourceBarrier(1, &barrier);

        auto prev_state = buffer->m_state;

        buffer->m_state = state;

        return prev_state;
    }

    ENGINE_API void DX12_CMD_Draw(DX12_CommandList* cmd_list, uint32_t num_vertices, uint32_t num_instances, uint32_t first_vertex, uint32_t first_instance)
    {
        cmd_list->m_list->DrawInstanced(num_vertices, num_instances, first_vertex, first_instance);
    }

    ENGINE_API void DX12_CMD_DrawIndexed(DX12_CommandList* cmd_list, uint num_indices_per_instance, uint num_instances)
    {
        uint start_index = 0;
        uint base_vertex = 0;
        uint start_instance = 0;
        cmd_list->m_list->DrawIndexedInstanced(num_indices_per_instance, num_instances, start_index, base_vertex, start_instance);
    }

    ENGINE_API void DX12_CMD_SetGraphicsConstants(DX12_CommandList* cmd_list, void* data, uint64_t size)
    {
        cmd_list->m_list->SetGraphicsRoot32BitConstants(0, (size >> 2), data, 0);
    }

    ENGINE_API void DX12_CMD_SetIndexBuffer(DX12_CommandList* cmd_list, DX12_Buffer buffer)
    {
        D3D12_INDEX_BUFFER_VIEW view = {
            .BufferLocation = buffer.m_gpu_address,
            .SizeInBytes = (UINT)buffer.m_size,
            .Format = DXGI_FORMAT_R32_UINT
        };
        cmd_list->m_list->IASetIndexBuffer(&view);
    }

    ENGINE_API DX12_Buffer DX12_Malloc(DX12_Device* device, DX12_BufferDesc desc)
    {
        DX12_Buffer result;

        ID3D12Resource* resource = nullptr;

        D3D12_HEAP_TYPE heap_type = ToD3D12HeapType(desc.heap_kind);

        CD3DX12_HEAP_PROPERTIES heap_prop = CD3DX12_HEAP_PROPERTIES(heap_type);
        D3D12_HEAP_FLAGS heap_flags       = D3D12_HEAP_FLAG_NONE; // @Temporary: Who knows...
        CD3DX12_RESOURCE_DESC res_desc    = CD3DX12_RESOURCE_DESC::Buffer(desc.size);
        D3D12_RESOURCE_STATES init_state  = D3D12_RESOURCE_STATE_COMMON;

        device->m_device->CreateCommittedResource(&heap_prop, heap_flags, &res_desc, init_state, nullptr, IID_PPV_ARGS(&resource));

        result.m_resource    = resource;
        result.m_gpu_address = resource->GetGPUVirtualAddress();
        result.m_size        = desc.size;
        result.m_state       = init_state;

        return result;
    }

    ENGINE_API void DX12_Free(DX12_Buffer buffer)
    {
        buffer.m_resource->Release();
    }

    ENGINE_API void* DX12_Map(DX12_Buffer buffer)
    {
        void *ptr = nullptr;
        D3D12_RANGE read_range(0, 0); // @Temporary: No read.
        UINT subresource = 0;
        buffer.m_resource->Map(subresource, &read_range, &ptr);
        return ptr;
    }

    ENGINE_API void DX12_Unmap(DX12_Buffer buffer)
    {
        D3D12_RANGE range(0, buffer.m_size);
        UINT subresource = 0;
        buffer.m_resource->Unmap(subresource, &range);
    }

    ENGINE_API DX12_Texture DX12_AllocTexture(DX12_Device* device, DX12_TextureDesc tex_desc)
    {
        DX12_Texture result = {};

        D3D12_RESOURCE_DESC res_desc = {
            .Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Width            = tex_desc.width,
            .Height           = tex_desc.height,
            .DepthOrArraySize = tex_desc.depth,
            .MipLevels        = tex_desc.mip_levels,
            .Format           = tex_desc.format,
            .SampleDesc = {
                .Count   = 1,
                .Quality = 0
            },
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags = D3D12_RESOURCE_FLAG_NONE
        };

        D3D12_HEAP_PROPERTIES heap_prop = {
            .Type                 = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask     = 1,
            .VisibleNodeMask      = 1
        };

        ID3D12Resource* res = nullptr;
        D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_COPY_DEST;
        CORE_ASSERT(SUCCEEDED(device->m_device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &res_desc, init_state, nullptr, IID_PPV_ARGS(&res))));

        result.m_resource = res;
        result.m_desc     = tex_desc;
        result.m_state    = init_state;

        return result;
    }

    ENGINE_API void DX12_ReleaseTexture(DX12_Texture tex)
    {
        if (tex.m_resource) {
            tex.m_resource->Release();
        }
    }

    ENGINE_API uint64 DX12_GetRequiredIntermediateSize(DX12_Device* device, DX12_Texture tex)
    {
        uint64 result = 0;

        uint first_subresource = 0;
        uint num_subresources = tex.m_desc.depth * tex.m_desc.mip_levels; // @Todo: Correct?

        auto res_desc = tex.m_resource->GetDesc();
        device->m_device->GetCopyableFootprints(&res_desc, first_subresource, num_subresources, 0, nullptr, nullptr, nullptr, &result);

        return result;
    }

    ENGINE_API bool DX12_InitPipeline(DX12_Device* device, DX12_Pipeline* pipeline,
                                      DXIL::CompiledShader* vs, DXIL::CompiledShader* ps,
                                      ID3D12RootSignature* root_signature,
                                      D3D12_INPUT_ELEMENT_DESC* input_element_descs, uint32_t num_input_element_descs)
    {
        if (pipeline)
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {
                .pRootSignature = root_signature,

                .VS = {
                    .pShaderBytecode = vs->bytes,
                    .BytecodeLength  = vs->length
                },
                .PS = {
                    .pShaderBytecode = ps->bytes,
                    .BytecodeLength  = ps->length
                },

                .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT), // @Temporary
                .SampleMask = 0xff,
                .RasterizerState = {
                    .FillMode              = D3D12_FILL_MODE_SOLID,
                    .CullMode              = D3D12_CULL_MODE_BACK,
                    .FrontCounterClockwise = TRUE,
                    .DepthBias             = 0,
                    .DepthBiasClamp        = 0.f, 
                    .SlopeScaledDepthBias  = 0.f,
                    .DepthClipEnable       = TRUE,
                    .MultisampleEnable     = FALSE,
                    .AntialiasedLineEnable = FALSE,
                    .ForcedSampleCount     = 0,
                    .ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
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
                    .pInputElementDescs = input_element_descs,
                    .NumElements = num_input_element_descs
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

            HRESULT hr = device->m_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline->m_pso));
            CORE_ASSERT(SUCCEEDED(hr));

            return true;
        }
        else
        {
            return false;
        }
    }

    ENGINE_API void DX12_DeinitPipeline(DX12_Pipeline* pipeline)
    {
        pipeline->m_pso->Release();
    }
}
