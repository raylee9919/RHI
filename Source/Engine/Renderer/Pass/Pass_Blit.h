// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Renderer/Renderer.h"

#include "Scene/Entity.h"

namespace Engine
{
    struct ENGINE_API Blit_Pass : IPass 
    {
        struct Push_Constants {
            u32 color_id;
            u32 linear_sampler_id;
        };

        struct Draw_Data {
            u32 linear_sampler_id;
        };

        virtual void draw(DX12_Command_List* cmd_list, void* param) override;
    };
}
