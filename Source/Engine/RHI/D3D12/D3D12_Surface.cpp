// Copyright Seong Woo Lee. All Rights Reserved.

#include "D3D12_Surface.h"
#include "D3D12_Device.h"
#include "Core/Core_Log.h"
#include "Window/Window.h"

#include "ThirdParty/DirectX/Include/d3d12.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dxgi1_6.h>

namespace Engine
{
    D3D12_Surface::D3D12_Surface(D3D12_Device* device, Window* window)
        : m_device(device)
    {
        // https://devblogs.microsoft.com/directx/dxgi-flip-model/
        // https://docs.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect
        //
        // BufferCount must be at least 2 to use flip model.
        // Also, you must resolve msaa elsewhere...?

        HWND hwnd = (HWND)window->GetPlatformWindow();

        u32 width  = window->GetWidth();
        u32 height = window->GetHeight();

        // @Temporary: Tearing??
        UINT swap_chain_flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        // @Temporary: V-Sync?
        UINT num_buffers = g_num_frames_in_flight;

        DXGI_SWAP_CHAIN_DESC1 desc = {
            .Width  = width,
            .Height = height,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = {
                .Count = 1,
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = num_buffers,
            .Scaling = DXGI_SCALING_NONE,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 
            .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
            .Flags = swap_chain_flags
        };

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {
            .Windowed = TRUE
        };

        IDXGISwapChain1* swap_chain = nullptr;
        HRESULT hr = device->m_factory->CreateSwapChainForHwnd(device->m_device, hwnd, &desc, &fullscreen_desc, nullptr, &swap_chain);
        CORE_ASSERT(SUCCEEDED(hr), "Failed to create swap chain");

        CORE_ASSERT(SUCCEEDED(swap_chain->QueryInterface(IID_PPV_ARGS(&m_swap_chain))), "Failed to query IDXGISwapChain3");

        swap_chain->Release();
        swap_chain = nullptr;

        Log("Created D3D12 Surface.");
    }

    D3D12_Surface::~D3D12_Surface()
    {
    }
}
