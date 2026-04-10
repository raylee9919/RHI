// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/Core_Common.h"
#include "Core/Core_String.h"

namespace Engine
{
    struct HLSL_Shader
    {
        uint8_t* source;
        uint64_t length;
        String   entry;
        String   target_profile;
    };
}
