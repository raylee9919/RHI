// Copyright Seong Woo Lee. All Rights Reserved.

#include "gfx.h"

namespace Engine
{
    using namespace DX12;

    namespace GFX
    {
        ENGINE_API bool Init(State* state, Device* device, CommandList* cmd_list, CommandQueue* cmd_queue, Fence* fence, DescriptorHeap* cbv_srv_uav_heap, DescriptorHeap* sampler_heap)
        {
            if (state)
            {
                state->device           = device;
                state->cmd_list         = cmd_list;
                state->cmd_queue        = cmd_queue;
                state->fence            = fence;
                state->cbv_srv_uav_heap = cbv_srv_uav_heap;
                state->sampler_heap     = sampler_heap;

                return true;
            }
            else
            {
                return false;
            }
        }

        ENGINE_API std::pair<Buffer, Descriptor> AllocStructuredBuffer(GFX::State* state, void* data, u64 stride, u64 count)
        {
            std::pair<Buffer, Descriptor> result;
            result.second = AllocDescriptor(state->cbv_srv_uav_heap);

            u64 size = stride * count;

            result.first = Malloc(state->device, {.size = size, .heap_kind = RHI_HEAP_KIND_DEFAULT});

            u64 staging_buffer_size = GetRequiredIntermediateSize(result.first.m_resource, 0, 1);
            auto staging_buffer = Malloc(state->device, { .size = staging_buffer_size, .heap_kind = RHI_HEAP_KIND_UPLOAD });

            void* ptr = Map(staging_buffer);
            memcpy(ptr, data, size);
            Unmap(staging_buffer);

            BeginCommandList(state->cmd_list);
            {
                auto prev1 = CmdTransitionBarrier(state->cmd_list, &result.first, D3D12_RESOURCE_STATE_COPY_DEST);
                auto prev2 = CmdTransitionBarrier(state->cmd_list, &staging_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
                CmdCopy(state->cmd_list, result.first, staging_buffer, size);
                CmdTransitionBarrier(state->cmd_list, &result.first, prev1);
                CmdTransitionBarrier(state->cmd_list, &staging_buffer, prev2);
            }
            EndCommandList(state->cmd_list);
            ExecuteCommandList(state->cmd_queue, state->cmd_list);

            PlaceFence(state->cmd_queue, state->fence);
            WaitForFence(state->fence);

            // Create srv.
            CD3DX12_SHADER_RESOURCE_VIEW_DESC desc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer(count, stride);
            state->device->m_device->CreateShaderResourceView(result.first.m_resource, &desc, result.second.cpu_handle);

            // Cleanup
            Free(staging_buffer);

            return result;
        }

        ENGINE_API Buffer AllocRawBuffer(GFX::State* state, void* data, u64 size)
        {
            Buffer buffer;

            buffer = Malloc(state->device, {.size = size, .heap_kind = RHI_HEAP_KIND_DEFAULT});

            u64 staging_buffer_size = GetRequiredIntermediateSize(buffer.m_resource, 0, 1);
            auto staging_buffer = Malloc(state->device, { .size = staging_buffer_size, .heap_kind = RHI_HEAP_KIND_UPLOAD });

            void* ptr = Map(staging_buffer);
            memcpy(ptr, data, size);
            Unmap(staging_buffer);

            BeginCommandList(state->cmd_list);
            {
                auto prev1 = CmdTransitionBarrier(state->cmd_list, &buffer, D3D12_RESOURCE_STATE_COPY_DEST);
                auto prev2 = CmdTransitionBarrier(state->cmd_list, &staging_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
                CmdCopy(state->cmd_list, buffer, staging_buffer, size);
                CmdTransitionBarrier(state->cmd_list, &buffer, prev1);
                CmdTransitionBarrier(state->cmd_list, &staging_buffer, prev2);
            }
            EndCommandList(state->cmd_list);
            ExecuteCommandList(state->cmd_queue, state->cmd_list);

            PlaceFence(state->cmd_queue, state->fence);
            WaitForFence(state->fence);

            // Cleanup
            Free(staging_buffer);

            return buffer;
        }
    }
}
