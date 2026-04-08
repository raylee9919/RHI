// Copyright Seong Woo Lee. All Rights Reserved.

#include "Window/Window.h"
#include "OS/OS_Main.h"

#include "RHI/DX12/RHI_DX12.h"


using namespace Engine;

int ENGINE_MAIN(int argc, const char** argv)
{
    Window* window = Window::Create("Hello", 1920, 1080);

    DX12_Device* device = new DX12_Device;
    DX12_InitDevice(device, true);

    DX12_CommandQueue* cmd_queue = new DX12_CommandQueue;
    DX12_InitCommandQueue(device, cmd_queue);

    HWND hwnd = (HWND)window->GetPlatformWindow();
    uint width  = 1920;
    uint height = 1080;
    uint num_frames = 3;
    DX12_SwapChain* swap_chain = new DX12_SwapChain;
    DX12_InitSwapchain(device, swap_chain, cmd_queue, hwnd, width, height, num_frames);

    DX12_DescriptorHeap* rtv_desc_heap = new DX12_DescriptorHeap;
    DX12_InitDescriptorHeap(device, rtv_desc_heap, 128, RHI_DESCRIPTOR_KIND_CBV_SRV_UAV);


    while (window->IsOpen()) 
    {
        window->PollEvents();
    }

    // Cleanup
    //
    Window::Destroy(window);

    return 0;
}
