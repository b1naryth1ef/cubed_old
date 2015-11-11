#pragma once

#include "global.h"
#include <SDL2/SDL.h>
#include <GL/gl.h>

class Window {
    public:
        SDL_Window *win = nullptr;
        SDL_Renderer *ren = nullptr;
        SDL_GLContext ctx;

        Window(std::string, int, int);

        ~Window() {
            if (this->ren != nullptr) {
                SDL_DestroyRenderer(this->ren);
            }

            SDL_GL_DeleteContext(this->ctx);

            if (this->win != nullptr) {
                SDL_DestroyWindow(this->win);
            }
        }
};
