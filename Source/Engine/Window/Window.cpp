// Copyright Seong Woo Lee. All Rights Reserved.

#include "Window.h"

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

        return window;
    }

    void Window::Destroy(Window* window)
    {
        CORE_ASSERT(window);

        SDL_DestroyWindow(window->sdl);

        delete window;
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
