// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Renderer/Renderer.h"

#include "Scene/Entity.h"

namespace Engine
{
    struct ENGINE_API Defer_Pass : IPass 
    {
        struct Push_Constants {
            u32 position_id;
            u32 normal_id;
            u32 uv_id;
            u32 material_id;

            u32 camera_id;
            u32 linear_sampler_id;
            u32 anisotropic_sampler_id;
        };

        struct Draw_Data {
            u32 camera_id;
            u32 linear_id;
            u32 anisotropic_id;
        };

        void execute(Resource_State* resource_state, DX12_Command_List* cmd_list, Draw_Data data);
    };
}
