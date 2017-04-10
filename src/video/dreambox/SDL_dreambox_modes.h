/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*  SDL dreambox backend
*  Copyright (C) 2017 Emanuel Strobel
*/

#ifndef __SDL_DREAMBOX_MODES_H__
#define __SDL_DREAMBOX_MODES_H__

#include "../../SDL_internal.h"


/* Display helper functions */
extern void dreambox_show_window(SDL_bool show);
extern int dreambox_get_videomode(char * dest);
extern int dreambox_set_displaymode(SDL_DisplayMode * mode);

void dreambox_free_display_mode_data(SDL_DisplayMode * mode);
int dreambox_set_videomode(const char *vmode);
int dreambox_set_framebuffer_resolution(int width, int height);
int dreambox_get_displaymode_from_videomode(const char *vmode, SDL_DisplayMode *mode);

/****************************************************************************/
/* SDL_VideoDevice functions declaration                                    */
/****************************************************************************/

/* Display functions */
extern void DREAM_GetDisplayModes(_THIS, SDL_VideoDisplay * display);
extern int DREAM_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
extern int DREAM_InitModes(_THIS);
extern void DREAM_QuitModes(_THIS);


#endif

/* vi: set ts=4 sw=4 expandtab: */
