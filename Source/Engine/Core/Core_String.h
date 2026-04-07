// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/Core_Common.h"

#include <string>

namespace Engine
{
    using String = std::string;

    ENGINE_API int FormatCStringV(char* buf, int len, const char* fmt, va_list args);
}
