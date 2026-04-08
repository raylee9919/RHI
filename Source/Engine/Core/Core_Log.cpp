// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core_Log.h"
#include "Core_String.h"

#include <cstdarg>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Engine
{
    void Log(const char* fmt, ...)
    {
        // @Fix: BROKEN formatting!!
        //

        va_list args;
        va_start(args, fmt);

        va_list args_copy;
        va_copy(args_copy, args);

        int len = FormatCStringV(nullptr, 0, fmt, args);
        char* buf = new char[len + 1];

        FormatCStringV(buf, len + 1, fmt, args_copy);

        va_end(args_copy);
        va_end(args);

        OutputDebugStringA("[Log] ");
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");

        delete[] buf;
    }
}
