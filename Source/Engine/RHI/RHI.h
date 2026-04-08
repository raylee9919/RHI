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

    enum RHI_DescriptorKind
    {
        RHI_DESCRIPTOR_KIND_NULL        = 0,
        RHI_DESCRIPTOR_KIND_CBV_SRV_UAV = 1,
        RHI_DESCRIPTOR_KIND_SAMPLER     = 2,
        RHI_DESCRIPTOR_KIND_RTV         = 3,
        RHI_DESCRIPTOR_KIND_DSV         = 4,
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
