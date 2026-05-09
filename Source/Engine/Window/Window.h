// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Basics.h"
#include "Core/SE_String.h"

#include "Input/Input.h"

#include <windows.h>

struct SDL_Window;

namespace Engine 
{
    struct ENGINE_API Window
    {
        SDL_Window*     sdl;
        int             width;
        int             height;
        bool            is_running;
        Input_System*   my_input_system;

        void  poll_events();
        void* get_platform_window();
    };

    ENGINE_API Window*  create_window(String title, int width, int height);
    ENGINE_API void     destroy_window(Window* window);
}
