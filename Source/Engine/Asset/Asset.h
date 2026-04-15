// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Math.h"

namespace Engine
{
    struct Vertex
    {
        vec3 position;
        vec2 uv;
    };

    struct Triangle_Mesh
    {
        Vertex*     vertices;
        uint32_t    num_vertices;

        uint32_t*   indices;
        uint32_t    num_indices;
    };

    
}
