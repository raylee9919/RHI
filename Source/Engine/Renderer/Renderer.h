// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Math.h"

namespace Engine
{
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
        struct Camera 
        {
            m4x4 view;
            m4x4 proj;
            m4x4 view_proj;

            vec4 position;
        };

        struct ENGINE_API IPass
        {
            virtual void Execute() = 0;

            Camera camera;
        };

        struct ENGINE_API PBR_Pass : IPass
        {
            virtual void Execute() override;
        };
    }
}
