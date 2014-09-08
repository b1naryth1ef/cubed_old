#pragma once

#include "renderer.h"



Window::Window(std::string title, int height, int width) {
    this->win = SDL_CreateWindow(title.c_str(),
        0, 0, height, width, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (this->win == nullptr) {
        ERROR("Failed to create new SDL window: %s!", SDL_GetError());
        throw Exception("Failed to create new SDL window!");
    }

    this->ren = SDL_CreateRenderer(this->win, -1, SDL_RENDERER_ACCELERATED);

    if (this->ren == nullptr) {
        ERROR("Failed to create new SDL renderer!");
        throw Exception("Failed to create new SDL renderer!");
    }

    this->ctx = SDL_GL_CreateContext(this->win);
}