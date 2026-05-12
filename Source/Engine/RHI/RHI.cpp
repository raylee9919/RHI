// Copyright Seong Woo Lee. All Rights Reserved.

#include "RHI.h"

namespace Engine
{
    ENGINE_API bool rhi_init_device(RHI_Type type, RHI_Device* device, bool debug)
    {
        if (!device) return false;

        if (type == RHI_TYPE_DX12) { return dx12_init_device(&device->dx12_device, debug); }
        else { CORE_ASSERT(0, "Not implemented yet!"); }

        return true;
    }

    ENGINE_API bool rhi_deinit_device(RHI_Device* device)
    {
        if (!device) return false;
        
        RHI_Type type = device->type;
        if (type == RHI_TYPE_DX12) { return dx12_deinit_device(&device->dx12_device); }
        else { CORE_ASSERT(0, "Not implemented yet!"); }

        return true;
    }

    INTERNAL D3D12_COMMAND_LIST_TYPE to_d3d12_command_list_type(RHI_Command_List_Type type)
    {
        switch(type) {
            INVALID_DEFAULT_CASE;
            case RHI_CMD_LIST_TYPE_DIRECT:        return D3D12_COMMAND_LIST_TYPE_DIRECT;
            case RHI_CMD_LIST_TYPE_COMPUTE:       return D3D12_COMMAND_LIST_TYPE_COMPUTE;
            case RHI_CMD_LIST_TYPE_COPY:          return D3D12_COMMAND_LIST_TYPE_COPY;
            case RHI_CMD_LIST_TYPE_VIDEO_ENCODE:  return D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE;
            case RHI_CMD_LIST_TYPE_VIDEO_DECODE:  return D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE;
            case RHI_CMD_LIST_TYPE_VIDEO_PROCESS: return D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS;
        }
    }

    ENGINE_API bool rhi_init_command_queue(RHI_Device* device, RHI_Command_Queue* cmd_queue, RHI_Command_List_Type type)
    {
        if (!device || !cmd_queue) return false;

        RHI_Type rhi_type = device->type;
        cmd_queue->type = rhi_type;

        if (rhi_type == RHI_TYPE_DX12) { return dx12_init_command_queue(&device->dx12_device, &cmd_queue->dx12_cmd_queue, to_d3d12_command_list_type(type)); }
        else { CORE_ASSERT(0, "Not implemented yet!"); }

        return true;
    }

    ENGINE_API bool rhi_deinit_command_queue(RHI_Command_Queue* cmd_queue)
    {
        if (!cmd_queue) return false;

        RHI_Type type = cmd_queue->type;

        if (type == RHI_TYPE_DX12) { return dx12_deinit_command_queue(&cmd_queue->dx12_cmd_queue); }
        else { CORE_ASSERT(0, "Not implemented yet!"); }

        return false;
    }

    ENGINE_API bool rhi_init_command_list(RHI_Device* device, RHI_Command_List* cmd_list, RHI_Command_List_Type cmd_type)
    {
        if (!device || !cmd_list) return false;
        
        RHI_Type type = device->type;
        cmd_list->type = type;

        if (type == RHI_TYPE_DX12) { return dx12_init_command_list(&device->dx12_device, &cmd_list->dx12_cmd_list, to_d3d12_command_list_type(cmd_type)); }
        else { CORE_ASSERT(0, "Not implemented yet!"); }

        return false;
    }

    ENGINE_API bool rhi_deinit_command_list(RHI_Command_List* cmd_list)
    {
        if (!cmd_list) return false;

        RHI_Type type = cmd_list->type;

        if (type == RHI_TYPE_DX12) { return dx12_deinit_command_list(&cmd_list->dx12_cmd_list); }
        else { CORE_ASSERT(0, "Not implemented yet!"); }

        return false;
    }

    ENGINE_API bool rhi_init_fence(RHI_Device* device, RHI_Fence* fence)
    {
        if (!device || !fence) return false;
        
        RHI_Type type = device->type;
        fence->type = type;

        if (type == RHI_TYPE_DX12) { return dx12_init_fence(&device->dx12_device, &fence->dx12_fence); }
        else { CORE_ASSERT(0, "Not implemented yet!"); }

        return false;
    }

    ENGINE_API bool rhi_deinit_fence(RHI_Fence* fence)
    {
        RHI_Type type = fence->type;

        if (type == RHI_TYPE_DX12) { return dx12_deinit_fence(&fence->dx12_fence); }
        else { CORE_ASSERT(0, "Not implemented yet!"); }

        return false;
    }
}
