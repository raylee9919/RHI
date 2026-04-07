// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core_String.h"
#include "ThirdParty/stb/Include/stb_sprintf.h"

#include <cstdarg>

namespace Engine
{
    ENGINE_API int FormatCStringV(char* buf, int len, const char* fmt, va_list args)
    {
        return stbsp_vsnprintf(buf, len, fmt, args);
    }
};
