#pragma once

#include "global.h"
#include <SDL2/SDL.h>
#include <GL3/gl3.h>

class Window {
    public:
        SDL_Window *win;
        SDL_GLContext ctx;
        SDL_Renderer *ren;

        Window(std::string, int, int);

        ~Window() {
            if (this->ren != nullptr) {
                SDL_DestroyRenderer(this->ren);
            }

            if (this->ctx != nullptr) {
                SDL_GL_DeleteContext(this->ctx);
            }

            if (this->win != nullptr) {
                SDL_DestroyWindow(this->win);
            }
        }
};