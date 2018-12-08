#pragma once
#include <unordered_map>
#include <SDL2/SDL.h>

#include "rv_av.h"
#include "rv_global.h"
#include "rv_memory.h"

class rv_sdl
{
public:
    rv_sdl() = delete;
    explicit rv_sdl(rv_memory& memory) : memory_{memory} {}

    rv_uint syscall_init(rv_uint arg0, rv_uint arg1);
    rv_uint syscall_set_framebuffer(rv_uint arg0);
    rv_uint syscall_delay(rv_uint arg0);
    rv_uint syscall_update();
    rv_uint syscall_set_palette(rv_uint arg0, rv_uint arg1);
    rv_uint syscall_poll_event(rv_uint arg0);
    rv_uint syscall_get_ticks();
    rv_uint syscall_shutdown();

private:
    void log_sdl_error(const char* syscall_name, const char* sdl_func);

private:
    rv_memory& memory_;
    SDL_Window *main_window_ = nullptr;
    SDL_Renderer *main_renderer_ = nullptr;
    SDL_Surface *main_surface_ = nullptr;
    SDL_Surface *screen_surface_ = nullptr;
    SDL_Texture *main_texture_ = nullptr;
    int width_ = -1;
    int height_ = -1;
};