// Copyright Seong Woo Lee. All Rights Reserved.

#include "Window.h"
#include "Core/SE_Log.h"

#include <SDL3/SDL.h>

namespace Engine
{
    Window* Window::Create(const char* title, int width, int height)
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);

        Window* window = new Window;

        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
        SDL_Window* sdl = SDL_CreateWindow(title, width, height, flags);

        window->sdl       = sdl;
        window->m_running = true;
        window->m_width   = width;
        window->m_height  = height;

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

        if (event.type == SDL_EVENT_QUIT) {
            m_running = false;
        }

        return ended;
    }
}
