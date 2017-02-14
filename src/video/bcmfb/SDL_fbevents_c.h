/* bcmfb based SDL video driver implementation.
*  SDL - Simple DirectMedia Layer
*  Copyright (C) 1997-2012 Sam Lantinga
*  
*  SDL bcmfb backend
*  Copyright (C) 2016 Emanuel Strobel
*/
#include "SDL_config.h"

#include "SDL_fbvideo.h"

/* Variables and functions exported by SDL_sysevents.c to other parts 
   of the native video subsystem (SDL_sysvideo.c)
*/
extern int BCMFB_OpenKeyboard(_THIS);
extern void BCMFB_CloseKeyboard(_THIS);
extern int BCMFB_OpenMouse(_THIS);
extern void BCMFB_CloseMouse(_THIS);
extern int BCMFB_EnterGraphicsMode(_THIS);
extern int BCMFB_InGraphicsMode(_THIS);
extern void BCMFB_LeaveGraphicsMode(_THIS);

extern void BCMFB_InitOSKeymap(_THIS);
extern void BCMFB_PumpEvents(_THIS);
