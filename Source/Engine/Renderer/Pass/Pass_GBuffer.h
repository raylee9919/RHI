// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Renderer/Renderer.h"

#include "Scene/Entity.h"

namespace Engine
{
    struct ENGINE_API GBuffer_Pass : IPass 
    {
        struct Push_Constants {
            u32 vertex_buffer_id;
            u32 transform_id;
            u32 material_id;

            u32 transforms_id;
            u32 camera_id;
            u32 anisotropic_sampler_id;
        };

        struct Draw_Data {
            Scene* world;
            Resource_State* resource_state;
            u32 transforms_id;
            u32 camera_id;
            u32 anisotropic_sampler_id;
        };

        virtual void draw(DX12_Command_List* cmd_list, void* param) override;
    };
}
