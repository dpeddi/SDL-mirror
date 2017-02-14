/* bcmfb based SDL video driver implementation.
*  SDL - Simple DirectMedia Layer
*  Copyright (C) 1997-2012 Sam Lantinga
*  
*  SDL bcmfb backend
*  Copyright (C) 2016 Emanuel Strobel
*/
#include "SDL_config.h"

/* Broadcom hardware acceleration for the SDL framebuffer console driver */

#include "SDL_fbvideo.h"

/* Set up the driver for acceleration */

#define FB_ACCEL_VC4 0xb0
#define FBIO_ACCEL 0x23

#define blitAlphaTest 1
#define blitAlphaBlend 2
#define blitScale 4
#define blitKeepAspectRatio 8

static unsigned int displaylist[1024];
static int ptr;
static int supportblendingflags = 1;

#define P(x, y) do { displaylist[ptr++] = x; displaylist[ptr++] = y; } while (0)
#define C(x) P(x, 0)

static int exec_list();
extern int bcm_accel_init();

extern void bcm_accel_blit(
		int src_addr, int src_width, int src_height, int src_stride, int bpp,
		int dst_addr, int dst_width, int dst_height, int dst_stride,
		int src_x, int src_y, int width, int height,
		int dst_x, int dst_y, int dwidth, int dheight,
		int pal_addr, int flags);

extern void bcm_accel_fill(
		int dst_addr, int dst_width, int dst_height, int dst_stride, int bpp,
		int x, int y, int width, int height,
		int pal_addr, unsigned long color);

extern int bcm_accel_has_alphablending();

extern void BCMFB_Accel(_THIS, __u32 card);
