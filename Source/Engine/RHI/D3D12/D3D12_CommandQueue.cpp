// Copyright Seong Woo Lee. All Rights Reserved.

#include "D3D12_CommandQueue.h"
#include "D3D12_CommandList.h"
#include "D3D12_Device.h"
#include "Core/Core_Common.h"

#include <ThirdParty/DirectX/Include/d3d12.h>

namespace Engine
{
    D3D12_CommandQueue::D3D12_CommandQueue(D3D12_Device* device, RHI_CommandType type)
        : m_device(device)
    {
        m_type = type;

        ID3D12CommandQueue* queue = nullptr;

        D3D12_COMMAND_QUEUE_DESC desc = {
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0,
        };

        switch (type)
        {
            case RHI_CommandType::DIRECT:       desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; break;
            case RHI_CommandType::COMPUTE:      desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE; break;
            case RHI_CommandType::COPY:         desc.Type = D3D12_COMMAND_LIST_TYPE_COPY; break;
            case RHI_CommandType::VIDEO_ENCODE: desc.Type = D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE; break;
            case RHI_CommandType::VIDEO_DECODE: desc.Type = D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE; break;
            default: CORE_ASSERT(0, "Undefined command queue type");
        }

        HRESULT hr = device->m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_cmd_queue));
        CORE_ASSERT(SUCCEEDED(hr), "Failed to create command queue");

        m_cmd_queue = queue;
    }

    D3D12_CommandQueue::~D3D12_CommandQueue()
    {
        if (m_cmd_queue)
        {
            m_cmd_queue->Release();
            m_cmd_queue = nullptr;
        }
    }

    RHI_CommandList* D3D12_CommandQueue::CreateCommandList()
    {
        D3D12_CommandList* list = new D3D12_CommandList(this);
        return list;
    }

    ENGINE_API D3D12_COMMAND_LIST_TYPE ToD3D12CommandType(RHI_CommandType type)
    {
        switch (type)
        {
            case RHI_CommandType::DIRECT:       return D3D12_COMMAND_LIST_TYPE_DIRECT;
            case RHI_CommandType::COMPUTE:      return D3D12_COMMAND_LIST_TYPE_COMPUTE;
            case RHI_CommandType::COPY:         return D3D12_COMMAND_LIST_TYPE_COPY;
            case RHI_CommandType::VIDEO_ENCODE: return D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE;
            case RHI_CommandType::VIDEO_DECODE: return D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE;
            default: CORE_ASSERT(0, "Undefined RHI command type"); return {};
        }
    }
}
