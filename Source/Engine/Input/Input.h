// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Basics.h"

#include <SDL3/SDL.h>

namespace Engine
{
    typedef u16 Key;
    enum
    {
        KEY_NULL = 0,

        KEY_TAB,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_UP,
        KEY_DOWN,
        KEY_PAGE_UP,
        KEY_PAGE_DOWN,
        KEY_HOME,
        KEY_END,
        KEY_INSERT,
        KEY_DELETE,
        KEY_BACKSPACE,
        KEY_SPACE,
        KEY_RETURN,
        KEY_ESC,
        KEY_APOSTROPHE,
        KEY_COMMA,
        KEY_MINUS,
        KEY_PERIOD,
        KEY_SLASH,
        KEY_SEMICOLON,
        KEY_EQUALS,
        KEY_LEFT_BRACKET,
        KEY_BACK_SLASH,
        KEY_RIGHT_BRACKET,
        KEY_GRAVE,
        KEY_CAPS_LOCK,
        KEY_SCROLL_LOCK,
        KEY_NUM_LOCK_CLEAR,
        KEY_PRINT_SCREEN,
        KEY_PAUSE,
        KEY_LEFT_CTRL,
        KEY_LEFT_SHIFT,
        KEY_LEFT_ALT,
        KEY_LEFT_GUI,
        KEY_RIGHT_CTRL,
        KEY_RIGHT_SHIFT,
        KEY_RIGHT_ALT,
        KEY_RIGHT_GUI,
        KEY_APPLICATION,

        KEY_0,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        
        KEY_A,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_E,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_I,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_M,
        KEY_N,
        KEY_O,
        KEY_P,
        KEY_Q,
        KEY_R,
        KEY_S,
        KEY_T,
        KEY_U,
        KEY_V,
        KEY_W,
        KEY_X,
        KEY_Y,
        KEY_Z,

        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,
        KEY_F13,
        KEY_F14,
        KEY_F15,
        KEY_F16,
        KEY_F17,
        KEY_F18,
        KEY_F19,
        KEY_F20,
        KEY_F21,
        KEY_F22,
        KEY_F23,
        KEY_F24,

        KEY_BACK,
        KEY_FORWARD,
    };

    struct ENGINE_API Input_System
    {
        Input_System();

        b8 IsDown[512];
    };

    ENGINE_API Key KeyFromSDLKey(SDL_Keycode sdl_key);
}
