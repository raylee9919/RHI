// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "RHI/RHI_Device.h"

struct ID3D12Device12;

namespace Engine
{
    class D3D12_Device final : public RHI_Device
    {
        public:
            D3D12_Device(bool debug);
            ~D3D12_Device();

        private:
            ID3D12Device12* m_device;
    };
}
