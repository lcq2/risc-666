#ifndef RV_AV_API_H
#define RV_AV_API_H

#include "../rv_av.h"

int av_init(int width, int height);
void av_delay(uint32_t ms);
int av_update(uint8_t *screen);
int av_poll_event();
int av_set_palette(uint32_t *palette, int ncolors);
uint32_t av_get_ticks();
void av_shutdown();

#endif
