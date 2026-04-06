// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/Core_Common.h"

#include <string>

namespace Engine
{
    using String = std::string;

    ENGINE_API void FormatCString(const char** fmt, ...);
}
