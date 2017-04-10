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

#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_DREAMBOX && SDL_VIDEO_OPENGL_EGL

#include <string.h>

#include "SDL_dreambox.h"
#include "SDL_dreambox_modes.h"

#ifndef DREAMBOX_DEBUG
#define DREAMBOX_DEBUG 1
#endif


/* Display helper functions */
void
dreambox_free_display_mode_data(SDL_DisplayMode * mode)
{
	if (mode->driverdata != NULL) {
		SDL_free(mode->driverdata);
		mode->driverdata = NULL;
	}
}

void
dreambox_show_window(SDL_bool show)
{
	FILE *fp = fopen("/proc/stb/video/alpha", "w");
	
#if DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: %s Window\n", show ? "Show" : "Hide");
#endif
	
	if (fp) {
		fprintf(fp, "%d", show ? 255 : 0);
		fclose(fp);
	}
	else
		SDL_SetError("DREAM: i/o file /proc/stb/video/alpha");
}

int 
dreambox_set_framebuffer_resolution(int width, int height)
{
	int fd;
	struct fb_var_screeninfo vinfo;
	
	fd = open("/dev/fb0", O_RDWR, 0);
	
	if (fd<0) {
		SDL_SetError("DREAM: Open Framebuffer failed!");
		return -1;
	}
	
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == 0) {
		vinfo.xres = width;
		vinfo.yres = height;
		vinfo.xres_virtual = width;
		vinfo.yres_virtual = height * 2;
		vinfo.bits_per_pixel = 32;
		vinfo.activate = FB_ACTIVATE_ALL;
		ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo);
		
#if DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: Set Framebuffer Resolution: %dx%d\n", width, height);
#endif
		
	} else {
		SDL_SetError("DREAM: Open Framebuffer ioctl failed!");
		return -1;
	}
	close(fd);
	return 0;
}

int
dreambox_set_videomode(const char * vmode)
{
	FILE *fp = fopen("/proc/stb/video/videomode", "w");
	
#if DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: Set Videomode: %s\n",vmode);
#endif
	
	if (fp) {
		fprintf(fp, vmode);
		fclose(fp);
	}
	else
		return SDL_SetError("DREAM: i/o file /proc/stb/video/videomode");
	return 0;
}

int  
dreambox_get_videomode(char * dest)
{
	FILE *fp = fopen("/proc/stb/video/videomode", "r");
	char *tmp;
	
	dest[0] = 0;
	
	if (fp) {
		fgets(dest, 16, fp);
		fclose(fp);
		tmp = SDL_strrchr(dest, '\n');
		if (tmp) 
			*tmp = '\0';  
#if DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: Get Videomode: %s\n", dest);
#endif
	}
	else
		return SDL_SetError("DREAM: i/o file /proc/stb/video/videomode");
	return 0;
}

int
dreambox_get_displaymode_from_videomode(const char * vmode, SDL_DisplayMode * mode)
{
	
	unsigned int i;
	char *pch;
	char buffer_w[16];
	char buffer_h[16];
	char buffer_r[16];
	const char types[] = { 'p', 'i', 'x' };
	
	mode->format = SDL_PIXELFORMAT_RGBA8888;
	mode->refresh_rate = 60;
	mode->driverdata = NULL;
	
	for (i = 0; i < 3; i++) {
		pch = SDL_strchr(vmode, types[i]);
		if (pch==NULL)
			continue;
		else {
			buffer_w[0] = 0;
			buffer_h[0] = 0;
			buffer_r[0] = 0;
			switch(types[i]) {
				case 'i':
				case 'p':
					if (types[i] == 'p')
						SDL_sscanf(vmode, "%[^p]p%s", buffer_h, buffer_r);
					else
						SDL_sscanf(vmode, "%[^i]i%s", buffer_h, buffer_r);
					
					mode->h = SDL_atoi(buffer_h);
					if (buffer_r[0] != 0)
						mode->refresh_rate = SDL_atoi(buffer_r);
					
					switch(mode->h) {
						case 480:
							mode->w = 640;
							break;
						case 576:
							mode->w = 720;
							break;
						case 720:
							mode->w = 1280;
							break;
						case 1080:
							mode->w = 1920;
							break;
						case 2160:
							mode->w = 3840;
							break;
						default: fprintf(stderr,"ERROR: unknown mode settings\n"); return 0;
						}
						if (mode->h > 1080) {
							mode->w = 1920;
							mode->h = 1080;
						}
					break;
				case 'x':
					SDL_sscanf(vmode, "%[^x]x%s", buffer_w, buffer_h);
					mode->h = SDL_atoi(buffer_h);
					mode->w = SDL_atoi(buffer_w);
					break;
					
				default: return 0;
			}
			return 1;
		}
	}
}

int 
dreambox_set_displaymode(SDL_DisplayMode * mode)
{
	if ( (dreambox_set_videomode(((SDL_DisplayModeData*)mode->driverdata)->videomode) < 0) ||
		(dreambox_set_framebuffer_resolution(mode->w, mode->h) < 0) ) {
		return -1;
	}
	return 0;
}

/****************************************************************************/
/* SDL_VideoDevice functions declaration                                    */
/****************************************************************************/

void 
DREAM_QuitModes(_THIS)
{
	unsigned int i, j;

	for (i = 0; i < _this->num_displays; i++) {
		SDL_VideoDisplay *display = &_this->displays[i];
		if (i == 0)
			DREAM_SetVideoMode(_this, display, DREAM_InitVideoMode);
		
		for (j = 0; j < display->num_display_modes; j++) {
			SDL_DisplayMode *mode = &display->display_modes[j];
			dreambox_free_display_mode_data(mode);
		}
	}
#if DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: QuitModes: restore old Videomode: %s\n", DREAM_InitVideoMode);
#endif
}

int 
DREAM_InitModes(_THIS)
{
	unsigned int i;
	const char *override = SDL_getenv("SDL_VIDEOMODE");
	char videomodes[256] = {0x0};
	char startmode[16] = {0x0};
	char *tmp;
	
	FILE *fp = fopen("/proc/stb/video/videomode_choices", "r");
	
	if(fp) {
		fgets(videomodes, 256, fp);
		fclose(fp);
		tmp = SDL_strrchr(videomodes, '\n');
		if (tmp) 
			*tmp = '\0'; 
		
		/* test override */
		if( (override) && (SDL_strlen(override) >= 4) &&
				(SDL_strstr(videomodes, override) != NULL) ) {
			SDL_strlcpy(startmode, override, 
				    SDL_strlen(override)+1);
#if DREAMBOX_DEBUG
			fprintf(stderr, "DREAM: InitModes: found SDL_VIDEOMODE=%s\n", startmode);
#endif
		}
		else {
#if DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: InitModes: using default: %s\n", DREAM_InitVideoMode);
#endif
			SDL_strlcpy(startmode, DREAM_InitVideoMode, 
				    SDL_strlen(DREAM_InitVideoMode)+1);
		}

	} else {
		SDL_SetError("DREAM: i/o file /proc/stb/video/videomode_choices");
		return -1;
	}
	
	for (i = 0; i < _this->num_displays; i++) {
		SDL_VideoDisplay *display = &_this->displays[i];
		DREAM_GetDisplayModes(_this, display);

		if (DREAM_SetVideoMode(_this, display, startmode) < 0) {
			SDL_SetError("DREAM: InitModes: can not set mode");
			return -1;
		} else {
#if DREAMBOX_DEBUG
			fprintf(stderr, "DREAM: InitModes: ok\n");
#endif
			return 0;
		}
	}
	SDL_SetError("DREAM: InitModes failed\n");
	return -1;
}

void
DREAM_GetDisplayModes(_THIS, SDL_VideoDisplay * display)
{
	char *videomode;
	char videomodes[256]={0x0};
	SDL_DisplayMode mode;
	SDL_DisplayModeData *modedata;
	char *tmp;
	
	FILE *fp = fopen("/proc/stb/video/videomode_choices", "r");
	
	if(fp) {
		fgets(videomodes, 256, fp);
		fclose(fp);
		tmp = SDL_strrchr(videomodes, '\n');
		if (tmp) 
			*tmp = '\0';
		
		videomode = strtok(videomodes," ");

		while(videomode != NULL)
		{
			if (dreambox_get_displaymode_from_videomode(videomode, &mode)) {
				
				modedata = (SDL_DisplayModeData *) SDL_calloc(1, sizeof(SDL_DisplayModeData));
				if (!modedata) {
					videomode=strtok(NULL," ");
					continue;
				}
				
				mode.driverdata = modedata;
				SDL_strlcpy(modedata->videomode, videomode, SDL_strlen(videomode)+1);
#if DREAMBOX_MODES_DEBUG
				fprintf(stderr,"DREAM: Videomode: %s\n", ((SDL_DisplayModeData*)mode.driverdata)->videomode );
				fprintf(stderr,"DREAM: width: %d\n", mode.w);
				fprintf(stderr,"DREAM: height: %d\n", mode.h);
				fprintf(stderr,"DREAM: refresh_rate: %d\n\n", mode.refresh_rate);
#endif
				if (!SDL_AddDisplayMode(display, &mode)) {
					SDL_free(modedata);
				}
				else
					SDL_SetError("DREAM: SDL_AddDisplayMode: can not add mode");
			}
			videomode=strtok(NULL," ");
		}
	}
	else
		SDL_SetError("DREAM: i/o file /proc/stb/video/videomode_choices");
}

int
DREAM_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
	if (dreambox_set_displaymode(mode) < 0) {
		return -1;
	}
	
	display->current_mode = *mode;
	
	return 0;
}

int
DREAM_SetVideoMode(_THIS, SDL_VideoDisplay * display, const char * vmode)
{
	unsigned int i;
	SDL_DisplayMode mode;

	if (!display->num_display_modes)
		DREAM_GetDisplayModes(_this, display);
	
	for (i = 0; i < display->num_display_modes; ++i) {
		mode = display->display_modes[i];
		if ( SDL_strcmp( ((SDL_DisplayModeData*)mode.driverdata)->videomode, vmode ) == 0 )
			/* mach mode here */
			break;
	}

	if ( DREAM_SetDisplayMode(_this, display, &mode) == 0 )
		return 0;
		
	return -1;
}

#endif /* SDL_VIDEO_DRIVER_DREAMBOX */

/* vi: set ts=4 sw=4 expandtab: */
