// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Basics.h"
#include "Core/SE_String.h"

namespace Engine
{
    namespace IO
    {
        ENGINE_API uint64_t ReadEntireFile(const String& path, void* buffer);
    }
}
