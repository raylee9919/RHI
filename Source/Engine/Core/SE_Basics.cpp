// Copyright Seong Woo Lee. All Rights Reserved.

#include "SE_Basics.h"

#if _MSC_VER
#  include <intrin.h>
#else
#  error Undefined compiler.
#endif

namespace Engine
{

    ENGINE_API uint32_t BitScanFromLSB(uint32_t x)
    {
        // @Todo: Since I'm compiling in C++20, I might want to just use 'std::countr_zero()'
        //
#ifdef _MSC_VER
        uint32_t index;
        if (BitScanForward((unsigned long*)&index, (unsigned int)x) == 0)
        {
            index = 32;
        }
        return index;
#else
#  error Undefined compiler.
#endif
    }


}
