// Copyright Seong Woo Lee. All Rights Reserved.

#include "DX12.h"

#include "Core/SE_Basics.h"

#if 0
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

    __declspec(dllexport) extern const u32 D3D12SDKVersion = 619;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\.";
}
#endif

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
        dx12_safe_release_and_set_to_null(&dxgi_factory_6);
        dx12_safe_release_and_set_to_null(&dxgi_info_queue);
        dx12_safe_release_and_set_to_null(&debug_interface_5);
        dx12_safe_release_and_set_to_null(&debug_interface);

        auto* result = new DX12_Device;
        result->native_device = device;

        return result;
    }

    ENGINE_API void dx12_destroy_device(DX12_Device* device)
    {
        if (device) {
            if (device->native_device) { device->native_device->Release(); }
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

        DX12_Descriptor_Heap* result = new DX12_Descriptor_Heap;
        result->native_heap = heap;
        result->cpu_handle  = CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart());
        result->gpu_handle  = gpu_handle;
        result->max         = max_descriptors;
        result->num         = 0;

        return result;
    }

    ENGINE_API void dx12_destroy_descriptor_heap(DX12_Descriptor_Heap* heap)
    {
        if (heap) {
            if (heap->native_heap) { heap->native_heap->Release(); }
        }
    }

}
