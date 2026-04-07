// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "RHI/RHI_CommandQueue.h"
#include "Core/Core_Common.h"

#include <ThirdParty/DirectX/Include/d3d12.h>

struct ID3D12CommandQueue;

namespace Engine
{
    class D3D12_Device;
    class D3D12_CommandList;

    class D3D12_CommandQueue final : public RHI_CommandQueue
    {
        friend class D3D12_CommandList;

        public:
            D3D12_CommandQueue(D3D12_Device* device, RHI_CommandType type);
            ~D3D12_CommandQueue();

            RHI_CommandList* CreateCommandList() override;

        private:
            RHI_CommandType m_cmd_type;
            D3D12_Device* m_device;
            ID3D12CommandQueue* m_cmd_queue;
    };

    ENGINE_API D3D12_COMMAND_LIST_TYPE ToD3D12CommandType(RHI_CommandType type);
}
