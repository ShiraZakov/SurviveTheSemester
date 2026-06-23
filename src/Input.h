#pragma once
/// @file Input.h
/// @brief Shared input helpers at the SDL boundary — global mouse to world-meter conversion.

#include "Config.h"
#include <SDL3/SDL.h>

namespace input {
    /// @brief Global mouse X mapped into world meters, relative to the render window.
    /// @param r SDL renderer (used to resolve the window position)
    /// @return Mouse X in world units (meters)
    inline float mouseWorldX(SDL_Renderer* r) {
        float globalX = 0.0f, globalY = 0.0f;
        SDL_GetGlobalMouseState(&globalX, &globalY);
        (void)globalY;
        int windowX = 0, windowY = 0;
        if (SDL_Window* window = SDL_GetRenderWindow(r))
            SDL_GetWindowPosition(window, &windowX, &windowY);
        return (globalX - static_cast<float>(windowX)) / Config::PPM;
    }
}
