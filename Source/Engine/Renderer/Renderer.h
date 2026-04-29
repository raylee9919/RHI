// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Math.h"

namespace Engine
{
    namespace Render
    {

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
