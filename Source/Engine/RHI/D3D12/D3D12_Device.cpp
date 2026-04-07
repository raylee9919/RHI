// Copyright Seong Woo Lee. All Rights Reserved.

#include "RHI/D3D12/D3D12_Device.h"
#include "RHI/D3D12/D3D12_CommandQueue.h"
#include "Core/Core_Common.h"
#include "Core/Core_Log.h"
#include "ThirdParty/DirectX/Include/d3d12.h"

#include <dxgi1_6.h>

extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

    __declspec(dllexport) extern const u32 D3D12SDKVersion = 619;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\.";
}

namespace Engine
{
    D3D12_Device::D3D12_Device(bool debug)
    {
        IDXGIAdapter1* adapter1 = nullptr;
        DXGI_ADAPTER_DESC1 adapter_desc = {};

        ID3D12Debug* debug_controller = nullptr;
        DWORD create_factory_flags = 0;

        ID3D12Device12* device;

        HRESULT hr;

        if (debug) 
        {
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) 
            {
                debug_controller->EnableDebugLayer();
                create_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
            }
            else 
            {
                CORE_ASSERT(!"Failed to get debug interface");
            }

            // GBV
            ID3D12Debug5* debug_controller5 = nullptr;
            if (SUCCEEDED(debug_controller->QueryInterface(IID_PPV_ARGS(&debug_controller5))))
            {
                debug_controller5->SetEnableGPUBasedValidation(true);
                debug_controller5->SetEnableAutoName(true);
                debug_controller5->Release();
            }
            else
            {
                CORE_ASSERT(!"Failed to query ID3D12Debug5");
            }
        }


        IDXGIFactory6* factory6 = nullptr;
        CORE_ASSERT(SUCCEEDED(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&factory6))));

        D3D_FEATURE_LEVEL feature_levels[] = 
        {
            D3D_FEATURE_LEVEL_12_2,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };
        UINT num_levels = _countof(feature_levels);

        for (UINT i = 0; i < num_levels; ++i)
        {
            int adapter_index = 0;
            while (SUCCEEDED(factory6->EnumAdapters1(adapter_index, &adapter1)))
            {
                DXGI_ADAPTER_DESC1 desc;
                adapter1->GetDesc1(&desc);

                if (SUCCEEDED(D3D12CreateDevice(adapter1, feature_levels[i], IID_PPV_ARGS(&device))))
                {
                    goto lb_exit;
                }

                adapter1->Release();
                adapter1 = nullptr;
                ++adapter_index;
            }
        }

lb_exit:
        CORE_ASSERT(device);


        if (debug_controller)
        {
            ID3D12InfoQueue* info_queue = nullptr;
            device->QueryInterface(IID_PPV_ARGS(&info_queue));

            CORE_ASSERT(info_queue);

            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

            D3D12_MESSAGE_ID hide[] = {
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
                D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
            };

            D3D12_INFO_QUEUE_FILTER filter = {
                .DenyList = {
                    .NumIDs = _countof(hide),
                    .pIDList = hide,
                },
            };

            info_queue->AddStorageFilterEntries(&filter);
            info_queue->Release();
        }

        // Set
        //
        m_device = device;
        m_factory = factory6;


        // Cleanup
        //
        if (debug_controller)
        {
            debug_controller->Release();
            debug_controller = nullptr;
        }

        if (adapter1)
        {
            adapter1->Release();
            adapter1 = nullptr;
        }

        Log("Created D3D12Device.");
    }

    D3D12_Device::~D3D12_Device()
    {
        SafeReleaseCOM(&m_device);

        Log("Destroyed D3D12Device.");
    }

    RHI_CommandQueue* D3D12_Device::CreateCommandQueue(RHI_CommandType type)
    {
        RHI_CommandQueue* cmd_queue = new D3D12_CommandQueue(this, type);
        return cmd_queue;
    }
}
