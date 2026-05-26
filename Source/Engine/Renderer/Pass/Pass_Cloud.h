// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Renderer/Renderer.h"

#include "Scene/Entity.h"

namespace Engine
{
    struct ENGINE_API Cloud_Pass : IPass 
    {
        struct Push_Constants {
            u32 camera_id;

            u32 noise_id;
            u32 weather_map_id;

            u32 linear_wrap_id;

            f32  sun_illuminance;
            vec3 sun_direction;
            f32  sun_angular_radius;

            s32 num_view_samples;
            s32 num_sun_samples;
            s32 num_cloud_samples;
        };

        void execute(Resource_State* resource_state, DX12_Command_List* cmd_list, Push_Constants push);
    };
}
