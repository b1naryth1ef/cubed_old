#pragma once

#include "global.h"
#include <SDL2/SDL.h>

class Window {
    public:
        SDL_Window *win;
        SDL_Renderer *ren;

        Window(std::string, int, int);

        ~Window() {
            if (this->win != nullptr) {
                SDL_DestroyWindow(this->win);
            }

            if (this->ren != nullptr) {
                SDL_DestroyRenderer(this->ren);
            }
        }
};