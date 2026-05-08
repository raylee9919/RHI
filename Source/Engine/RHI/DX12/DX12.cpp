// Copyright Seong Woo Lee. All Rights Reserved.

#include "DX12.h"

#include "Core/SE_Basics.h"

extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

    __declspec(dllexport) extern const u32 D3D12SDKVersion = 619;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\.";
}

namespace Engine
{
    template <typename T>
    INTERNAL void dx12_safe_release_and_set_to_null(T** pptr)
    {
        if (pptr && *pptr) {
            (*pptr)->Release();
            *pptr = nullptr;
        }
    }

    ENGINE_API DX12_Device* dx12_create_device()
    {
        bool use_debug_layer = true;
        DWORD dxgi_factory_flags = 0;
        ID3D12Debug* debug_interface = nullptr;
        ID3D12Debug5* debug_interface_5 = nullptr;

        if (use_debug_layer) {
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)))) {
                if (SUCCEEDED(debug_interface->QueryInterface(IID_PPV_ARGS(&debug_interface_5)))) {
                    debug_interface_5->EnableDebugLayer();
                    debug_interface_5->SetEnableGPUBasedValidation(true);
                    debug_interface_5->SetEnableSynchronizedCommandQueueValidation(true);
                } else {
                    assert(0);
                }
            } else {
                assert(0);
            }
        } else {
            assert(0);
        }

        //
        // @Todo: DRED?
        //

        IDXGIInfoQueue* dxgi_info_queue = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue)))) {
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
        } else {
            assert(0);
        }

        IDXGIFactory6* dxgi_factory_6 = nullptr;
        if (FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory_6)))) {
            assert(0);
        }

        IDXGIAdapter1* adapter = nullptr;
        ID3D12Device* device_0 = nullptr;

        u64 max_vram_size = 0;
        D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_12_2; // @Temporary

        for (u32 index = 0;
             dxgi_factory_6->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND;
             ++index, dx12_safe_release_and_set_to_null(&adapter)) 
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)   { continue; }
            if (desc.DedicatedVideoMemory < max_vram_size) { continue; }

            max_vram_size = desc.DedicatedVideoMemory;

            // Put it at the last.
            dx12_safe_release_and_set_to_null(&device_0);
            if (FAILED(D3D12CreateDevice(adapter, feature_level, IID_PPV_ARGS(&device_0)))) {
                continue;
            }

            char buf[1024] = {};
            std::wcstombs(buf, desc.Description, 1024);
        }

        if (!device_0) {
            assert(0, "couldn't find a capable device.");
        }

        ID3D12Device10* device = nullptr;
        device_0->QueryInterface(IID_PPV_ARGS(&device));

        // Cleanups
        dx12_safe_release_and_set_to_null(&device_0);
        dx12_safe_release_and_set_to_null(&adapter);
        dx12_safe_release_and_set_to_null(&dxgi_info_queue);
        dx12_safe_release_and_set_to_null(&debug_interface_5);
        dx12_safe_release_and_set_to_null(&debug_interface);

        auto* result = new DX12_Device;
        result->native_device = device;
        result->factory = dxgi_factory_6;

        return result;
    }

    ENGINE_API void dx12_destroy_device(DX12_Device* device)
    {
        if (device) {
            if (device->native_device) { device->native_device->Release(); }
            if (device->factory)       { device->factory->Release(); }
        }
    }

    ENGINE_API DX12_Swap_Chain* dx12_create_swap_chain(DX12_Device* device, DX12_Command_Queue* cmd_queue, DX12_Descriptor_Heap* rtv_heap, HWND hwnd, u32 width, u32 height, u32 num_frames)
    {
        IDXGISwapChain1* swap_chain_1 = nullptr;
        IDXGISwapChain3* swap_chain_3 = nullptr;

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {
            .Width  = width,
            .Height = height,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .Stereo = FALSE, // Probably for VR
            .SampleDesc = {
                .Count   = 1, // @Temporary: Has to do with MSAA?
                .Quality = 0
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = num_frames,
            .Scaling     = DXGI_SCALING_STRETCH,
            .SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
            .AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED,
            .Flags       = 0, // @Temporary: Tearing?
        };

        // @Note: You can optionally set 4th parameter pFullscreenDesc to create a 
        // full-screen swap chain. If set to null, a windowed swap chain will be created.
        if (FAILED(device->factory->CreateSwapChainForHwnd(cmd_queue->native_cmd_queue, hwnd, &swap_chain_desc, nullptr, nullptr, &swap_chain_1))) {
            return nullptr;
        }
        if (FAILED(swap_chain_1->QueryInterface(IID_PPV_ARGS(&swap_chain_3)))) {
            return nullptr;
        }

        DX12_Swap_Chain* result = new DX12_Swap_Chain;
        {
            result->native_swap_chain   = swap_chain_3;
            result->num_frames          = num_frames;
            result->current_frame_index = swap_chain_3->GetCurrentBackBufferIndex();

            result->resources = new ID3D12Resource*[3];
            result->rtvs = new DX12_Descriptor[3];
        }

        for (u32 i = 0; i < num_frames; ++i) {
            swap_chain_3->GetBuffer(i, IID_PPV_ARGS(&result->resources[i]));
            auto rtv = dx12_alloc_descriptor(rtv_heap);
            result->rtvs[i] = rtv;
            device->native_device->CreateRenderTargetView(result->resources[i], nullptr, rtv.cpu_handle);
        }

        dx12_safe_release_and_set_to_null(&swap_chain_1);

        return result;
    }

    ENGINE_API void dx12_destroy_swap_chain(DX12_Swap_Chain* swap_chain)
    {
        if (swap_chain) {
            if (swap_chain->native_swap_chain) { swap_chain->native_swap_chain->Release(); }
            for (u32 i = 0; i < swap_chain->num_frames; ++i) {
                swap_chain->resources[i]->Release();
                dx12_dealloc_descriptor(swap_chain->rtvs[i]);
            }
            delete [] swap_chain->resources;
            delete [] swap_chain->rtvs;
        }
    }

    ENGINE_API DX12_Command_Queue* dx12_create_command_queue(DX12_Device* device, D3D12_COMMAND_LIST_TYPE type)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {
            .Type     = type,
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            .Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0, // Single GPU
        };

        DX12_Command_Queue* result = new DX12_Command_Queue;

        if (FAILED(device->native_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&result->native_cmd_queue)))) {
            return nullptr;
        }

        return result;
    }

    ENGINE_API DX12_Command_List* dx12_create_command_list(DX12_Device* device, D3D12_COMMAND_LIST_TYPE type)
    {
        if (!device) { return nullptr; }

        ID3D12CommandAllocator*     dx12_cmd_allocator;
        ID3D12GraphicsCommandList7* dx12_cmd_list;

        if (FAILED(device->native_device->CreateCommandAllocator(type, IID_PPV_ARGS(&dx12_cmd_allocator)))) { return nullptr; }
        if (FAILED(device->native_device->CreateCommandList1(0, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&dx12_cmd_list)))) { return nullptr; }

        DX12_Command_List* result = new DX12_Command_List;
        result->native_cmd_allocator = dx12_cmd_allocator;
        result->native_cmd_list      = dx12_cmd_list;

        return result;
    }
 
    ENGINE_API void dx12_destroy_command_queue(DX12_Command_Queue* cmd_queue)
    {
        if (cmd_queue) {
            if (cmd_queue->native_cmd_queue) { cmd_queue->native_cmd_queue->Release(); }
        }
    }

    ENGINE_API void dx12_destroy_command_list(DX12_Command_List* cmd_list)
    {
        if (cmd_list) {
            if (cmd_list->native_cmd_list)      { cmd_list->native_cmd_list->Release();      }
            if (cmd_list->native_cmd_allocator) { cmd_list->native_cmd_allocator->Release(); }
        }
    }

    ENGINE_API DX12_Fence* dx12_create_fence(DX12_Device* device)
    {
        ID3D12Fence* fence;
        if (FAILED(device->native_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
            return nullptr;
        }

        DX12_Fence* result = new DX12_Fence;
        result->native_fence = fence;
        result->value = 0;
        result->event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        return result;
    }

    ENGINE_API void dx12_destroy_fence(DX12_Fence* fence)
    {
        if (fence) {
            if (fence->native_fence) { fence->native_fence->Release(); }
            if (fence->event)        { CloseHandle(fence->event);      }
        }
    }

    ENGINE_API DX12_Descriptor_Heap* dx12_create_descriptor_heap(DX12_Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 max_descriptors)
    {
        if (!device || !device->native_device) { return nullptr; }

        D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if      (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)  { flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; }
        else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)      { flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; }
        else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)          { flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; }
        else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)          { flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; }
        else    { return nullptr; }

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {
            .Type           = type,
            .NumDescriptors = max_descriptors,
            .Flags          = flags,
            .NodeMask       = 0
        };

        ID3D12DescriptorHeap* heap = nullptr;
        if (FAILED(device->native_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap)))) {
            return nullptr;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = {};
        if (flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
            gpu_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart());
        }

        // Make a free list.
        u32 aligned_count = align_up(max_descriptors, 64u);
        u32 num_nodes = aligned_count / 64u;
        u64* free_list = new u64[num_nodes];
        memset(free_list, 0xff, sizeof(u64) * num_nodes);

        DX12_Descriptor_Heap* result = new DX12_Descriptor_Heap;
        result->native_heap     = heap;
        result->type            = type;
        result->descriptor_size = device->native_device->GetDescriptorHandleIncrementSize(type);
        result->cpu_handle      = CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart());
        result->gpu_handle      = gpu_handle;
        result->max             = max_descriptors;
        result->num             = 0;
        result->free_list       = free_list;
        result->num_nodes       = num_nodes;

        return result;
    }

    ENGINE_API void dx12_destroy_descriptor_heap(DX12_Descriptor_Heap* heap)
    {
        if (heap) {
            if (heap->native_heap) { heap->native_heap->Release(); }
            if (heap->free_list)   { delete [] heap->free_list;    }
        }
    }

    ENGINE_API DX12_Descriptor dx12_alloc_descriptor(DX12_Descriptor_Heap* heap)
    {
        int index = -1;
        for (u32 ni = 0; ni < heap->num_nodes; ++ni) {
            u64 bits = heap->free_list[ni];
            int b = tzcnt64(bits);
            if (b < 64) {
                bits ^= (1ull << b);
                index = ni * 64 + b;
                heap->free_list[ni] = bits;
                break;
            }
        }

        // @Todo: Should grow?
        assert(index != -1);

        int offset = index * heap->descriptor_size;

        DX12_Descriptor result = {};
        result.cpu_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->cpu_handle, offset);

        auto type = heap->type;

        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
            result.gpu_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->gpu_handle, offset);
        } else if (type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV && type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {
            assert(0);
        }

        assert(index != -1);

        result.index = index;
        result.my_heap = heap;
        
        return result;
    }

    ENGINE_API void dx12_dealloc_descriptor(const DX12_Descriptor& descriptor)
    {
        u64 a = descriptor.index / 64;
        u64 b = descriptor.index % 64;
        u64* bits = descriptor.my_heap->free_list + a;
        *bits = ( *bits | (1ull << b) );
    }

    void DX12_Command_List::begin()
    {
        native_cmd_allocator->Reset();
        native_cmd_list->Reset(native_cmd_allocator, nullptr);
    }

    void DX12_Command_List::end()
    {
        native_cmd_list->Close();
    }

    void DX12_Command_List::clear_rtv(DX12_Descriptor* descriptor, float r, float g, float b, float a)
    {
        FLOAT color[] = { r, g, b, a };
        native_cmd_list->ClearRenderTargetView(descriptor->cpu_handle, color, 0, nullptr);
    }

    void DX12_Command_List::clear_dsv(DX12_Descriptor* descriptor, float depth, int top_left_x, int top_left_y, int width, int height)
    {
        D3D12_RECT rect = {
            .left   = top_left_x,
            .top    = top_left_y,
            .right  = top_left_x + width,
            .bottom = top_left_y + height
        };
        native_cmd_list->ClearDepthStencilView(descriptor->cpu_handle, D3D12_CLEAR_FLAG_DEPTH, depth, 0u, 1, &rect);
    }

    void DX12_Command_List::transition_barrier(ID3D12Resource* resource, uint32_t subresource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
    {
        D3D12_RESOURCE_BARRIER barrier = {
            .Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .Transition = {
                .pResource   = resource,
                .Subresource = subresource,
                .StateBefore = before,
                .StateAfter  = after,
            }
        };
        native_cmd_list->ResourceBarrier(1, &barrier);
    }

    void DX12_Command_List::set_pipeline_state(DX12_Pipeline_State* state)
    {
        native_cmd_list->SetPipelineState(state->pso);
    }

    void DX12_Command_List::set_resource_and_sampler_heap(DX12_Descriptor_Heap* resource_heap, DX12_Descriptor_Heap* sampler_heap)
    {
        assert(resource_heap->type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        assert(sampler_heap->type  == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        ID3D12DescriptorHeap *heaps[] = {
            resource_heap->native_heap,
            sampler_heap->native_heap,
        };
        native_cmd_list->SetDescriptorHeaps(_countof(heaps), heaps);
    }

    void DX12_Command_List::set_graphics_root_signature(ID3D12RootSignature* root_signature)
    {
        native_cmd_list->SetGraphicsRootSignature(root_signature);
    }

    void DX12_Command_List::set_graphics_root_constants(u32 root_parameter_index, u32 count, void* data)
    {
        native_cmd_list->SetGraphicsRoot32BitConstants(root_parameter_index, count, data, 0);
    }

    void DX12_Command_List::set_viewport(int top_left_x, int top_left_y, int width, int height) {
        D3D12_VIEWPORT viewport = {
            .TopLeftX = (FLOAT)top_left_x,
            .TopLeftY = (FLOAT)top_left_y,
            .Width    = (FLOAT)width,
            .Height   = (FLOAT)height,
            .MinDepth = D3D12_MIN_DEPTH,
            .MaxDepth = D3D12_MAX_DEPTH
        };
        native_cmd_list->RSSetViewports(1, &viewport);
    }

    void DX12_Command_List::set_scissor(int top_left_x, int top_left_y, int width, int height) {
        D3D12_RECT rect = {
            .left = top_left_x,
            .top  = top_left_y,
            .right = top_left_x + width,
            .bottom = top_left_y + height,
        };
        native_cmd_list->RSSetScissorRects(1, &rect);
    }

    void DX12_Command_List::set_topology(D3D12_PRIMITIVE_TOPOLOGY topology)
    {
        native_cmd_list->IASetPrimitiveTopology(topology);
    }

    void DX12_Command_List::set_render_target(u32 num_rtvs, DX12_Descriptor** rtvs, DX12_Descriptor* dsv)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_handles[32];
        assert(num_rtvs < _countof(rtv_handles));
        for (u32 i = 0; i < num_rtvs; ++i) { rtv_handles[i] = rtvs[i]->cpu_handle; }
        D3D12_CPU_DESCRIPTOR_HANDLE* dsv_handle = dsv ? &dsv->cpu_handle : nullptr;
        native_cmd_list->OMSetRenderTargets(num_rtvs, rtv_handles, false, dsv_handle);
    }

    void DX12_Command_List::set_index_buffer(DX12_Resource* resource)
    {
        DXGI_FORMAT format = DXGI_FORMAT_R32_UINT;
        D3D12_INDEX_BUFFER_VIEW view = {
            .BufferLocation = resource->native_resource->GetGPUVirtualAddress(),
            .SizeInBytes    = static_cast<UINT>(resource->desc.buffer.size),
            .Format         = format
        };
        native_cmd_list->IASetIndexBuffer(&view);
    }

    void DX12_Command_List::draw(u32 num_vertices, u32 num_instances, u32 begin_vertex, u32 begin_instance)
    {
        native_cmd_list->DrawInstanced(num_vertices, num_instances, begin_vertex, begin_instance);
    }

    void DX12_Command_List::draw_indexed(u32 num_indices_per_instance, u32 num_instances, u32 start_index, int base_vertex, u32 start_instance)
    {
        native_cmd_list->DrawIndexedInstanced(num_indices_per_instance, num_instances, start_index, base_vertex, start_instance);
    }

    ENGINE_API void dx12_execute_command_list(DX12_Command_Queue* cmd_queue, DX12_Command_List* cmd_list)
    {
        ID3D12CommandList* lists[] = { cmd_list->native_cmd_list };
        cmd_queue->native_cmd_queue->ExecuteCommandLists(1, lists);
    }

    ENGINE_API void dx12_signal_fence(DX12_Command_Queue* cmd_queue, DX12_Fence* fence)
    {
        cmd_queue->native_cmd_queue->Signal(fence->native_fence, ++fence->value);
    }

    ENGINE_API void dx12_wait_fence(DX12_Fence* fence)
    {
        if (fence->value > fence->native_fence->GetCompletedValue()) {
            fence->native_fence->SetEventOnCompletion(fence->value, fence->event);
            WaitForSingleObject(fence->event, INFINITE);
        }
    }

    void DX12_Swap_Chain::present()
    {
        UINT sync_interval = 0;
        native_swap_chain->Present(sync_interval, 0); // @Temporary: flags?
        current_frame_index = native_swap_chain->GetCurrentBackBufferIndex();
    }

    ID3D12Resource* DX12_Swap_Chain::get_current_resource()
    {
        return resources[current_frame_index];
    }

    DX12_Descriptor* DX12_Swap_Chain::get_current_rtv()
    {
        return rtvs + current_frame_index;
    }

    ENGINE_API DX12_Resource* dx12_alloc_resource(DX12_Device* device, DX12_Resource_Desc desc)
    {
        DX12_Resource* result = nullptr;
        ID3D12Resource* resource = nullptr;
        D3D12_HEAP_PROPERTIES heap_prop = {
            .Type                 = desc.heap_type,
            .CPUPageProperty      = desc.cpu_page_property,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask     = 1,
            .VisibleNodeMask      = 1
        };
        D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_COMMON;
        D3D12_RESOURCE_DESC res_desc;
        bool ok = false;
        bool should_clear = ((desc.resource_flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ||
                             (desc.resource_flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) && desc.do_clear;
        D3D12_CLEAR_VALUE* clear_value = should_clear ? &desc.clear_value : nullptr;


        if (desc.type == DX12_RESOURCE_TYPE_BUFFER) {
            auto buffer = desc.buffer;
            res_desc = {
                .Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER,
                .Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
                .Width            = buffer.size,
                .Height           = 1,
                .DepthOrArraySize = 1,
                .MipLevels        = 1,
                .Format           = DXGI_FORMAT_UNKNOWN,
                .SampleDesc       = {
                    .Count   = 1,
                    .Quality = 0
                },
                .Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags            = desc.resource_flags,
            };
            
            if (SUCCEEDED(device->native_device->CreateCommittedResource(&heap_prop, desc.heap_flags, &res_desc, init_state, clear_value, IID_PPV_ARGS(&resource)))) {
                ok = true;
            }

        } else if (desc.type == DX12_RESOURCE_TYPE_TEXTURE_2D) {
            auto tex = desc.texture;
            res_desc = {
                .Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, // @Todo: Correct?
                .Width            = tex.width,
                .Height           = tex.height,
                .DepthOrArraySize = tex.depth,
                .MipLevels        = tex.mip_levels,
                .Format           = tex.format,
                .SampleDesc       = {
                    .Count   = tex.num_samples,
                    .Quality = 0
                },
                .Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags            = desc.resource_flags,
            };

            if (SUCCEEDED(device->native_device->CreateCommittedResource(&heap_prop, desc.heap_flags, &res_desc, init_state, clear_value, IID_PPV_ARGS(&resource)))) {
                ok = true;
            }
        } else {
            assert("invalid resource creation type.");
        }

        if (ok) {
            result = new DX12_Resource;
            result->desc = desc;
            result->native_resource = resource;
            resource->SetName(L"HELLO");
        }

        return result;
    }

    ENGINE_API void dx12_dealloc_resource(DX12_Resource* resource)
    {
        if (resource) {
            if (resource->native_resource) { resource->native_resource->Release(); }
        }
    }

    ENGINE_API ID3D12RootSignature* dx12_create_bindless_root_signature(DX12_Device* device)
    {
        ID3D12RootSignature* result = nullptr;

        D3D12_ROOT_PARAMETER1 root_params[1] = {};
        root_params[0] = {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants = {
                .ShaderRegister = 0,
                .RegisterSpace  = 0,
                .Num32BitValues = 36 // @Todo: wha's the big enough value
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

        if (FAILED(D3D12SerializeVersionedRootSignature(&root_signature_desc, &root_signature_blob, &error_blob))) {

        }

        if (FAILED(device->native_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&result)))) {

        }

        // Cleanup
        if (root_signature_blob) { root_signature_blob->Release(); }
        if (error_blob)          { error_blob->Release(); }

        return result;
    }

    ENGINE_API DX12_Pipeline_State dx12_create_graphics_pipeline_state(DX12_Device* device, const DX12_Graphics_Pipeline_Desc& desc)
    {
        DX12_Pipeline_State result = {};

        ID3D12PipelineState* pso = nullptr;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {
            .pRootSignature = desc.root_signature,

            .VS = {
                .pShaderBytecode = desc.vs_bytecode,
                .BytecodeLength  = desc.vs_length
            },
            .PS = {
                .pShaderBytecode = desc.ps_bytecode,
                .BytecodeLength  = desc.ps_length
            },

            // @Temporary
            .BlendState = {
                .AlphaToCoverageEnable  = FALSE,
                .IndependentBlendEnable = FALSE,
                .RenderTarget = {
                    { 
                        .BlendEnable           = TRUE,
                        .LogicOpEnable         = FALSE,

                        .SrcBlend              = D3D12_BLEND_SRC_ALPHA,
                        .DestBlend             = D3D12_BLEND_INV_SRC_ALPHA,
                        .BlendOp               = D3D12_BLEND_OP_ADD,

                        .SrcBlendAlpha         = D3D12_BLEND_ONE,
                        .DestBlendAlpha        = D3D12_BLEND_ZERO,
                        .BlendOpAlpha          = D3D12_BLEND_OP_ADD,

                        .LogicOp               = D3D12_LOGIC_OP_NOOP,
                        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
                    }
                }
            },
            .SampleMask = 0xff,
            .RasterizerState = {
                .FillMode              = D3D12_FILL_MODE_SOLID,
                .CullMode              = desc.cull_mode,
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
                .DepthEnable    = desc.depth_enabled,
                .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
                .DepthFunc      = D3D12_COMPARISON_FUNC_LESS,

                .StencilEnable  = FALSE, 
                //UINT8 StencilReadMask;
                //UINT8 StencilWriteMask;
                //D3D12_DEPTH_STENCILOP_DESC FrontFace;
                //D3D12_DEPTH_STENCILOP_DESC BackFace;
            },
            .InputLayout = {
                .pInputElementDescs = desc.input_elements,
                .NumElements        = desc.num_input_elements
            },
            .PrimitiveTopologyType = desc.topology,

            .SampleDesc = {
                .Count = 1,
                .Quality = 0
            },

            // .CachedPSO;

            .Flags = D3D12_PIPELINE_STATE_FLAG_NONE // @Study: D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG
        };

        pso_desc.NumRenderTargets = desc.num_render_targets;
        pso_desc.DSVFormat        = desc.dsv_format;
        memcpy(pso_desc.RTVFormats, desc.rtv_formats, sizeof(desc.rtv_formats));

        if (SUCCEEDED(device->native_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso)))) {

        }

        result.pso = pso;
        result.desc = desc;

        return result;
    }

    ENGINE_API void dx12_create_dsv(DX12_Device* device, DX12_Resource* resource, DX12_Descriptor* descriptor, DXGI_FORMAT format)
    {
        assert(descriptor->my_heap->type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
            .Format        = format,
            .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
            .Flags         = D3D12_DSV_FLAG_NONE,
            .Texture2D = {
                .MipSlice = 0
            }
        };
        device->native_device->CreateDepthStencilView(resource->native_resource, &dsv_desc, descriptor->cpu_handle);
    }

    ENGINE_API void dx12_create_srv(DX12_Device* device, DX12_Resource* resource, DX12_Descriptor* descriptor, u32 num_elements, u32 stride_in_bytes)
    {
        if (resource->desc.type == DX12_RESOURCE_TYPE_BUFFER) {
            auto srv_desc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer(num_elements, stride_in_bytes);
            device->native_device->CreateShaderResourceView(resource->native_resource, &srv_desc, descriptor->cpu_handle);

        } else if (resource->desc.type == DX12_RESOURCE_TYPE_TEXTURE_2D) {
            auto tex = resource->desc.texture;
            auto srv_desc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(tex.format, tex.mip_levels);
            device->native_device->CreateShaderResourceView(resource->native_resource, &srv_desc, descriptor->cpu_handle);

        } else {
            assert(0);
        }
    }

    ENGINE_API void dx12_upload_buffer(DX12_Device *device, DX12_Command_Queue* cmd_queue, DX12_Command_List* cmd_list, DX12_Fence* fence, DX12_Resource* resource, void* data, u64 size)
    {
        const u32 subresource        = 0;
        const u32 num_subresource    = 1;
        const u64 upload_buffer_size = GetRequiredIntermediateSize(resource->native_resource, subresource, num_subresource);
        DX12_Resource* upload_buffer = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_BUFFER, .heap_type = D3D12_HEAP_TYPE_UPLOAD, .buffer = { .size = upload_buffer_size } });

        const D3D12_RANGE read_range = { 0, 0 };
        void* ptr;
        upload_buffer->native_resource->Map(0, &read_range, &ptr);
        memcpy(ptr, data, size);
        upload_buffer->native_resource->Unmap(0, nullptr);

        cmd_list->begin();
        {
            cmd_list->transition_barrier(resource->native_resource, 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            cmd_list->transition_barrier(upload_buffer->native_resource, 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

            cmd_list->native_cmd_list->CopyBufferRegion(resource->native_resource, 0, upload_buffer->native_resource, 0, size);

            cmd_list->transition_barrier(resource->native_resource, 0, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
            cmd_list->transition_barrier(upload_buffer->native_resource, 0, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
        }
        cmd_list->end();

        dx12_execute_command_list(cmd_queue, cmd_list);

        dx12_signal_fence(cmd_queue, fence);
        dx12_wait_fence(fence);

        dx12_dealloc_resource(upload_buffer);
    }
}
