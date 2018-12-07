#ifndef RV_GRAPHICS
#define RV_GRAPHICS

#include <stdint.h>

enum SYS_av_syscalls
{
    SYS_av_init = 2048,
    SYS_av_update,
    SYS_av_set_palette,
    SYS_av_delay,
    SYS_av_poll_event,
    SYS_av_get_ticks,
    SYS_av_shutdown
};

#endif