// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for SDL library
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>
#include <stdint.h>

//#include "SDL.h"

#include "m_swap.h"
#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#include "rv_av_api.h"

// Fake mouse handling.
boolean		grabMouse;


//
//  Translates the key 
//

int xlatekey(struct av_key *key)
{
    int rc;

    switch(key->vk_code)
    {
      case SDLK_LEFT:	rc = KEY_LEFTARROW;	break;
      case SDLK_RIGHT:	rc = KEY_RIGHTARROW;	break;
      case SDLK_DOWN:	rc = KEY_DOWNARROW;	break;
      case SDLK_UP:	rc = KEY_UPARROW;	break;
      case SDLK_ESCAPE:	rc = KEY_ESCAPE;	break;
      case SDLK_RETURN:	rc = KEY_ENTER;		break;
      case SDLK_TAB:	rc = KEY_TAB;		break;
      case SDLK_F1:	rc = KEY_F1;		break;
      case SDLK_F2:	rc = KEY_F2;		break;
      case SDLK_F3:	rc = KEY_F3;		break;
      case SDLK_F4:	rc = KEY_F4;		break;
      case SDLK_F5:	rc = KEY_F5;		break;
      case SDLK_F6:	rc = KEY_F6;		break;
      case SDLK_F7:	rc = KEY_F7;		break;
      case SDLK_F8:	rc = KEY_F8;		break;
      case SDLK_F9:	rc = KEY_F9;		break;
      case SDLK_F10:	rc = KEY_F10;		break;
      case SDLK_F11:	rc = KEY_F11;		break;
      case SDLK_F12:	rc = KEY_F12;		break;
	
      case SDLK_BACKSPACE:
      case SDLK_DELETE:	rc = KEY_BACKSPACE;	break;

      case SDLK_PAUSE:	rc = KEY_PAUSE;		break;

      case SDLK_EQUALS:	rc = KEY_EQUALS;	break;

      case SDLK_KP_MINUS:
      case SDLK_MINUS:	rc = KEY_MINUS;		break;

      case SDLK_LSHIFT:
      case SDLK_RSHIFT:
	rc = KEY_RSHIFT;
	break;
	
      case SDLK_LCTRL:
      case SDLK_RCTRL:
	rc = KEY_RCTRL;
	break;
	
      case SDLK_LALT:
//      case SDLK_LMETA:
      case SDLK_RALT:
      //case SDLK_RMETA:
	rc = KEY_RALT;
	break;
	
      default:
        rc = key->vk_code;
	break;
    }

    return rc;
}

void I_ShutdownGraphics(void)
{
    av_shutdown();
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

/* This processes SDL events */
void I_GetEvent(struct av_event *evt)
{
    uint8_t buttonstate;
    struct av_event_keyboard *keyevt;
    event_t event;
    memset(&event, 0, sizeof(event_t));

    switch (evt->event_type) {
    case AV_event_keydown:
        keyevt = (struct av_event_keyboard *)evt;
	    event.type = ev_keydown;
	    event.data1 = xlatekey(&keyevt->key);
	    D_PostEvent(&event);
        break;

    case AV_event_keyup:
        keyevt = (struct av_event_keyboard *)evt;
	    event.type = ev_keyup;
	    event.data1 = xlatekey(&keyevt->key);
	    D_PostEvent(&event);
	    break;

    case AV_event_quit:
        I_Quit();
        break;
/*
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
	buttonstate = SDL_GetMouseState(NULL, NULL);
	event.type = ev_mouse;
	event.data1 = 0
	    | (buttonstate & SDL_BUTTON(1) ? 1 : 0)
	    | (buttonstate & SDL_BUTTON(2) ? 2 : 0)
	    | (buttonstate & SDL_BUTTON(3) ? 4 : 0);
	event.data2 = event.data3 = 0;
	D_PostEvent(&event);
	break;

#if (SDL_MAJOR_VERSION >= 0) && (SDL_MINOR_VERSION >= 9)
      case SDL_MOUSEMOTION:*/
	/* Ignore mouse warp events */
// 	if ((Event->motion.x != screen->w/2)||(Event->motion.y != screen->h/2))
// 	{
// 	    /* Warp the mouse back to the center */
// 	    if (grabMouse) {
// 		SDL_WarpMouse(screen->w/2, screen->h/2);
// 	    }
// 	    event.type = ev_mouse;
// 	    event.data1 = 0
// 	        | (Event->motion.state & SDL_BUTTON(1) ? 1 : 0)
// 	        | (Event->motion.state & SDL_BUTTON(2) ? 2 : 0)
// 	        | (Event->motion.state & SDL_BUTTON(3) ? 4 : 0);
// 	    event.data2 = Event->motion.xrel << 2;
// 	    event.data3 = -Event->motion.yrel << 2;
// 	    D_PostEvent(&event);
// 	}
// 	break;
// #endif
     }

}

//
// I_StartTic
//
void I_StartTic (void)
{
    char buf[AV_EVENT_BUF_SIZE] = {0};
    struct av_event *evt = (struct av_event *)&buf[0];
    while (av_poll_event(evt))
        I_GetEvent(evt);
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{

    static int	lasttic;
    int		tics;
    int		i;

    // draws little dots on the bottom of the screen
    if (devparm) {
    	i = I_GetTime();
	    tics = i - lasttic;
    	lasttic = i;
	    if (tics > 20)
            tics = 20;

	    for (i=0 ; i<tics*2 ; i+=2)
	        screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
	    for ( ; i<20*2 ; i+=2)
	        screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    }

    av_update();
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}


//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
    int i;
    struct av_color colors[256];

    for ( i=0; i<256; ++i ) {
	    colors[i].r = gammatable[usegamma][*palette++];
	    colors[i].g = gammatable[usegamma][*palette++];
	    colors[i].b = gammatable[usegamma][*palette++];
	    colors[i].a = 255;
    }

    av_set_palette(colors, 256);
}


void I_InitGraphics(void)
{

    static int	firsttime=1;

    if (!firsttime)
	    return;
    firsttime = 0;

    // check if the user wants to grab the mouse (quite unnice)
    grabMouse = !!M_CheckParm("-grabmouse");

    screens[0] = (unsigned char *)malloc(SCREENWIDTH*SCREENHEIGHT);
    if (screens[0] == NULL)
        I_Error("Couldn't allocate screen memory");
    av_set_framebuffer(screens[0]);
}
