// Copyright Seong Woo Lee. All Rights Reserved.

#include "D3D12_CommandList.h"
#include "D3D12_CommandQueue.h"
#include "D3D12_Device.h"
#include "Core/Core_Common.h"
#include "Core/Core_Log.h"

#include <ThirdParty/DirectX/Include/d3d12.h>

namespace Engine
{
    D3D12_CommandList::D3D12_CommandList(D3D12_CommandQueue* queue)
        : m_cmd_queue(queue)
    {
        D3D12_COMMAND_LIST_TYPE type = ToD3D12CommandType(queue->m_type);
        auto device = queue->m_device->m_device;
        CORE_ASSERT(SUCCEEDED(device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_cmd_allocator))));
        CORE_ASSERT(SUCCEEDED(device->CreateCommandList(0, type, m_cmd_allocator, nullptr, IID_PPV_ARGS(&m_cmd_list))));

        Log("Created D3D12_CommandList.");
    }

    D3D12_CommandList::~D3D12_CommandList()
    {
        SafeReleaseCOM(&m_cmd_allocator);
        SafeReleaseCOM(&m_cmd_list);

        Log("Destroyed D3D12_CommandList.");
    }
}
