#include "rv_sdl.h"

rv_uint rv_sdl::syscall_init(rv_uint arg0, rv_uint arg1)
{
    int retval = SDL_Init(SDL_INIT_VIDEO);
    if (retval < 0) {
        fprintf(stderr, "[e] error: SDL_Init() failed with %s\n", SDL_GetError());
        return (rv_uint)retval;
    }

    width_ = (int)arg0;
    height_ = (int)arg1;

    retval = SDL_CreateWindowAndRenderer(width_, height_, 0, &main_window_, &main_renderer_);
    if (retval < 0)
        return (rv_uint)retval;

    main_surface_ = SDL_CreateRGBSurface(0, width_, height_, 32, 0, 0, 0, 0);
    screen_surface_ = SDL_CreateRGBSurface(0, width_, height_, 8, 0, 0, 0, 0);
    main_texture_ = SDL_CreateTexture(main_renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width_, height_);
    return 0;
}

rv_uint rv_sdl::syscall_set_palette(rv_uint arg0, rv_uint arg1)
{
    if (arg0 == 0 || arg1 == 0)
        return (rv_uint)-EINVAL;

    SDL_Color *colors = reinterpret_cast<SDL_Color*>(memory_.ram_ptr(arg0));
    int cnt = (int)arg1;

    if (SDL_SetPaletteColors(screen_surface_->format->palette, colors, 0, cnt)) {
        printf("SDL_SetPaletteColors() failed with: %s\n", SDL_GetError());
        return (rv_uint)-1;
    }
    return 0;
}


rv_uint rv_sdl::syscall_update(rv_uint arg0)
{
    if (arg0 == 0)
        return (rv_uint)-EINVAL;

    const uint8_t* bits = reinterpret_cast<const uint8_t*>(memory_.ram_ptr(arg0));
    if (SDL_MUSTLOCK(screen_surface_)) {
        SDL_LockSurface(screen_surface_);
    }
    memcpy(screen_surface_->pixels, bits, width_*height_);
    if (SDL_MUSTLOCK(screen_surface_)) {
        SDL_UnlockSurface(screen_surface_);
    }
    if (SDL_BlitSurface(screen_surface_, nullptr, main_surface_, nullptr) < 0) {
        fprintf(stderr, "[e] error: SDL_BlitSurface() failed with %s\n", SDL_GetError());
        return (rv_uint)-1;
    }
    if (SDL_UpdateTexture(main_texture_, nullptr, main_surface_->pixels, main_surface_->pitch) < 0) {
        fprintf(stderr, "[e] error: SDL_UpdateTexture() failed with %s\n", SDL_GetError());
        return (rv_uint)-1;
    }
    SDL_RenderClear(main_renderer_);
    SDL_RenderCopy(main_renderer_, main_texture_, nullptr, nullptr);
    SDL_RenderPresent(main_renderer_);

    return 0;
}

rv_uint rv_sdl::syscall_poll_event()
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {

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