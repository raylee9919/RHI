// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "RHI/RHI_Device.h"

struct ID3D12Device12;
struct IDXGIFactory6;

namespace Engine
{
    class D3D12_CommandQueue;

    class D3D12_Device final : public RHI_Device
    {
        friend class D3D12_CommandQueue;
        friend class D3D12_CommandList;
        friend class D3D12_Surface;

        public:
            D3D12_Device(bool debug);
            ~D3D12_Device();

            virtual RHI_CommandQueue* CreateCommandQueue(RHI_CommandType type) override;

        protected:
            IDXGIFactory6* m_factory;
            ID3D12Device12* m_device;
    };
}
