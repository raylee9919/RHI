// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Basics.h"
#include "DX12/DX12.h"
#include "VK/VK.h"

namespace Engine
{
    enum RHI_Type {
        RHI_TYPE_INVALID = 0,
        RHI_TYPE_DX12,
        RHI_TYPE_VK,
    };

    struct RHI_Device {
        RHI_Type type;
        union {
            DX12_Device dx12_device;
        };
    };

    enum RHI_Command_List_Type {
        RHI_CMD_LIST_TYPE_DIRECT,
        RHI_CMD_LIST_TYPE_COMPUTE,
        RHI_CMD_LIST_TYPE_COPY,
        RHI_CMD_LIST_TYPE_VIDEO_ENCODE,
        RHI_CMD_LIST_TYPE_VIDEO_DECODE,
        RHI_CMD_LIST_TYPE_VIDEO_PROCESS,
    };

    struct RHI_Command_Queue {
        RHI_Type type;
        union {
            DX12_Command_Queue dx12_cmd_queue;
        };
    };

    struct RHI_Command_List {
        RHI_Type type;
        union {
            DX12_Command_List dx12_cmd_list;
        };
    };

    struct RHI_Fence {
        RHI_Type type;
        union {
            DX12_Fence dx12_fence;
        };
    };

    ENGINE_API bool rhi_init_device(RHI_Type type, RHI_Device* device, bool use_debug);
    ENGINE_API bool rhi_deinit_device(RHI_Device* device);

    ENGINE_API bool rhi_init_command_queue(RHI_Device* device, RHI_Command_Queue* cmd_queue, RHI_Command_List_Type cmd_type);
    ENGINE_API bool rhi_deinit_command_queue(RHI_Command_Queue* cmd_queue);

    ENGINE_API bool rhi_init_command_list(RHI_Device* device, RHI_Command_List* cmd_list, RHI_Command_List_Type cmd_type);
    ENGINE_API bool rhi_deinit_command_list(RHI_Command_List* cmd_list);

    ENGINE_API bool rhi_init_fence(RHI_Device* device, RHI_Fence* fence);
    ENGINE_API bool rhi_deinit_fence(RHI_Fence* fence);
}
