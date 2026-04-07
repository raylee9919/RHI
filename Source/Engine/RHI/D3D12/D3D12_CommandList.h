// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "RHI/RHI_CommandList.h"

struct ID3D12CommandAllocator;
struct ID3D12CommandList;

namespace Engine
{
    class D3D12_Device;
    class D3D12_CommandQueue;

    class D3D12_CommandList final : public RHI_CommandList
    {
        public:
            D3D12_CommandList(D3D12_CommandQueue* queue);
            ~D3D12_CommandList();

        private:
            D3D12_CommandQueue*    m_cmd_queue;

            ID3D12CommandAllocator* m_cmd_allocator;
            ID3D12CommandList*     m_cmd_list;
    };
}
