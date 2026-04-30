// Copyright Seong Woo Lee. All Rights Reserved.

#include "Input.h"

namespace Engine
{
    ENGINE_API Key KeyFromSDL(SDL_Keycode sdl_key)
    {
        switch (sdl_key) 
        {
            INVALID_DEFAULT_CASE;

            case SDLK_TAB:              return KEY_TAB;
            case SDLK_LEFT:             return KEY_LEFT;
            case SDLK_RIGHT:            return KEY_RIGHT;
            case SDLK_UP:               return KEY_UP;
            case SDLK_DOWN:             return KEY_DOWN;
            case SDLK_PAGEUP:           return KEY_PAGE_UP;
            case SDLK_PAGEDOWN:         return KEY_PAGE_DOWN;
            case SDLK_HOME:             return KEY_HOME;
            case SDLK_END:              return KEY_END;
            case SDLK_INSERT:           return KEY_INSERT;
            case SDLK_DELETE:           return KEY_DELETE;
            case SDLK_BACKSPACE:        return KEY_BACKSPACE;
            case SDLK_SPACE:            return KEY_SPACE;
            case SDLK_RETURN:           return KEY_RETURN;
            case SDLK_ESCAPE:           return KEY_ESC;
            case SDLK_APOSTROPHE:       return KEY_APOSTROPHE;
            case SDLK_COMMA:            return KEY_COMMA;
            case SDLK_MINUS:            return KEY_MINUS;
            case SDLK_PERIOD:           return KEY_PERIOD;
            case SDLK_SLASH:            return KEY_SLASH;
            case SDLK_SEMICOLON:        return KEY_SEMICOLON;
            case SDLK_EQUALS:           return KEY_EQUALS;
            case SDLK_LEFTBRACKET:      return KEY_LEFT_BRACKET;
            case SDLK_BACKSLASH:        return KEY_BACK_SLASH;
            case SDLK_RIGHTBRACKET:     return KEY_RIGHT_BRACKET;
            case SDLK_GRAVE:            return KEY_GRAVE;
            case SDLK_CAPSLOCK:         return KEY_CAPS_LOCK;
            case SDLK_SCROLLLOCK:       return KEY_SCROLL_LOCK;
            case SDLK_NUMLOCKCLEAR:     return KEY_NUM_LOCK_CLEAR;
            case SDLK_PRINTSCREEN:      return KEY_PRINT_SCREEN;
            case SDLK_PAUSE:            return KEY_PAUSE;
            case SDLK_LCTRL:            return KEY_LEFT_CTRL;
            case SDLK_LSHIFT:           return KEY_LEFT_SHIFT;
            case SDLK_LALT:             return KEY_LEFT_ALT;
            case SDLK_LGUI:             return KEY_LEFT_GUI;
            case SDLK_RCTRL:            return KEY_RIGHT_CTRL;
            case SDLK_RSHIFT:           return KEY_RIGHT_SHIFT;
            case SDLK_RALT:             return KEY_RIGHT_ALT;
            case SDLK_RGUI:             return KEY_RIGHT_GUI;
            case SDLK_APPLICATION:      return KEY_APPLICATION;

            case SDLK_0: return KEY_0;
            case SDLK_1: return KEY_1;
            case SDLK_2: return KEY_2;
            case SDLK_3: return KEY_3;
            case SDLK_4: return KEY_4;
            case SDLK_5: return KEY_5;
            case SDLK_6: return KEY_6;
            case SDLK_7: return KEY_7;
            case SDLK_8: return KEY_8;
            case SDLK_9: return KEY_9;
            case SDLK_A: return KEY_A;
            case SDLK_B: return KEY_B;
            case SDLK_C: return KEY_C;
            case SDLK_D: return KEY_D;
            case SDLK_E: return KEY_E;
            case SDLK_F: return KEY_F;
            case SDLK_G: return KEY_G;
            case SDLK_H: return KEY_H;
            case SDLK_I: return KEY_I;
            case SDLK_J: return KEY_J;
            case SDLK_K: return KEY_K;
            case SDLK_L: return KEY_L;
            case SDLK_M: return KEY_M;
            case SDLK_N: return KEY_N;
            case SDLK_O: return KEY_O;
            case SDLK_P: return KEY_P;
            case SDLK_Q: return KEY_Q;
            case SDLK_R: return KEY_R;
            case SDLK_S: return KEY_S;
            case SDLK_T: return KEY_T;
            case SDLK_U: return KEY_U;
            case SDLK_V: return KEY_V;
            case SDLK_W: return KEY_W;
            case SDLK_X: return KEY_X;
            case SDLK_Y: return KEY_Y;
            case SDLK_Z: return KEY_Z;

            case SDLK_F1: return KEY_F1;
            case SDLK_F2: return KEY_F2;
            case SDLK_F3: return KEY_F3;
            case SDLK_F4: return KEY_F4;
            case SDLK_F5: return KEY_F5;
            case SDLK_F6: return KEY_F6;
            case SDLK_F7: return KEY_F7;
            case SDLK_F8: return KEY_F8;
            case SDLK_F9: return KEY_F9;
            case SDLK_F10: return KEY_F10;
            case SDLK_F11: return KEY_F11;
            case SDLK_F12: return KEY_F12;
            case SDLK_F13: return KEY_F13;
            case SDLK_F14: return KEY_F14;
            case SDLK_F15: return KEY_F15;
            case SDLK_F16: return KEY_F16;
            case SDLK_F17: return KEY_F17;
            case SDLK_F18: return KEY_F18;
            case SDLK_F19: return KEY_F19;
            case SDLK_F20: return KEY_F20;
            case SDLK_F21: return KEY_F21;
            case SDLK_F22: return KEY_F22;
            case SDLK_F23: return KEY_F23;
            case SDLK_F24: return KEY_F24;

            case SDLK_AC_BACK: return KEY_BACK;
            case SDLK_AC_FORWARD: return KEY_FORWARD;
        }

        return KEY_NULL;
    }

    ENGINE_API Mouse_Button MouseButtonFromSDL(SDL_MouseButtonFlags flags)
    {
        switch (flags)
        {
            INVALID_DEFAULT_CASE;

            case SDL_BUTTON_LEFT:   return MOUSE_LEFT;
            case SDL_BUTTON_MIDDLE: return MOUSE_MIDDLE;
            case SDL_BUTTON_RIGHT:  return MOUSE_RIGHT;
        }
        return MOUSE_NULL;
    }

    ENGINE_API Input_System::Input_System()
    {
        memset(key_is_down, 0, sizeof(key_is_down[0]) * ARRAY_COUNT(key_is_down));

        memset(mouse_was_down, 0, sizeof(mouse_was_down[0]) * ARRAY_COUNT(mouse_was_down));
        memset(mouse_is_down, 0, sizeof(mouse_is_down[0]) * ARRAY_COUNT(mouse_is_down));
    }
}
