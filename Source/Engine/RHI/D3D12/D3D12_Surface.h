// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "RHI/RHI_Surface.h"

struct IDXGISwapChain3;

namespace Engine
{
    class D3D12_Device;
    class Window;

    class D3D12_Surface final : public RHI_Surface
    {
        public:
            D3D12_Surface(D3D12_Device* device, Window* window);
            ~D3D12_Surface();

        private:
            D3D12_Device* m_device;
            IDXGISwapChain3* m_swap_chain;
    };
}
