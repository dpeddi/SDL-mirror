/*
	Simple DirectMedia Layer
	Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

	This software is provided 'as-is', without any express or implied
	warranty.	In no event will the authors be held liable for any damages
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

*	SDL dreambox backend
*	Copyright (C) 2017 Emanuel Strobel
*/

#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_DREAMBOX && SDL_VIDEO_OPENGL_EGL

/* SDL internals */
#include "../SDL_sysvideo.h"
#include "SDL_version.h"
#include "SDL_syswm.h"
#include "SDL_events.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_keyboard_c.h"

#ifdef SDL_INPUT_LINUXEV
#include "../../core/linux/SDL_evdev.h"
#endif

/* DREAM declarations */
#include "SDL_dreambox.h"
#include "SDL_dreambox_gles.h"
#include "SDL_dreambox_modes.h"
#include "SDL_dreambox_events.h"

#ifndef DREAMBOX_DEBUG
#define DREAMBOX_DEBUG 1
#endif


/* dreambox helper functions */

void 
dreambox_wait_for_sync(void)
{
	int fd;
	unsigned int c = 0;
	
	fd = open("/dev/fb0", O_RDWR, 0);
	
	if (fd<0) {
#if DREAMBOX_DEBUG
		fprintf(stderr, "ERROR: DREAM: open framebuffer failed\n");
#endif
		return;
	}
	
	ioctl(fd, FBIO_WAITFORVSYNC, &c);
	close(fd);
}

/****************************************************************************/
/* SDL_VideoDevice functions declaration                                    */
/****************************************************************************/

static int
DREAM_Available(void)
{
	char name[32]={0x0};
	FILE *fd = fopen("/proc/stb/info/model","r");
	
	if (fd<0) return 0;
	
	fgets(name,32,fd);
	fclose(fd);

	if ( (SDL_strncmp(name, "dm820", 5) == 0) || 
		(SDL_strncmp(name, "dm900", 5) == 0) || 
		(SDL_strncmp(name, "dm7080", 6) == 0) ) {
		
#if DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: Available: %s", name);
#endif
		return 1;
	}
	return 0;
}

static void
DREAM_Destroy(SDL_VideoDevice * device)
{
#if DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: Destroy\n");
#endif
	if (device->driverdata != NULL) {
		SDL_free(device->driverdata);
		device->driverdata = NULL;
	}
	SDL_free(device);
}

static SDL_VideoDevice *
DREAM_Create()
{
	SDL_VideoDevice *device;
	SDL_VideoData *phdata;

	/* Initialize SDL_VideoDevice structure */
	device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
	if (device == NULL) {
		SDL_OutOfMemory();
		return NULL;
	}

	/* Initialize internal Dreambox specific data */
	phdata = (SDL_VideoData *) SDL_calloc(1, sizeof(SDL_VideoData));
	if (phdata == NULL) {
		SDL_OutOfMemory();
		SDL_free(device);
		return NULL;
	}

	device->driverdata = phdata;

	phdata->egl_initialized = SDL_TRUE;
	
	/* FIXME:has no effect */
	phdata->egl_swapinterval = 0;
	
	phdata->egl_refcount = 0;

	/* Setup amount of available displays */
	device->num_displays = 0;

	/* Set device free function */
	device->free = DREAM_Destroy;

	/* Setup all functions which we can handle */
	device->VideoInit = DREAM_VideoInit;
	device->VideoQuit = DREAM_VideoQuit;
	device->GetDisplayModes = DREAM_GetDisplayModes;
	device->SetDisplayMode = DREAM_SetDisplayMode;
	device->CreateWindow = DREAM_CreateWindow;
	device->CreateWindowFrom = DREAM_CreateWindowFrom;
	device->SetWindowTitle = DREAM_SetWindowTitle;
	device->SetWindowIcon = DREAM_SetWindowIcon;
	device->SetWindowPosition = DREAM_SetWindowPosition;
	device->SetWindowSize = DREAM_SetWindowSize;
	device->ShowWindow = DREAM_ShowWindow;
	device->HideWindow = DREAM_HideWindow;
	device->RaiseWindow = DREAM_RaiseWindow;
	device->MaximizeWindow = DREAM_MaximizeWindow;
	device->MinimizeWindow = DREAM_MinimizeWindow;
	device->RestoreWindow = DREAM_RestoreWindow;
	device->SetWindowGrab = DREAM_SetWindowGrab;
	device->DestroyWindow = DREAM_DestroyWindow;
	device->GetWindowWMInfo = DREAM_GetWindowWMInfo;
	device->GL_LoadLibrary = DREAM_EGL_LoadLibrary;
	device->GL_GetProcAddress = DREAM_EGL_GetProcAddress;
	device->GL_UnloadLibrary = DREAM_EGL_UnloadLibrary;
	device->GL_CreateContext = DREAM_EGL_CreateContext;
	device->GL_MakeCurrent = DREAM_EGL_MakeCurrent;
	device->GL_SetSwapInterval = DREAM_EGL_SetSwapInterval;
	device->GL_GetSwapInterval = DREAM_EGL_GetSwapInterval;
	device->GL_SwapWindow = DREAM_EGL_SwapWindow;
	device->GL_DeleteContext = DREAM_EGL_DeleteContext;
	device->PumpEvents = DREAM_PumpEvents;

	/* !!! FIXME: implement SetWindowBordered */

	return device;
}

VideoBootStrap DREAM_bootstrap = {

	"dreambox",
	"Dreambox EGL Video Driver",
	
	DREAM_Available,
	DREAM_Create
};

/*****************************************************************************/
/* SDL Video and Display initialization/handling functions					 */
/*****************************************************************************/
int
DREAM_VideoInit(_THIS)
{
	SDL_VideoDisplay display;
	SDL_DisplayMode current_mode;
	SDL_DisplayModeData *modedata;
	const char *videomode = "720p";

#if DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: VideoInit\n");
#endif
	SDL_zero(current_mode);
	modedata = (SDL_DisplayModeData *) SDL_calloc(1, sizeof(SDL_DisplayModeData));
	if (!modedata)
		return -1;
	
	current_mode.w = 1280;
	current_mode.h = 720;
	current_mode.refresh_rate = 60;
	current_mode.format = SDL_PIXELFORMAT_RGBA8888;
	current_mode.driverdata = modedata;
	SDL_strlcpy(modedata->videomode, videomode, SDL_strlen(videomode)+1);

	SDL_zero(display);
	display.desktop_mode = current_mode;
	display.current_mode = current_mode;
	
	dreambox_get_videomode(DREAM_InitVideoMode);
	dreambox_set_displaymode(&current_mode);

	SDL_AddVideoDisplay(&display);
	
	if (DREAM_InitModes(_this) < 0) {
		return -1;
	}
	
#ifdef SDL_INPUT_LINUXEV    
    if (SDL_EVDEV_Init() < 0) {
        return -1;
    }
#endif    
	return 1;
}

void
DREAM_VideoQuit(_THIS)
{
#ifdef SDL_INPUT_LINUXEV    
	SDL_EVDEV_Quit();
#endif 
	DREAM_QuitModes(_this);
	
	dreambox_show_window(SDL_TRUE);
#if DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: VideoQuit: exit Video Driver...\n");
#endif
}

int
DREAM_CreateWindow(_THIS, SDL_Window * window)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	SDL_WindowData *wdata;
	EGLNativeDisplayType *nativeDisplay=NULL;
	
#if DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: CreateWindow\n");
#endif
	
	/* Allocate window internal data */
	wdata = (SDL_WindowData *) SDL_calloc(1, sizeof(SDL_WindowData));
	if (wdata == NULL) {
		return SDL_OutOfMemory();
	}

	/* Setup driver data for this window */
	window->driverdata = wdata;

	/* Check if window must support OpenGL ES rendering */
	if ((window->flags & SDL_WINDOW_OPENGL) == SDL_WINDOW_OPENGL) {
		EGLBoolean initstatus;
#if DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: CreateWindow: app must support OpenGL ES rendering\n");
#endif
		/* Mark this window as OpenGL ES compatible */
		wdata->uses_gles = SDL_TRUE;

		/* Create connection to OpenGL ES */
		if (phdata->egl_display == EGL_NO_DISPLAY) {
			
			nativeDisplay = (EGLNativeDisplayType)0L;
			phdata->egl_display = eglGetDisplay(nativeDisplay);
			
			if (phdata->egl_display == EGL_NO_DISPLAY) {
				return SDL_SetError("DREAM: Can't get connection to OpenGL ES");
			}

			initstatus = eglInitialize(phdata->egl_display, NULL, NULL);
			if (initstatus != EGL_TRUE) {
				return SDL_SetError("DREAM: Can't init OpenGL ES library");
			}
		}
		phdata->egl_refcount++;
	}
	
	SDL_SetMouseFocus(window);
	SDL_SetKeyboardFocus(window);
	
	/* Window has been successfully created */
	return 0;
}

int
DREAM_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
	return -1;
}

void
DREAM_SetWindowTitle(_THIS, SDL_Window * window)
{
}
void
DREAM_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon)
{
}
void
DREAM_SetWindowPosition(_THIS, SDL_Window * window)
{
}
void
DREAM_SetWindowSize(_THIS, SDL_Window * window)
{
}

void
DREAM_ShowWindow(_THIS, SDL_Window * window)
{
	dreambox_show_window(SDL_TRUE);
}

void
DREAM_HideWindow(_THIS, SDL_Window * window)
{
	dreambox_show_window(SDL_FALSE);
}

void
DREAM_RaiseWindow(_THIS, SDL_Window * window)
{
}
void
DREAM_MaximizeWindow(_THIS, SDL_Window * window)
{
}
void
DREAM_MinimizeWindow(_THIS, SDL_Window * window)
{
}
void
DREAM_RestoreWindow(_THIS, SDL_Window * window)
{
}
void
DREAM_SetWindowGrab(_THIS, SDL_Window * window, SDL_bool grabbed)
{
}

void
DREAM_DestroyWindow(_THIS, SDL_Window * window)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	eglTerminate(phdata->egl_display);
}

/*****************************************************************************/
/* SDL Window Manager function                                               */
/*****************************************************************************/

SDL_bool
DREAM_GetWindowWMInfo(_THIS, SDL_Window * window, struct SDL_SysWMinfo *info)
{
	if (info->version.major <= SDL_MAJOR_VERSION) {
		return SDL_TRUE;
	} else {
		SDL_SetError("application not compiled with SDL %d.%d",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
		return SDL_FALSE;
	}

	/* Failed to get window manager information */
	return SDL_FALSE;
}

#endif /* SDL_VIDEO_DRIVER_DREAMBOX && SDL_VIDEO_OPENGL_EGL */

/* vi: set ts=4 sw=4 expandtab: */
