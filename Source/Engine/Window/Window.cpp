// Copyright Seong Woo Lee. All Rights Reserved.

#include "Window.h"
#include "Core/SE_Log.h"

#include <SDL3/SDL.h>

#include "ThirdParty/DearIMGUI/imgui.h"
#include "ThirdParty/DearIMGUI/imgui_impl_sdl3.h"
#include "ThirdParty/DearIMGUI/imgui_impl_dx12.h"

namespace Engine
{
    ENGINE_API Window* create_window(String title, int width, int height)
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);

        Window* window = new Window;

        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
        SDL_Window* sdl = SDL_CreateWindow(title.c_str(), width, height, flags);

        window->sdl             = sdl;
        window->width           = width;
        window->height          = height;
        window->is_running      = true;
        window->my_input_system = new Input_System;

        return window;
    }

    ENGINE_API void destroy_window(Window* window)
    {
        if (window) {
            SDL_DestroyWindow(window->sdl);
            delete window;
        }
    }

    void* Window::get_platform_window()
    {
#if PLATFORM_WINDOWS
        return SDL_GetPointerProperty(SDL_GetWindowProperties(sdl), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#else
#  error Undefined platform.
#endif
    }

    void Window::poll_events()
    {
        memcpy(my_input_system->mouse_was_down, my_input_system->mouse_is_down, sizeof(my_input_system->mouse_was_down[0]) * ARRAY_COUNT(my_input_system->mouse_was_down));
        memcpy(my_input_system->key_was_down, my_input_system->key_is_down, sizeof(my_input_system->key_was_down[0]) * ARRAY_COUNT(my_input_system->key_was_down));

        // Poll events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // @Temporary:
            ImGui_ImplSDL3_ProcessEvent(&event);

            // Quit
            if (event.type == SDL_EVENT_QUIT) { is_running = false; }

            ImGuiIO& io = ImGui::GetIO();

            if (event.type == SDL_EVENT_KEY_DOWN) {
                my_input_system->key_is_down[KeyFromSDL(event.key.key)] = true;
            }
            if (event.type == SDL_EVENT_KEY_UP) {
                my_input_system->key_is_down[KeyFromSDL(event.key.key)] = false;
            }

            if (!io.WantCaptureMouse) {
                SDL_MouseButtonFlags flags = SDL_GetMouseState(&my_input_system->current_mouse_x, &my_input_system->current_mouse_y);
                my_input_system->mouse_is_down[MOUSE_LEFT]   = (flags & SDL_BUTTON_MASK(SDL_BUTTON_LEFT));
                my_input_system->mouse_is_down[MOUSE_RIGHT]  = (flags & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT));
                my_input_system->mouse_is_down[MOUSE_MIDDLE] = (flags & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE));
            } else {
                // Clear mouse state so camera doesn't keep moving when ImGui takes over
                my_input_system->mouse_is_down[MOUSE_LEFT]   = false;
                my_input_system->mouse_is_down[MOUSE_RIGHT]  = false;
                my_input_system->mouse_is_down[MOUSE_MIDDLE] = false;
            }
        }
    }
}
