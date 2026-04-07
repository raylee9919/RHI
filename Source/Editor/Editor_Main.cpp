// Copyright Seong Woo Lee. All Rights Reserved.

#include "Window/Window.h"
#include "OS/OS_Main.h"

#include "Core/Core_Module.h"

#include "RHI/D3D12/D3D12_Device.h"
#include "RHI/D3D12/D3D12_CommandQueue.h"

using namespace Engine;

int ENGINE_MAIN(int argc, const char** argv)
{
    Window* window = Window::Create("Hello", 1920, 1080);

    RHI_Device* device = new D3D12_Device(true);
    RHI_CommandQueue* cmd_queue = device->CreateCommandQueue(RHI_CommandType::DIRECT);

    while (window->IsOpen()) {
        window->PollEvents();
    }

    Window::Destroy(window);

    return 0;
}
