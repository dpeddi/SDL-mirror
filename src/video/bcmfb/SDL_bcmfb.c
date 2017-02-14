/* bcmfb based SDL video driver implementation.
*  SDL - Simple DirectMedia Layer
*  Copyright (C) 1997-2012 Sam Lantinga
*  
*  SDL bcmfb backend
*  Copyright (C) 2016 Emanuel Strobel
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#include "SDL_config.h"
#include "SDL_video.h"
#include "../SDL_blit.h"
#include "../SDL_surface.h"
#include "SDL_bcmfb.h"

#define BCMFB_ACCEL_DEBUG

/* SDL_bcmfb */
int bcm_accel_init()
{
	if (exec_list())
	{
		return 1;
	}
	else {
		fprintf(stderr, "BCMFB accel interface found\n");
	}
	/* now test for blending flags support */
	P(0x60, 0);
	if (exec_list())
	{
		fprintf(stderr, "BCMFB support blending flags 0x60 not available\n");
		supportblendingflags = 0;
	}
	return 0;
}

static int exec_list()
{
	SDL_VideoDevice *this = current_video;
	int ret;
	struct
	{
		void *ptr;
		int len;
	} l;

	l.ptr = displaylist;
	l.len = ptr;
	ret = ioctl(console_fd, FBIO_ACCEL, &l);
	ptr = 0;
	if (ret != 0) {
		SDL_SetError("BCMFB accel ioctl faild!");
	}
	return ret;
}

int bcm_accel_has_alphablending()
{
	return supportblendingflags;
}

void bcm_accel_fill(int dst_addr, int dst_width, int dst_height, int dst_stride, int bpp,
		int x, int y, int width, int height,
		int pal_addr, unsigned long color)
{
	C(0x43); // reset source
	C(0x53); // reset dest
	C(0x5b); // reset pattern
	C(0x67); // reset blend
	C(0x75); // reset output

	// clear dest surface
	P(0x0, 0);
	P(0x1, 0);
	P(0x2, 0);
	P(0x3, 0);
	P(0x4, 0);
	C(0x45);

	// clear src surface
	P(0x0, 0);
	P(0x1, 0);
	P(0x2, 0);
	P(0x3, 0);
	P(0x4, 0);
	C(0x5);

	P(0x2d, color);

	P(0x2e, x); // prepare output rect
	P(0x2f, y);
	P(0x30, width);
	P(0x31, height);
	C(0x6e); // set this rect as output rect

	P(0x0, dst_addr); // prepare output surface
	P(0x1, dst_stride);
	P(0x2, dst_width);
	P(0x3, dst_height);
	
	switch (bpp)
	{
	case 8:
		P(0x4, 0x12e40008); // indexed 8bit
		P(0x78, 256);
		P(0x79, pal_addr);
		P(0x7a, 0x7e48888);
		break;
	case 16:
		P(0x4, 0x06e40565); // RGB_565 16 bit
		break;
	case 32:
		P(0x4, 0x7e48888); // format: ARGB 8888
		break;
	}
	
	C(0x69); // set output surface

	P(0x6f, 0);
	P(0x70, 0);
	P(0x71, 2);
	P(0x72, 2);
	C(0x73); // select color keying

	C(0x77);  // do it

	exec_list();
}

void bcm_accel_blit(int src_addr, int src_width, int src_height, int src_stride, int bpp,
		int dst_addr, int dst_width, int dst_height, int dst_stride,
		int src_x, int src_y, int width, int height,
		int dst_x, int dst_y, int dwidth, int dheight,
		int pal_addr, int flags)
{
	C(0x43); // reset source
	C(0x53); // reset dest
	C(0x5b);  // reset pattern
	C(0x67); // reset blend
	C(0x75); // reset output

	P(0x0, src_addr); // set source addr
	P(0x1, src_stride);  // set source pitch
	P(0x2, src_width); // source width
	P(0x3, src_height); // height
	switch (bpp)
	{
	case 8:
		P(0x4, 0x12e40008); // indexed 8bit
		P(0x78, 256);
		P(0x79, pal_addr);
		P(0x7a, 0x7e48888);
		break;
	case 16:
		P(0x4, 0x06e40565); // RGB_565 16 bit
		break;
	case 32:
		P(0x4, 0x7e48888); // format: ARGB 8888
		break;
	}

	C(0x5); // set source surface (based on last parameters)

	P(0x2e, src_x); // define  rect
	P(0x2f, src_y);
	P(0x30, width);
	P(0x31, height);

	C(0x32); // set this rect as source rect

	P(0x0, dst_addr); // prepare output surface
	P(0x1, dst_stride);
	P(0x2, dst_width);
	P(0x3, dst_height);
	P(0x4, 0x7e48888);

	C(0x69); // set output surface

	P(0x2e, dst_x); // prepare output rect
	P(0x2f, dst_y);
	P(0x30, dwidth);
	P(0x31, dheight);

	C(0x6e); // set this rect as output rect

	/* blend flags... 
	 * We'd really like some blending support in the drivers,
	 * to avoid punching holes in the osd 
	 */
	if (supportblendingflags && flags) P(0x80, flags);

	C(0x77);  // do it

	exec_list();
}

/* Wait for vertical retrace */
static void WaitVBL(_THIS)
{
#ifdef BCMFB_ACCEL_DEBUG
	fprintf(stderr, "WaitVBL\n");
#endif

#ifdef FBIOWAITRETRACE /* Heheh, this didn't make it into the main kernel */
	ioctl(console_fd, FBIOWAITRETRACE, 0);
#endif
	return;
}

static void WaitIdle(_THIS)
{
#ifdef BCMFB_ACCEL_DEBUG
	fprintf(stderr, "WaitIdle\n");
#endif
	return;
}

/* Sets video mem colorkey and accelerated blit function */
static int SetHWColorKey(_THIS, SDL_Surface *surface, Uint32 key)
{
#ifdef BCMFB_ACCEL_DEBUG
	fprintf(stderr, "SetHWColorKey\n");
#endif
	return(0);
}

static int FillHWRect(_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color)
{
	Uint32 dst_base;
	int rectX, rectY, rectW, rectH,
		dst_width, dst_height, dst_addr,
		pal_addr, bpp, bypp, stride, fb_size;
	unsigned long dst_color;

	/* Set the destination pixel format */
	bpp = dst->format->BitsPerPixel;
	bypp = bpp/8;
	
	/* convert color */
	dst_color = (unsigned long)color;
	
	/* Calculate base coordinates */
	rectX = rect->x;
	rectY = rect->y;
	rectW = rect->w;
	rectH = rect->h;
	
	/* Calculate destination base coordinates */
	dst_width = dst->w;
	dst_height = dst->h;
	stride = dst_width * bypp;
	
	if ( dst == this->screen ) {
		SDL_mutexP(hw_lock);
	}
				
	/* set current pal */
	pal_addr = this->physpal;
	
	/* Calculate destination base address */
	dst_addr =  (Uint8 *)dst->pixels +
				(Uint16)rect->y*dst->pitch +
				(Uint16)rect->x*dst->format->BytesPerPixel;

	/* Calculate fb_size for destination */
	fb_size = stride * dst_height;
	
#ifdef BCMFB_ACCEL_DEBUG
	fprintf(stderr, "FillHWRect - dst_addr: %p\n\t dst_width: %d\n\t dst_height: %d\n\t stride: %d\n\t rectX: %d\n\t rectY: %d\n\t rectW: %d\n\t rectH: %d\n\t color: %d\n", 
					dst_addr, dst_width, dst_height, stride,
					rectX, rectY, rectW, rectH, dst_color);
#endif

	/* now accel */
	bcm_accel_fill(dst_addr, dst_width, dst_height, stride, bpp,
					rectX, rectY, rectW, rectH, pal_addr, dst_color);
	
	BCMFB_AddBusySurface(dst);
	
	if ( dst == this->screen ) {
		SDL_mutexV(hw_lock);
	}
	return(0);
}

static int HWAccelBlit(SDL_Surface *src, SDL_Rect *srcrect,
                       SDL_Surface *dst, SDL_Rect *dstrect)
{
	SDL_VideoDevice *this = current_video;
	int src_addr, src_rectX, src_rectY, src_rectW, 
		src_rectH, src_stride, dst_width, dst_height, 
		dst_addr, dst_rectX, dst_rectY, dst_rectW, 
		dst_rectH, dst_stride, src_width, src_height,
		flip_addr, flip_size, bypp, bpp, pal_addr, flags;
	
	/* Set the destination pixel format */
	bpp = dst->format->BitsPerPixel;
	bypp = bpp/8;
	
	/* Calculate destination base coordinates */
	dst_rectX = dstrect->x;
	dst_rectY = dstrect->y;
	dst_rectW = dstrect->w;
	dst_rectH = dstrect->h;
	dst_width = dst->w;
	dst_height = dst->h;
	dst_stride = dst_width * bypp;

	/* Calculate source base coordinates */
	src_rectX = srcrect->x;
	src_rectY = srcrect->y;
	src_rectW = srcrect->w;
	src_rectH = srcrect->h;
	src_width = src->w;
	src_height = src->h;
	src_stride = src_width * bypp;
	
	/* Calculate source base address */
	src_addr = (Uint8 *)src->pixels +
				(Uint16)srcrect->y*src->pitch +
				(Uint16)srcrect->x*src->format->BytesPerPixel;
	
	/* Calculate destination address */	
	dst_addr =	(Uint8 *)accel_mem +
				(Uint16)dstrect->y * dst->pitch +
				(Uint16)dstrect->x * dst->format->BytesPerPixel;
	
	/* Calculate end destination address */
	flip_addr = (Uint8 *)dst->pixels +
				(Uint16)dstrect->y * dst->pitch +
				(Uint16)dstrect->x * dst->format->BytesPerPixel;
				
	flip_size = dstrect->y * dst_stride;
	
	/* init flags */
	flags = 0;
	if ( (src->flags & SDL_SRCALPHA) ) {
		flags += blitAlphaTest;
		flags += blitAlphaBlend;
	}
	if ((src_rectX != dst_rectX) || (src_rectY != dst_rectY)) {
		flags += blitScale;
	}
	if ((src_width == dst_width) && (src_height == dst_height)) {
		flags += blitKeepAspectRatio;
	}
	
	/* set current pal */
	pal_addr = this->physpal;
	
#ifdef BCMFB_ACCEL_DEBUG
	fprintf(stderr, "HWAccelBlit - dst_addr: %p\n\t dst_width: %d\n\t dst_height: %d\n\t dst_stride: %d\n\t dst_rectX: %d\n\t dst_rectY: %d\n\t dst_rectW: %d\n\t dst_rectH: %d\n", 
					dst_addr, dst_width, dst_height, dst_stride, 
					dst_rectX, dst_rectY, dst_rectW, dst_rectH);
					
	fprintf(stderr, "HWAccelBlit - src_addr: %p\n\t src_width: %d\n\t src_height: %d\n\t src_stride: %d\n\t src_rectX: %d\n\t src_rectY: %d\n\t src_rectW: %d\n\t src_rectH: %d\n\t flip_addr: %p\n\t flip_size: %d\n", 
					src_addr, src_width, src_height, src_stride, 
					src_rectX, src_rectY, src_rectW, src_rectH, flip_addr, flip_size);
#endif

	/* now accel */
	bcm_accel_blit(src_addr, src_width, src_height, src_stride, bpp,
					dst_addr, dst_width, dst_height, dst_stride,
					src_rectX, src_rectY, src_rectW, src_rectH,
					dst_rectX, dst_rectY, dst_rectW, dst_rectH,
					pal_addr, flags);
					
	/* flip rect to screen */
	memcpy(flip_addr, src_addr, flip_size);
		
	BCMFB_AddBusySurface(dst);
	
	if ( dst == this->screen ) {
		SDL_mutexV(hw_lock);
	}
	
	return(0);
}

static int CheckHWBlit(_THIS, SDL_Surface *src, SDL_Surface *dst)
{
	if (!bcm_can_accel(dst)) {
		return 0;
	}
	
	int accelerated;

	/* Set initial acceleration on */
	src->flags |= SDL_HWACCEL;

	/* Set the surface attributes */
	if ( (src->flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
		if ( ! this->info.blit_hw_A ) {
			src->flags &= ~SDL_HWACCEL;
		}
	}
	if ( (src->flags & SDL_SRCCOLORKEY) == SDL_SRCCOLORKEY ) {
		if ( ! this->info.blit_hw_CC ) {
			src->flags &= ~SDL_HWACCEL;
		}
	}

	/* Check to see if final surface blit is accelerated */
	accelerated = !!(src->flags & SDL_HWACCEL);
	if ( accelerated ) {
		src->map->hw_blit = HWAccelBlit;
	}
#ifdef BCMFB_ACCEL_DEBUG
	fprintf(stderr, "CheckHWBlit - return(accelerated=%d)n", accelerated);
#endif
	return(accelerated);
}

void BCMFB_Accel(_THIS, __u32 card)
{
	/* We have hardware accelerated surface functions */
	this->CheckHWBlit = CheckHWBlit;
	wait_vbl = WaitVBL;
	wait_idle = WaitIdle;

	/* The BCMFB has an accelerated color fill */
	this->info.blit_fill = 1;
	this->FillHWRect = FillHWRect;

	/* The BCMFB has accelerated blit */
	this->info.blit_hw = 1;
	this->info.blit_hw_A = 1;
	this->info.blit_hw_CC = 1;
	this->SetHWColorKey = SetHWColorKey;
}
