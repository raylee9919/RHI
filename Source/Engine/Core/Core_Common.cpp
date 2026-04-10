// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core_Common.h"

#if _MSC_VER
#  include <intrin.h>
#else
#  error Undefined compiler.
#endif

namespace Engine
{

    ENGINE_API uint32_t BitScanFromLSB(uint32_t x)
    {
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
