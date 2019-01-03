#ifndef RV_AV_API_H
#define RV_AV_API_H

#include "../rv_av.h"

int av_init(int width, int height);
int av_set_framebuffer(uint8_t *pixels);
void av_delay(uint32_t ms);
int av_update();
int av_poll_event(struct av_event *evt);
int av_set_palette(struct av_color *palette, int ncolors);
uint32_t av_get_ticks();
uint32_t av_get_mouse_state(int *x, int *y);
int av_warp_mouse(int x, int y);
void av_shutdown();

#endif
