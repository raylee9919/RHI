// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/Core_Common.h"

#include <windows.h>

struct SDL_Window;

namespace Engine 
{
    class ENGINE_API Window
    {
        public:
            static Window* Create(const char* title, int width, int height);
            static void Destroy(Window* window);

            FORCE_INLINE bool IsOpen()   { return (bool)m_running; }
            FORCE_INLINE u32 GetWidth()  { return m_width; }
            FORCE_INLINE u32 GetHeight() { return m_height; }

            void* GetPlatformWindow();

            bool PollEvents();


        private:
            SDL_Window* sdl;

            b32  m_running;
            u32  m_width;
            u32  m_height;
    };
}
