// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/Core_Common.h"

namespace Engine
{
    enum RHI_Kind
    {
        RHI_KIND_NULL   = 0,
        RHI_KIND_DX12   = 1,
        RHI_KIND_VULKAN = 2,
    };

    struct RHI_Device
    {
        RHI_Kind kind;
        union {
            struct DX12_Device* dx12;
        };
    };

    struct RHI_CommandQueue
    {
        RHI_Kind kind;
        union {
            struct DX12_CommandQueue* dx12;
        };
    };
}
