#pragma once
#include <SDL.h>
#include <stdexcept>
#include <string>

class SDLApp
{
public:
    SDLApp(Uint32 flags)
    {
        if (SDL_Init(flags) != 0)
        {
            throw std::runtime_error("SDL cannot be initialized: " + std::string(SDL_GetError()));
        }
    }

    ~SDLApp() { SDL_Quit(); }
};

class SDLWindow
{
public:
    SDLWindow(const char* title, int x, int y, int w, int h, Uint32 flags)
    {
        window = SDL_CreateWindow(title, x, y, w, h, flags);
        if (!window)
        {
            throw std::runtime_error("SDL window cannot be created: " + std::string(SDL_GetError()));
        }
    }

    ~SDLWindow()
    {
        if (window != nullptr)
        {
            SDL_DestroyWindow(window);
        }
    }

    SDL_Window* getWindow() const { return window; }
private:
    SDL_Window* window = nullptr;
};
