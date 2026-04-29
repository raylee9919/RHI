// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Basics.h"

#include "RHI/DX12/RHI_DX12.h"

namespace Engine
{
    using namespace DX12;

    namespace GFX
    {
        struct State
        {
            Device* device;
            CommandList* cmd_list;
            CommandQueue* cmd_queue;
            DescriptorHeap* cbv_srv_uav_heap;
            Fence* fence;
        };

        ENGINE_API bool Init(State* state, Device* device, CommandList* cmd_list, CommandQueue* cmd_queue, Fence* fence, DescriptorHeap* cbv_srv_uav_heap);

        ENGINE_API std::pair<Buffer, Descriptor> AllocStructuredBuffer(State* state, void* data, u64 stride, u64 count);

        ENGINE_API Buffer AllocRawBuffer(State* state, void* data, u64 size);
    }
}
