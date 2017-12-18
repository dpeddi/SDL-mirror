/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* Dummy SDL video driver implementation; this is just enough to make an
 *  SDL-based application THINK it's got a working video driver, for
 *  applications that call SDL_Init(SDL_INIT_VIDEO) when they don't need it,
 *  and also for use as a collection of stubs when porting SDL to a new
 *  platform for which you haven't yet written a valid video driver.
 *
 * This is also a great way to determine bottlenecks: if you think that SDL
 *  is a performance problem for a given platform, enable this driver, and
 *  then see if your application runs faster without video overhead.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "OLED" by Sam Lantinga.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_oledvideo.h"
#include "SDL_nullevents_c.h"
#include "SDL_nullmouse_c.h"

#define OLEDVID_DRIVER_NAME "oled"

#define OLED_DEBUG 1

//#define BGR565(r,g,b) (((r)>>3)<<11 | ((g)>>2)<<5 | (b))
//#define RGB565_BE(r,g,b) (((r)>>3)<<8 | ((g)>>2)<<3 | (b)>>3))

/* Initialization/Query functions */
static int OLED_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **OLED_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *OLED_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int OLED_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void OLED_VideoQuit(_THIS);

/* Hardware surface functions */
static int OLED_AllocHWSurface(_THIS, SDL_Surface *surface);
static int OLED_LockHWSurface(_THIS, SDL_Surface *surface);
static void OLED_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void OLED_FreeHWSurface(_THIS, SDL_Surface *surface);

/* etc. */
static void OLED_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

/* OLED driver bootstrap functions */

static int OLED_Available(void)
{
	const char *envr = SDL_getenv("SDL_VIDEODRIVER");
	
	if ((envr) && (SDL_strcmp(envr, OLEDVID_DRIVER_NAME) == 0)) {
		return(1);
	}

	return(0);
}

static void OLED_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *OLED_CreateDevice(int devindex)
{
	SDL_VideoDevice *this;

	/* Initialize all variables that we clean on shutdown */
	this = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( this ) {
		SDL_memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			SDL_free(this);
		}
		return(0);
	}
	SDL_memset(this->hidden, 0, (sizeof *this->hidden));

	/* Set the function pointers */
	this->VideoInit = OLED_VideoInit;
	this->ListModes = OLED_ListModes;
	this->SetVideoMode = OLED_SetVideoMode;
	this->CreateYUVOverlay = NULL;
	this->SetColors = OLED_SetColors;
	this->UpdateRects = NULL;
	this->VideoQuit = OLED_VideoQuit;
	this->AllocHWSurface = OLED_AllocHWSurface;
	this->CheckHWBlit = NULL;
	this->FillHWRect = NULL;
	this->SetHWColorKey = NULL;
	this->SetHWAlpha = NULL;
	this->LockHWSurface = OLED_LockHWSurface;
	this->UnlockHWSurface = OLED_UnlockHWSurface;
	this->FlipHWSurface = NULL;
	this->FreeHWSurface = OLED_FreeHWSurface;
	this->SetCaption = NULL;
	this->SetIcon = NULL;
	this->IconifyWindow = NULL;
	this->GrabInput = NULL;
	this->GetWMInfo = NULL;
	this->InitOSKeymap = OLED_InitOSKeymap;
	this->PumpEvents = OLED_PumpEvents;

	this->free = OLED_DeleteDevice;
	return this;
}

VideoBootStrap OLED_bootstrap = {
	OLEDVID_DRIVER_NAME, "SDL Dreambox OLED video driver",
	OLED_Available, OLED_CreateDevice
};

int OLED_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	FILE *boxtype_file;
	char boxtype_name[20];
	const char *SDL_fddev;
	
	oled_fd = -1;
	
	/* Initialize the fd */
	SDL_fddev = SDL_getenv("SDL_OLED");
	if ( SDL_fddev == NULL ) {
		SDL_fddev = "/dev/dbox/oled0";
	}
	
	oled_fd = open(SDL_fddev, O_RDWR);
#ifdef OLED_DEBUG
	printf("OLED_VideoInit: oled_fd=%d\n", oled_fd);
#endif
	
	if ( oled_fd < 0 ) {
		SDL_SetError("Unable to open %s", SDL_fddev);
		return(-1);
	}
	
	boxtype_file = fopen("/proc/stb/info/model", "r");
	
	if (boxtype_file)
	{
		fgets(boxtype_name, sizeof(boxtype_name), boxtype_file);
		fclose(boxtype_file);
#ifdef OLED_DEBUG
		printf("OLED_VideoInit: boxtype_name: %s", boxtype_name);
#endif
		if( SDL_strncmp(boxtype_name, "dm900", 5) == 0 || SDL_strncmp(boxtype_name, "dm920", 5) == 0)
		{
			/* BGR_565_LE */
			oled_type = DM900;
			vformat->BitsPerPixel = 16;
			vformat->BytesPerPixel = 2;
			this->info.current_w = 400;
			this->info.current_h = 240;
		}
		else if(SDL_strncmp(boxtype_name, "dm7080", 6) == 0)
		{
			oled_type = DM7080;
			vformat->BitsPerPixel = 8;
			vformat->BytesPerPixel = 1;
			this->info.current_w = 132;
			this->info.current_h = 64;
		}
		else if(SDL_strncmp(boxtype_name, "dm820", 5) == 0)
		{
			/* RGB_565_BE */
			oled_type = DM820;
			vformat->BitsPerPixel = 16;
			vformat->BytesPerPixel = 2;
			this->info.current_w = 96;
			this->info.current_h = 64;
		}
		else {
			SDL_SetError("OLED_VideoInit: unsupported boxtype");
			return(-1);
		}
#ifdef OLED_DEBUG
		printf("OLED_VideoInit: oled_type=%d bpp=%d\n", oled_type, vformat->BitsPerPixel);
#endif
	}
	else
	{
		SDL_SetError("Unable to open /proc/stb/info/model");
		return(-1);
	}
	
	vformat->Rmask = 0;
	vformat->Gmask = 0;
	vformat->Bmask = 0;
	
	this->info.wm_available = 0;
	this->info.hw_available = 0;
	this->info.video_mem = this->info.current_w * this->info.current_h * (vformat->BitsPerPixel / 8)/1024;

	/* We're done! */
	return(0);
}

SDL_Rect **OLED_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	static SDL_Rect r;
	static SDL_Rect *rs[2] = { &r, NULL };
	
	r.x = r.y = 0;
	r.w = this->info.current_w;
	r.h = this->info.current_h;
	
	if((flags & SDL_FULLSCREEN) && (format == NULL)) {
#ifdef OLED_DEBUG
			printf("OLED_ListModes: SDL_FULLSCREEN DM900\n");
#endif
			return rs;
	}

	switch(oled_type) {
		case DM900:
		{
			if(format->BitsPerPixel == 16)
			{
#ifdef OLED_DEBUG
				printf("OLED_ListModes: BitsPerPixel=16 DM900\n");
#endif
				return rs;
			}
			break;
		}
		case DM820:
		{
			if(format->BitsPerPixel == 16)
			{
#ifdef OLED_DEBUG
				printf("OLED_ListModes: BitsPerPixel=16 DM820\n");
#endif
				return rs;
			}
			break;
		}
		case DM7080:
		{
			if(format->BitsPerPixel == 8)
			{
#ifdef OLED_DEBUG
				printf("OLED_ListModes: BitsPerPixel=8 DM7080\n");
#endif
				return rs;
			}
			break;
		}
		default:
		{
#ifdef OLED_DEBUG
			printf("OLED_ListModes: BitsPerPixel=%d default - NULL\n", format->BitsPerPixel);
#endif
		}
	}
#ifdef OLED_DEBUG
	printf("OLED_ListModes: BitsPerPixel=%d - NULL\n", format->BitsPerPixel);
#endif
	return NULL;
}

SDL_Surface *OLED_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	int i;
	Uint32 Rmask = 0;
	Uint32 Gmask = 0;
	Uint32 Bmask = 0;
	Uint32 red_length = 0;
	Uint32 green_length = 0;
	Uint32 blue_length = 0;
	Uint32 red_offset = 0;
	Uint32 green_offset = 0;
	Uint32 blue_offset = 0;

	switch(oled_type) {
		case DM900:
		{
			/* BGR565: RRRRRGGGGGGBBBBB */
			red_length = 5;
			red_offset = 11;
			green_length = 6;
			green_offset = 5;
			blue_length = 5;
			blue_offset = 0;
			break;
		}
		case DM820:
		{
			/* RGB565: GGGRRRRRBBBBBGGG */
			red_length = 5;
			red_offset = 8;
			green_length = 6;
			green_offset = 13;
			blue_length = 5;
			blue_offset = 3;
			break;
		}
		case DM7080:
		{
			red_length = 8;
			red_offset = 0;
			green_length = 8;
			green_offset = 0;
			blue_length = 8;
			blue_offset = 0;
			break;
		}
	}

	Rmask = 0;
	for ( i=0; i<red_length; ++i ) {
		Rmask <<= 1;
		Rmask |= (0x00000001<<red_offset);
	}
	Gmask = 0;
	for ( i=0; i<green_length; ++i ) {
		Gmask <<= 1;
		Gmask |= (0x00000001<<green_offset);
	}
	Bmask = 0;
	for ( i=0; i<blue_length; ++i ) {
		Bmask <<= 1;
		Bmask |= (0x00000001<<blue_offset);
	}
	
	if ( this->hidden->buffer ) {
		SDL_free( this->hidden->buffer );
	}

	this->hidden->buffer = SDL_malloc(width * height * (bpp / 8));
	if ( ! this->hidden->buffer ) {
		SDL_SetError("Couldn't allocate buffer for requested mode");
		return(NULL);
	}
#ifdef OLED_DEBUG
	printf("OLED_SetVideoMode: %dx%d@(%d)bpp\n", width, height, bpp);
#endif
	SDL_memset(this->hidden->buffer, 0, width * height * (bpp / 8));

	/* Allocate the new pixel format for the screen */
	if ( ! SDL_ReallocFormat(current, bpp, Rmask, Gmask, Bmask, 0) ) {
		SDL_free(this->hidden->buffer);
		this->hidden->buffer = NULL;
		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return(NULL);
	}

	/* Set up the new mode framebuffer */
	current->flags &= SDL_FULLSCREEN;
	current->flags |= SDL_HWSURFACE;
	
	this->hidden->w = current->w = width;
	this->hidden->h = current->h = height;
	current->pitch = current->w * (bpp / 8);
	current->pixels = this->hidden->buffer;

	this->UpdateRects = OLED_UpdateRects;
	/* We're done */
	return(current);
}

/* We don't actually allow hardware surfaces other than the main one */
static int OLED_AllocHWSurface(_THIS, SDL_Surface *surface)
{
#ifdef OLED_DEBUG
	printf("OLED_AllocHWSurface\n");
#endif
	return(-1);
}
static void OLED_FreeHWSurface(_THIS, SDL_Surface *surface)
{
#ifdef OLED_DEBUG
	printf("OLED_FreeHWSurface\n");
#endif
	return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int OLED_LockHWSurface(_THIS, SDL_Surface *surface)
{
#ifdef OLED_DEBUG
	printf("OLED_LockHWSurface\n");
#endif
	return(0);
}

static void OLED_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
#ifdef OLED_DEBUG
	printf("OLED_UnlockHWSurface\n");
#endif
}

static void OLED_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
#ifdef OLED_DEBUG
	printf("OLED_UpdateRects\n");
#endif
	write(oled_fd, this->screen->pixels, this->screen->pitch * this->screen->h);
}

int OLED_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
#ifdef OLED_DEBUG
	printf("OLED_SetColors\n");
#endif
	return(1);
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void OLED_VideoQuit(_THIS)
{
	if (this->screen->pixels != NULL)
	{
		SDL_free(this->screen->pixels);
		this->screen->pixels = NULL;
	}

	/* Close file descriptors */
	if ( oled_fd > 0 ) {
		/* We're all done with the oled */
		close(oled_fd);
		oled_fd = -1;
	}
#ifdef OLED_DEBUG
	printf("OLED_VideoQuit\n");
#endif
}
