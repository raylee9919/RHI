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

    DX12_CommandList* cmd_list = new DX12_CommandList;
    DX12_InitCommandList(device, cmd_list, D3D12_COMMAND_LIST_TYPE_DIRECT);

    DX12_DescriptorHeap* rtv_heap = new DX12_DescriptorHeap;
    DX12_InitDescriptorHeap(device, rtv_heap, 53, RHI_DESCRIPTOR_KIND_RTV);

    HWND hwnd = (HWND)window->GetPlatformWindow();
    uint width  = 1920;
    uint height = 1080;
    uint num_frames = 3;
    DX12_SwapChain* swap_chain = new DX12_SwapChain;
    DX12_InitSwapChain(device, swap_chain, rtv_heap, cmd_queue, hwnd, width, height, num_frames);

    DX12_Fence* fence = new DX12_Fence;
    DX12_InitFence(device, fence);


    while (window->IsOpen()) 
    {
        window->PollEvents();

        DX12_BeginCommandList(cmd_list);
        {
            DX12_SetViewport(cmd_list, 0, 0, width, height);
            DX12_SetScissor(cmd_list, 0, 0, width, height);

            // @Temporary: Update current frame index!
            DX12_TransitionBarrier(cmd_list, swap_chain->m_resources[swap_chain->current_frame_index], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            // @Temporary: Update current frame index!
            DX12_ClearRTV(cmd_list, swap_chain->m_descriptors[swap_chain->current_frame_index], 0.0f, 0.2f, 0.4f, 1.0f);

            // @Temporary: Update current frame index!
            DX12_TransitionBarrier(cmd_list, swap_chain->m_resources[swap_chain->current_frame_index], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        }
        DX12_EndCommandList(cmd_list);
        DX12_ExecuteCommandList(cmd_queue, cmd_list);

        DX12_Present(swap_chain);

        DX12_PushFence(cmd_queue, fence);
        DX12_WaitForFence(fence);

        swap_chain->current_frame_index = swap_chain->m_swap_chain->GetCurrentBackBufferIndex();
    }

    // Cleanup
    //
    Window::Destroy(window);

    return 0;
}
