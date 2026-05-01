// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Math.h"

namespace Engine
{
    struct Input_System;

    namespace Render
    {

        enum class Texture_Format
        {
            RGBA8_UNORM
        };

        // @Important: Please sync with shaders.
        struct Material
        {
            // Bindless SRV indices
            s32 albedo;
            s32 normal;
            s32 orm;
            s32 emissive;
            s32 sampler;
        };

        // @Important: Constant buffer alignment!
        struct ENGINE_API Camera
        {
            m4x4 view;
            m4x4 proj;
            m4x4 view_proj;
            vec4 position;

            f32 aspect_ratio;
            f32 near_z;
            f32 far_z;
            f32 yaw;
            f32 pitch;
            f32 speed;
            f32 last_mouse_x;
            f32 last_mouse_y;


            void Update(f32 dt, Input_System* input);
        };
    }
}
