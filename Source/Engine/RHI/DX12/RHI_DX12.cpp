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
    ENGINE_API bool DX12_InitDevice(DX12_Device* device, bool use_debug_layer)
    {
        using namespace Microsoft::WRL;

        DWORD dxgi_factory_flags = 0;

        if (use_debug_layer)
        {
            ID3D12Debug* debug_interface = nullptr;

            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface))))
            {
                ID3D12Debug1* debug_interface1 = nullptr;
                if (SUCCEEDED(debug_interface->QueryInterface(IID_PPV_ARGS(&debug_interface1))))
                {
                    debug_interface1->SetEnableGPUBasedValidation(true);
                    debug_interface1->SetEnableSynchronizedCommandQueueValidation(true);
                }
            }
            else
            {
                CORE_ASSERT(!"Unable to use debug layer.");
            }

            // @Todo: DRED?
        }

        IDXGIInfoQueue* dxgi_info_queue = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue))))
        {
            dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;

            dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
            dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);

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

        // Cleanup
        temp_device->Release();

        
        Log("Created DX12 device.");
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

    ENGINE_API bool DX12_InitSwapchain(DX12_Device* device, DX12_SwapChain* swap_chain, HWND hwnd, uint width, uint height, uint num_frames)
    {
        if (!device || !swap_chain)
        {
            return false;
        }

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
            .Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH, // @Temporary: Sure?
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

        if (FAILED(device->m_factory->CreateSwapChainForHwnd(device->m_device, hwnd, &desc, &fullscreen_desc, nullptr, &swap_chain->m_swap_chain)))
        {
            return false;
        }

        return true;
    }

    ENGINE_API void DX12_DeinitSwapchain(DX12_SwapChain* swap_chain)
    {
        if (swap_chain)
        {
            SafeReleaseCOM(&swap_chain->m_swap_chain);
            Log("Deinitted DX12 swap chain.");
        }
    }

    ENGINE_API bool DX12_InitDescriptorHeap(DX12_DescriptorHeap* descriptor_heap)
    {
        return true;
    }

    ENGINE_API void DX12_DeinitDescriptorHeap(DX12_DescriptorHeap* descriptor_heap)
    {
    }
}

