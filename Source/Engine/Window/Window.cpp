// Copyright Seong Woo Lee. All Rights Reserved.

#include "Window.h"
#include "Core/SE_Log.h"

#include <SDL3/SDL.h>

namespace Engine
{
    Window* Window::Create(const char* title, int width, int height, Input_System* input_system)
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);

        Window* window = new Window;

        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
        SDL_Window* sdl = SDL_CreateWindow(title, width, height, flags);

        window->sdl            = sdl;
        window->m_input_system = input_system;
        window->m_running      = true;
        window->m_width        = width;
        window->m_height       = height;

        Log("Created Window.");

        return window;
    }

    void Window::Destroy(Window* window)
    {
        CORE_ASSERT(window);

        SDL_DestroyWindow(window->sdl);

        delete window;

        Log("Destroyed Window.");
    }

    void* Window::GetPlatformWindow()
    {
#if PLATFORM_WINDOWS
        return SDL_GetPointerProperty(SDL_GetWindowProperties(sdl), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#else
#  error Undefined platform.
#endif
    }

    bool Window::PollEvents()
    {
        SDL_Event event;

        bool ended = SDL_PollEvent(&event);

        if (event.type == SDL_EVENT_QUIT) 
        {
            m_running = false;
        }

        if (m_input_system)
        {
            // Keyboard
            //
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                Key key = KeyFromSDL(event.key.key);
                m_input_system->key_is_down[key] = true;
            }

            if (event.type == SDL_EVENT_KEY_UP)
            {
                Key key = KeyFromSDL(event.key.key);
                m_input_system->key_is_down[key] = false;
            }

            // Mouse
            //
            memcpy(m_input_system->mouse_was_down, m_input_system->mouse_is_down, sizeof(m_input_system->mouse_was_down[0]) * ARRAY_COUNT(m_input_system->mouse_was_down));

            SDL_MouseButtonFlags flags = SDL_GetMouseState(&m_input_system->current_mouse_x, &m_input_system->current_mouse_y);
            m_input_system->mouse_is_down[MOUSE_LEFT]   = (flags & SDL_BUTTON_MASK(SDL_BUTTON_LEFT));
            m_input_system->mouse_is_down[MOUSE_RIGHT]  = (flags & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT));
            m_input_system->mouse_is_down[MOUSE_MIDDLE] = (flags & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE));
        }

        return ended;
    }
}
