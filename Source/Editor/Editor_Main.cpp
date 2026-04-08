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

    while (window->IsOpen()) 
    {
        window->PollEvents();
    }

    // Cleanup
    //
    Window::Destroy(window);

    return 0;
}
