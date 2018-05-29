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

*	SDL vuplus backend
*	Copyright (C) 2017 Emanuel Strobel
*/

#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_VUPLUS && SDL_VIDEO_OPENGL_EGL

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

/* VU declarations */
#include "SDL_vuplus.h"
#include "SDL_vuplus_gles.h"
#include "SDL_vuplus_modes.h"
#include "SDL_vuplus_events.h"

#ifndef VUPLUS_DEBUG
#define VUPLUS_DEBUG 1
#endif

/* vuplus helper functions */

void 
vuplus_wait_for_sync(void)
{
	int fd;
	unsigned int c = 0;
	
	fd = open("/dev/fb0", O_RDWR, 0);
	
	if (fd<0) {
#if VUPLUS_DEBUG
		fprintf(stderr, "ERROR: VU: open framebuffer failed\n");
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
VU_Available(void)
{
	char name[32]={0x0};
	FILE *fd = fopen("/proc/stb/info/vumodel","r");
	
	if (fd<0) return 0;
	
	fgets(name,32,fd);
	fclose(fd);

	if ( (SDL_strncmp(name, "solo4k", 6) == 0) ) {
		
#if VUPLUS_DEBUG
	fprintf(stderr, "VU: Available: %s", name);
#endif
		return 1;
	}
	return 0;
}

static void
VU_Destroy(SDL_VideoDevice * device)
{
#if VUPLUS_DEBUG
	fprintf(stderr, "VU: Destroy\n");
#endif
	if (device->driverdata != NULL) {
		SDL_free(device->driverdata);
		device->driverdata = NULL;
	}
	SDL_free(device);
}

static SDL_VideoDevice *
VU_Create()
{
	SDL_VideoDevice *device;
	SDL_VideoData *phdata;

	/* Initialize SDL_VideoDevice structure */
	device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
	if (device == NULL) {
		SDL_OutOfMemory();
		return NULL;
	}

	/* Initialize internal Vuplus specific data */
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
	device->free = VU_Destroy;

	/* Setup all functions which we can handle */
	device->VideoInit = VU_VideoInit;
	device->VideoQuit = VU_VideoQuit;
	device->GetDisplayModes = VU_GetDisplayModes;
	device->SetDisplayMode = VU_SetDisplayMode;
	device->CreateWindow = VU_CreateWindow;
	device->CreateWindowFrom = VU_CreateWindowFrom;
	device->SetWindowTitle = VU_SetWindowTitle;
	device->SetWindowIcon = VU_SetWindowIcon;
	device->SetWindowPosition = VU_SetWindowPosition;
	device->SetWindowSize = VU_SetWindowSize;
	device->ShowWindow = VU_ShowWindow;
	device->HideWindow = VU_HideWindow;
	device->RaiseWindow = VU_RaiseWindow;
	device->MaximizeWindow = VU_MaximizeWindow;
	device->MinimizeWindow = VU_MinimizeWindow;
	device->RestoreWindow = VU_RestoreWindow;
	device->SetWindowGrab = VU_SetWindowGrab;
	device->DestroyWindow = VU_DestroyWindow;
	device->GetWindowWMInfo = VU_GetWindowWMInfo;
	device->GL_LoadLibrary = VU_EGL_LoadLibrary;
	device->GL_GetProcAddress = VU_EGL_GetProcAddress;
	device->GL_UnloadLibrary = VU_EGL_UnloadLibrary;
	device->GL_CreateContext = VU_EGL_CreateContext;
	device->GL_MakeCurrent = VU_EGL_MakeCurrent;
	device->GL_SetSwapInterval = VU_EGL_SetSwapInterval;
	device->GL_GetSwapInterval = VU_EGL_GetSwapInterval;
	device->GL_SwapWindow = VU_EGL_SwapWindow;
	device->GL_DeleteContext = VU_EGL_DeleteContext;
	device->PumpEvents = VU_PumpEvents;

	/* !!! FIXME: implement SetWindowBordered */

	return device;
}

VideoBootStrap VU_bootstrap = {

	"vuplus",
	"Vuplus EGL Video Driver",
	
	VU_Available,
	VU_Create
};

/*****************************************************************************/
/* SDL Video and Display initialization/handling functions					 */
/*****************************************************************************/
int
VU_VideoInit(_THIS)
{
	SDL_VideoDisplay display;
	SDL_DisplayMode current_mode;
	SDL_DisplayModeData *modedata;
	const char *videomode = "720p";
	float aspect = 1.0f;

#if VUPLUS_DEBUG
	fprintf(stderr, "VU: VideoInit\n");
#endif
	SDL_zero(current_mode);
	modedata = (SDL_DisplayModeData *) SDL_calloc(1, sizeof(SDL_DisplayModeData));
	if (!modedata)
		return -1;
	
	current_mode.w = 1280;
	current_mode.h = 720;
	current_mode.refresh_rate = 50;
	current_mode.format = SDL_PIXELFORMAT_RGBA8888;
	current_mode.driverdata = modedata;
	SDL_strlcpy(modedata->videomode, videomode, SDL_strlen(videomode)+1);

	SDL_zero(display);
	display.desktop_mode = current_mode;
	display.current_mode = current_mode;
	
	vuplus_get_videomode(VU_InitVideoMode);
	vuplus_set_displaymode(&current_mode);

	VUGLES_InitPlatformAndDefaultDisplay(NULL, &aspect,
			current_mode.w, current_mode.h);

	VUGLES_RegisterDisplayPlatform(&display);
	SDL_AddVideoDisplay(&display);
	
	if (VU_InitModes(_this) < 0) {
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
VU_VideoQuit(_THIS)
{
#ifdef SDL_INPUT_LINUXEV    
	SDL_EVDEV_Quit();
#endif 
	VU_QuitModes(_this);
	
	vuplus_show_window(SDL_TRUE);
#if VUPLUS_DEBUG
	fprintf(stderr, "VU: VideoQuit: exit Video Driver...\n");
#endif
}

int
VU_CreateWindow(_THIS, SDL_Window * window)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	SDL_WindowData *wdata;
	EGLNativeDisplayType *nativeDisplay=NULL;
	
#if VUPLUS_DEBUG
	fprintf(stderr, "VU: CreateWindow\n");
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
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: CreateWindow: app must support OpenGL ES rendering\n");
#endif
		/* Mark this window as OpenGL ES compatible */
		wdata->uses_gles = SDL_TRUE;

		/* Create connection to OpenGL ES */
		if (phdata->egl_display == EGL_NO_DISPLAY) {
			
			nativeDisplay = (EGLNativeDisplayType)0L;
			phdata->egl_display = eglGetDisplay(nativeDisplay);
			
			if (phdata->egl_display == EGL_NO_DISPLAY) {
				return SDL_SetError("VU: Can't get connection to OpenGL ES");
			}

			initstatus = eglInitialize(phdata->egl_display, NULL, NULL);
			if (initstatus != EGL_TRUE) {
				return SDL_SetError("VU: Can't init OpenGL ES library");
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
VU_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
	return -1;
}

void
VU_SetWindowTitle(_THIS, SDL_Window * window)
{
}
void
VU_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon)
{
}
void
VU_SetWindowPosition(_THIS, SDL_Window * window)
{
}
void
VU_SetWindowSize(_THIS, SDL_Window * window)
{
}

void
VU_ShowWindow(_THIS, SDL_Window * window)
{
	vuplus_show_window(SDL_TRUE);
}

void
VU_HideWindow(_THIS, SDL_Window * window)
{
	vuplus_show_window(SDL_FALSE);
}

void
VU_RaiseWindow(_THIS, SDL_Window * window)
{
}
void
VU_MaximizeWindow(_THIS, SDL_Window * window)
{
}
void
VU_MinimizeWindow(_THIS, SDL_Window * window)
{
}
void
VU_RestoreWindow(_THIS, SDL_Window * window)
{
}
void
VU_SetWindowGrab(_THIS, SDL_Window * window, SDL_bool grabbed)
{
}

void
VU_DestroyWindow(_THIS, SDL_Window * window)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	eglTerminate(phdata->egl_display);
}

/*****************************************************************************/
/* SDL Window Manager function                                               */
/*****************************************************************************/

SDL_bool
VU_GetWindowWMInfo(_THIS, SDL_Window * window, struct SDL_SysWMinfo *info)
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

#endif /* SDL_VIDEO_DRIVER_VUPLUS && SDL_VIDEO_OPENGL_EGL */

/* vi: set ts=4 sw=4 expandtab: */
