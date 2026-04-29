// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Math.h"

namespace Engine
{
    namespace Renderer
    {
        struct Camera 
        {
            m4x4 view;
            m4x4 proj;
            m4x4 view_proj;

            vec4 position;
        };
    }
}
