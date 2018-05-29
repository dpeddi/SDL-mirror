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

*  SDL vuplus backend
*  Copyright (C) 2017 Emanuel Strobel
*/

#include "../../SDL_internal.h"

#ifndef __SDL_VUPLUS_H__
#define __SDL_VUPLUS_H__

#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#ifdef SDL_VIDEO_OPENGL_EGL

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <KHR/khrplatform.h>
#include <eglvuplus.h>

#ifdef SDL_VIDEO_OPENGL_ES2

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "../SDL_sysvideo.h"

static VUGLES_PlatformHandle vugles_handle = 0;

typedef struct SDL_VideoData
{
    SDL_bool egl_initialized;   /* OpenGL ES device initialization status */
    EGLDisplay egl_display;     /* OpenGL ES display connection           */
    uint32_t egl_refcount;      /* OpenGL ES reference count              */
    uint32_t egl_swapinterval;  /* OpenGL ES default swap interval        */

} SDL_VideoData;


typedef struct SDL_DisplayModeData {
	char videomode[16];
} SDL_DisplayModeData;


typedef struct SDL_WindowData
{
    SDL_bool uses_gles;         /* if true window must support OpenGL ES */

    EGLConfig gles_configs[32];
    EGLint gles_config;         /* OpenGL ES configuration index      */
    EGLContext gles_context;    /* OpenGL ES context                  */
    EGLint gles_attributes[256]; /* OpenGL ES attributes for context   */
    EGLSurface gles_surface;    /* OpenGL ES target rendering surface */

} SDL_WindowData;


/* store videomode startvalue */
char VU_InitVideoMode[16];

/* Display helper functions */
void vuplus_wait_for_sync();


/****************************************************************************/
/* SDL_VideoDevice functions declaration                                    */
/****************************************************************************/

/* Display and window functions */
int VU_VideoInit(_THIS);
void VU_VideoQuit(_THIS);
int VU_CreateWindow(_THIS, SDL_Window * window);
int VU_CreateWindowFrom(_THIS, SDL_Window * window, const void *data);
void VU_SetWindowTitle(_THIS, SDL_Window * window);
void VU_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon);
void VU_SetWindowPosition(_THIS, SDL_Window * window);
void VU_SetWindowSize(_THIS, SDL_Window * window);
void VU_ShowWindow(_THIS, SDL_Window * window);
void VU_HideWindow(_THIS, SDL_Window * window);
void VU_RaiseWindow(_THIS, SDL_Window * window);
void VU_MaximizeWindow(_THIS, SDL_Window * window);
void VU_MinimizeWindow(_THIS, SDL_Window * window);
void VU_RestoreWindow(_THIS, SDL_Window * window);
void VU_SetWindowGrab(_THIS, SDL_Window * window, SDL_bool grabbed);
void VU_DestroyWindow(_THIS, SDL_Window * window);

/* Window manager function */
SDL_bool VU_GetWindowWMInfo(_THIS, SDL_Window * window,
                             struct SDL_SysWMinfo *info);

#endif /* SDL_VIDEO_OPENGL_ES2 */

#endif /* SDL_VIDEO_OPENGL_EGL */

#endif /* __SDL_VUPLUS_H__ */

/* vi: set ts=4 sw=4 expandtab: */
