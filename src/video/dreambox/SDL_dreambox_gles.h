/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no GLES will the authors be held liable for any damages
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

#include "../../SDL_internal.h"

#ifndef __SDL_DREAMBOX_GLES_H__
#define __SDL_DREAMBOX_GLES_H__

#if SDL_VIDEO_DRIVER_DREAMBOX && SDL_VIDEO_OPENGL_EGL

#include "../SDL_sysvideo.h"
#include "../SDL_egl_c.h"

/* OpenGL/OpenGL ES functions */
//#define DREAM_EGL_GetAttribute SDL_EGL_GetAttribute
//#define DREAM_EGL_GetProcAddress SDL_EGL_GetProcAddress
#define DREAM_EGL_UnloadLibrary SDL_EGL_UnloadLibrary
//#define DREAM_EGL_SetSwapInterval SDL_EGL_SetSwapInterval
//#define DREAM_EGL_GetSwapInterval SDL_EGL_GetSwapInterval

extern int DREAM_EGL_LoadLibrary(_THIS, const char *path);

extern void *DREAM_EGL_GetProcAddress(_THIS, const char *proc);
extern int DREAM_EGL_LoadLibrary(_THIS, const char *path);
extern SDL_GLContext DREAM_EGL_CreateContext(_THIS, SDL_Window * window);
extern int DREAM_EGL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context);
extern int DREAM_EGL_SwapWindow(_THIS, SDL_Window * window);
extern void DREAM_EGL_DeleteContext(_THIS, SDL_GLContext context);
extern int DREAM_EGL_SetSwapInterval(_THIS, int interval);
extern int DREAM_EGL_GetSwapInterval(_THIS);
#endif /* SDL_VIDEO_DRIVER_DREAMBOX && SDL_VIDEO_OPENGL_EGL */

#endif /* __SDL_DREAMBOX_GLES_H__ */

/* vi: set ts=4 sw=4 expandtab: */
