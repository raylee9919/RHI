// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Renderer/Renderer.h"

#include "Scene/Entity.h"

namespace Engine
{
    struct ENGINE_API Composition_Pass : IPass 
    {
        struct Push_Constants {
            u32 color_id;
            u32 bilinear_clamp_id;
            u32 bloom_id;
            f32 bloom_strength;
        };

        void execute(Resource_State* resource_state, DX12_Command_List* cmd_list, Push_Constants push);
    };
}
