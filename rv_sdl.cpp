#include "rv_sdl.h"

void rv_sdl::log_sdl_error(const char *syscall_name, const char *sdl_func)
{
    fprintf(stderr, "[e] error: syscall_%s - SDL_%s() failed with: %s\n", syscall_name, sdl_func, SDL_GetError());
}

rv_uint rv_sdl::syscall_init(rv_uint arg0, rv_uint arg1)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        log_sdl_error("init", "Init");
        return (rv_uint)-1;
    }

    width_ = (int)arg0;
    height_ = (int)arg1;

    if (SDL_CreateWindowAndRenderer(width_, height_, 0, &main_window_, &main_renderer_) < 0) {
        log_sdl_error("init", "CreateWindowAndRenderer");
        return (rv_uint)-1;
    }

    SDL_SetWindowTitle(main_window_, "RISC-666");

    main_surface_ = SDL_CreateRGBSurface(0, width_, height_, 32, 0, 0, 0, 0);
    if (main_surface_ == nullptr) {
        log_sdl_error("init", "CreateRGBSurface");
        return (rv_uint)-1;
    }

    main_texture_ = SDL_CreateTexture(main_renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width_, height_);
    if (main_texture_ == nullptr) {
        log_sdl_error("init", "CreateTexture");
        return (rv_uint)-1;
    }
    return 0;
}

rv_uint rv_sdl::syscall_set_palette(rv_uint arg0, rv_uint arg1)
{
    if (arg0 == 0 || arg1 == 0)
        return (rv_uint)-EINVAL;

    SDL_Color *colors = reinterpret_cast<SDL_Color*>(memory_.ram_ptr(arg0));
    int cnt = (int)arg1;

    if (SDL_SetPaletteColors(screen_surface_->format->palette, colors, 0, cnt) < 0) {
        log_sdl_error("set_palette", "SetPaletteColors");
        return (rv_uint)-1;
    }
    return 0;
}

rv_uint rv_sdl::syscall_set_framebuffer(rv_uint arg0)
{
    if (arg0 == 0)
        return (rv_uint)-EINVAL;

    void *pixels = reinterpret_cast<void*>(memory_.ram_ptr(arg0));
    screen_surface_ = SDL_CreateRGBSurfaceFrom(pixels, width_, height_, 8, width_, 0, 0, 0, 0);
    if (screen_surface_ == nullptr) {
        log_sdl_error("set_framebuffer", "CreateRGBSurfaceFrom");
        return (rv_uint)-1;
    }
    return 0;
}

rv_uint rv_sdl::syscall_update()
{
    if (SDL_BlitSurface(screen_surface_, nullptr, main_surface_, nullptr) < 0) {
        log_sdl_error("update", "BlitSurface");
        return (rv_uint)-1;
    }
    if (SDL_UpdateTexture(main_texture_, nullptr, main_surface_->pixels, main_surface_->pitch) < 0) {
        log_sdl_error("update", "UpdateTexture");
        return (rv_uint)-1;
    }
    if (SDL_RenderClear(main_renderer_) < 0) {
        log_sdl_error("update", "RenderClear");
        return (rv_uint)-1;
    }
    if (SDL_RenderCopy(main_renderer_, main_texture_, nullptr, nullptr) < 0) {
        log_sdl_error("update", "RenderCopy");
        return (rv_uint)-1;
    }
    SDL_RenderPresent(main_renderer_);

    return 0;
}

rv_uint rv_sdl::syscall_poll_event(rv_uint arg0)
{
    if (arg0 == 0)
        return (rv_uint)-1;

    SDL_Event event;
    if(SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            av_event_keyboard *keyevt = reinterpret_cast<av_event_keyboard *>(memory_.ram_ptr(arg0));
            if (event.type == SDL_KEYDOWN)
                keyevt->hdr.event_type = AV_event_keydown;
            else if (event.type == SDL_KEYUP)
                keyevt->hdr.event_type = AV_event_keyup;
            keyevt->key.scan_code = event.key.keysym.scancode;
            keyevt->key.vk_code = event.key.keysym.sym;
            keyevt->hdr.timestamp = SDL_GetTicks();
        }
            break;

        case SDL_QUIT: {
            av_event *evt = reinterpret_cast<av_event *>(memory_.ram_ptr(arg0));
            evt->event_type = AV_event_quit;
            evt->timestamp = SDL_GetTicks();
        }
            break;
        }
        return 1;
    }

    return 0;
}

rv_uint rv_sdl::syscall_delay(rv_uint arg0)
{
    SDL_Delay(arg0);
    return 0;
}

rv_uint rv_sdl::syscall_get_ticks()
{
    return (rv_uint)SDL_GetTicks();
}

rv_uint rv_sdl::syscall_shutdown()
{
    if (main_surface_ != nullptr)
        SDL_FreeSurface(main_surface_);
    if (screen_surface_ != nullptr)
        SDL_FreeSurface(screen_surface_);
    if (main_texture_ != nullptr)
        SDL_DestroyTexture(main_texture_);

    if (main_renderer_ != nullptr)
        SDL_DestroyRenderer(main_renderer_);
    if (main_window_ != nullptr)
        SDL_DestroyWindow(main_window_);

    SDL_Quit();
    return 0;
}