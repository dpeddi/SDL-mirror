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

#ifndef __SDL_DREAMBOX_H__
#define __SDL_DREAMBOX_H__

#include <GLES/egl.h>

#include "../../SDL_internal.h"
#include "../SDL_sysvideo.h"

typedef struct SDL_VideoData
{
    SDL_bool egl_initialized;   /* OpenGL ES device initialization status */
    EGLDisplay egl_display;     /* OpenGL ES display connection           */
    uint32_t egl_refcount;      /* OpenGL ES reference count              */
    uint32_t swapinterval;      /* OpenGL ES default swap interval        */

} SDL_VideoData;


typedef struct SDL_DisplayData
{

} SDL_DisplayData;


typedef struct SDL_WindowData
{
    SDL_bool uses_gles;         /* if true window must support OpenGL ES */

    EGLConfig gles_configs[32];
    EGLint gles_config;         /* OpenGL ES configuration index      */
    EGLContext gles_context;    /* OpenGL ES context                  */
    EGLint gles_attributes[256];        /* OpenGL ES attributes for context   */
    EGLSurface gles_surface;    /* OpenGL ES target rendering surface */

} SDL_WindowData;


/****************************************************************************/
/* SDL_VideoDevice functions declaration                                    */
/****************************************************************************/

/* Display helper functions */
void DREAM_setfbresolution(int width, int height);
void DREAM_setvideomode(const char *mode);
void DREAM_waitforsync();

/* Display and window functions */
int DREAM_videoinit(_THIS);
void DREAM_videoquit(_THIS);
void DREAM_getdisplaymodes(_THIS, SDL_VideoDisplay * display);
int DREAM_setdisplaymode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
int DREAM_createwindow(_THIS, SDL_Window * window);
int DREAM_createwindowfrom(_THIS, SDL_Window * window, const void *data);
void DREAM_setwindowtitle(_THIS, SDL_Window * window);
void DREAM_setwindowicon(_THIS, SDL_Window * window, SDL_Surface * icon);
void DREAM_setwindowposition(_THIS, SDL_Window * window);
void DREAM_setwindowsize(_THIS, SDL_Window * window);
void DREAM_showwindow(_THIS, SDL_Window * window);
void DREAM_hidewindow(_THIS, SDL_Window * window);
void DREAM_raisewindow(_THIS, SDL_Window * window);
void DREAM_maximizewindow(_THIS, SDL_Window * window);
void DREAM_minimizewindow(_THIS, SDL_Window * window);
void DREAM_restorewindow(_THIS, SDL_Window * window);
void DREAM_setwindowgrab(_THIS, SDL_Window * window, SDL_bool grabbed);
void DREAM_destroywindow(_THIS, SDL_Window * window);

/* Window manager function */
SDL_bool DREAM_getwindowwminfo(_THIS, SDL_Window * window,
                             struct SDL_SysWMinfo *info);

/* OpenGL/OpenGL ES functions */
int DREAM_gl_loadlibrary(_THIS, const char *path);
void *DREAM_gl_getprocaddres(_THIS, const char *proc);
void DREAM_gl_unloadlibrary(_THIS);
SDL_GLContext DREAM_gl_createcontext(_THIS, SDL_Window * window);
int DREAM_gl_makecurrent(_THIS, SDL_Window * window, SDL_GLContext context);
int DREAM_gl_setswapinterval(_THIS, int interval);
int DREAM_gl_getswapinterval(_THIS);
int DREAM_gl_swapwindow(_THIS, SDL_Window * window);
void DREAM_gl_deletecontext(_THIS, SDL_GLContext context);


#endif /* __SDL_DREAMBOX_H__ */

/* vi: set ts=4 sw=4 expandtab: */
