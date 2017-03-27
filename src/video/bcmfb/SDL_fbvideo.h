/* bcmfb based SDL video driver implementation.
*  SDL - Simple DirectMedia Layer
*  Copyright (C) 1997-2012 Sam Lantinga
*  
*  SDL bcmfb backend
*  Copyright (C) 2016 Emanuel Strobel
*/
#include "SDL_config.h"

#ifndef _SDL_fbvideo_h
#define _SDL_fbvideo_h

#include <sys/types.h>
#include <termios.h>
#include <linux/fb.h>

#include "SDL_mouse.h"
#include "SDL_mutex.h"
#include "../SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

typedef void BCMFB_bitBlit(
		Uint8 *src_pos,
		int src_right_delta,	/* pixels, not bytes */
		int src_down_delta,		/* pixels, not bytes */
		Uint8 *dst_pos,
		int dst_linebytes,
		int width,
		int height);

/* This is the structure we use to keep track of video memory */
typedef struct vidmem_bucket {
	struct vidmem_bucket *prev;
	int used;
	int dirty;
	char *base;
	unsigned int size;
	struct vidmem_bucket *next;
} vidmem_bucket;

/* Private display data */
struct SDL_PrivateVideoData {

	struct fb_var_screeninfo cache_vinfo;
	struct fb_var_screeninfo saved_vinfo;
	int saved_cmaplen;
	__u16 *saved_cmap;

	int console_fd;
	int kbd_index;
	int m_index;
	int kbd_fd[32];
	int mice_fd[32];
	int panel;
	int kbd;
	int arc;
	int rc;
	
	char *mapped_mem;
	char *accel_mem;
	char *shadow_mem;
	int mapped_memlen;
	int accel_offset;
	int accel_memlen;
	int flip_page;
	char *flip_address[2];
	int rotate;
	int shadow_fb;				/* Tells whether a shadow is being used. */
	BCMFB_bitBlit *blitFunc;
	int physlinebytes;			/* Length of a line in bytes in physical fb */

#define NUM_MODELISTS	4		/* 8, 16, 24, and 32 bits-per-pixel */
	int SDL_nummodes[NUM_MODELISTS];
	SDL_Rect **SDL_modelist[NUM_MODELISTS];

	vidmem_bucket surfaces;
	int surfaces_memtotal;
	int surfaces_memleft;

	SDL_mutex *hw_lock;
	int switched_away;
	struct fb_var_screeninfo screen_vinfo;
	Uint32 screen_arealen;
	Uint8 *screen_contents;
	__u16  screen_palette[3*256];

	void (*wait_vbl)(_THIS);
	void (*wait_idle)(_THIS);
};
/* Old variable names */
#define console_fd			(this->hidden->console_fd)
#define kbd_fd				(this->hidden->kbd_fd)
#define kbd_index			(this->hidden->kbd_index)
#define mice_fd				(this->hidden->mice_fd)
#define m_index				(this->hidden->m_index)
#define panel				(this->hidden->panel)
#define kbd					(this->hidden->kbd)
#define arc					(this->hidden->arc)
#define rc					(this->hidden->rc)
#define cache_vinfo			(this->hidden->cache_vinfo)
#define saved_vinfo			(this->hidden->saved_vinfo)
#define saved_cmaplen		(this->hidden->saved_cmaplen)
#define saved_cmap			(this->hidden->saved_cmap)
#define mapped_mem			(this->hidden->mapped_mem)
#define shadow_mem			(this->hidden->shadow_mem)
#define mapped_memlen		(this->hidden->mapped_memlen)
#define mapped_offset		(this->hidden->mapped_offset)
#define accel_memlen       	(this->hidden->accel_memlen)
#define accel_offset     	(this->hidden->accel_offset)
#define accel_mem       	(this->hidden->accel_mem)
#define flip_page			(this->hidden->flip_page)
#define flip_address		(this->hidden->flip_address)
#define rotate				(this->hidden->rotate)
#define shadow_fb			(this->hidden->shadow_fb)
#define blitFunc			(this->hidden->blitFunc)
#define physlinebytes		(this->hidden->physlinebytes)
#define SDL_nummodes		(this->hidden->SDL_nummodes)
#define SDL_modelist		(this->hidden->SDL_modelist)
#define surfaces			(this->hidden->surfaces)
#define surfaces_memtotal	(this->hidden->surfaces_memtotal)
#define surfaces_memleft	(this->hidden->surfaces_memleft)
#define hw_lock				(this->hidden->hw_lock)
#define switched_away		(this->hidden->switched_away)
#define screen_vinfo		(this->hidden->screen_vinfo)
#define screen_arealen		(this->hidden->screen_arealen)
#define screen_contents		(this->hidden->screen_contents)
#define screen_palette		(this->hidden->screen_palette)
#define wait_vbl			(this->hidden->wait_vbl)
#define wait_idle			(this->hidden->wait_idle)


/* These functions are defined in SDL_fbvideo.c */
extern void BCMFB_SavePaletteTo(_THIS, int palette_len, __u16 *area);
extern void BCMFB_RestorePaletteFrom(_THIS, int palette_len, __u16 *area);

/* These are utility functions for working with video surfaces */

static __inline__ void BCMFB_AddBusySurface(SDL_Surface *surface)
{
	((vidmem_bucket *)surface->hwdata)->dirty = 1;
}

static __inline__ int BCMFB_IsSurfaceBusy(SDL_Surface *surface)
{
	return ((vidmem_bucket *)surface->hwdata)->dirty;
}

static __inline__ void BCMFB_WaitBusySurfaces(_THIS)
{
	vidmem_bucket *bucket;

	/* Wait for graphic operations to complete */
	wait_idle(this);

	/* Clear all surface dirty bits */
	for ( bucket=&surfaces; bucket; bucket=bucket->next ) {
		bucket->dirty = 0;
	}
}

static __inline__ void BCMFB_dst_to_xy(_THIS, SDL_Surface *dst, int *x, int *y)
{
	*x = (long)((char *)dst->pixels - mapped_mem)%this->screen->pitch;
	*y = (long)((char *)dst->pixels - mapped_mem)/this->screen->pitch;
	if ( dst == this->screen ) {
		*x += this->offset_x;
		*y += this->offset_y;
	}
}

#endif /* _SDL_fbvideo_h */
