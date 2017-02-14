/* bcmfb based SDL video driver implementation.
*  SDL - Simple DirectMedia Layer
*  Copyright (C) 1997-2012 Sam Lantinga
*  
*  SDL bcmfb backend
*  Copyright (C) 2016 Emanuel Strobel
*/
#include "SDL_config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#include <linux/keyboard.h>
#include <linux/input.h>

#include "SDL_timer.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"
#include "SDL_fbvideo.h"
#include "SDL_fbevents_c.h"

//#define DEBUG_KEYBOARD
//#define DEBUG_MOUSE

static int posted = 0;
static SDLKey KeyTranslate(unsigned short code, int type);

void BCMFB_InitOSKeymap(_THIS)
{
	return;
}

int BCMFB_InGraphicsMode(_THIS)
{
	return(kbd_fd[0]);
}

int BCMFB_EnterGraphicsMode(_THIS)
{
	return(kbd_fd[0]);
}

void BCMFB_LeaveGraphicsMode(_THIS)
{
	return;
}

void BCMFB_CloseKeyboard(_THIS)
{
	int i;
	for (i = 0; i < 32; i++) {
		if (kbd_fd[i] != -1) {
			close(kbd_fd[i]);
		}
		kbd_fd[i] = -1;
	}
	kbd_index = -1;
	panel = -1;
	kbd = -1;
	arc = -1;
	rc = -1;
}

int BCMFB_OpenKeyboard(_THIS)
{
	unsigned char mask[EV_MAX/8 + 1];
	char device[64], name[128];
	int type=0;
	int fd, i, j;
	
	kbd_index = -1;
	panel = -1;
	kbd = -1;
	arc = -1;
	rc = -1;
	for (i = 0; i < 32; i++) {
		kbd_fd[i] = -1;
	}
	
	for(i=0; i<32; i++ )
	{
		snprintf(device, sizeof(device), "/dev/input/event%i", i);
		if(access(device, R_OK)) continue;
		fd = open(device, O_RDONLY);
		if(fd<0)continue;

		type=0;
		name[0] = 0;

        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
		ioctl(fd, EVIOCGBIT(0, sizeof(mask)), mask);
		close(fd);

		for(j=0; j<EV_MAX; j++)
		{
			if(mask[(j)/8] & (1 << ((j)%8)))
			{
				switch(j)
				{
					case EV_KEY: type |= 1; break;
					case EV_REP: type |= 2;	break;
					case EV_REL: type |= 4; break;
					//case EV_LED: type |= 8; break;
				}
			}
		}
		if(type==3)
		{
			if (SDL_strncmp(name, "dreambox front panel", 20) == 0)
			{
				if((kbd_fd[++kbd_index]=open(device, O_RDONLY|O_NONBLOCK)) == -1)
				{
					SDL_SetError("Unable to open %s",name);
					exit(0);
				}

				else {
					panel = kbd_fd[kbd_index];
#ifdef DEBUG_KEYBOARD
					fprintf(stderr,"'%s' found %s\n",name,device);
#endif
				}
			}
			else if (SDL_strncmp(name, "dreambox remote control", 23) == 0)
			{
				if((kbd_fd[++kbd_index]=open(device, O_RDONLY|O_NONBLOCK)) == -1)
				{
					SDL_SetError("Unable to open %s",name);
					exit(0);
				}
				else {
					rc = kbd_fd[kbd_index];
#ifdef DEBUG_KEYBOARD
					fprintf(stderr,"'%s' found %s\n",name,device);
#endif
				}
			}
			else if (SDL_strncmp(name, "dreambox advanced remote control", 32) == 0 )
			{
				if((kbd_fd[++kbd_index]=open(device, O_RDONLY|O_NONBLOCK)) == -1)
				{
					SDL_SetError("Unable to open %s",name);
					exit(0);
				}
				else {
					arc = kbd_fd[kbd_index];
#ifdef DEBUG_KEYBOARD
					fprintf(stderr,"'%s' found %s\n",name,device);
#endif
				}
			}
			else if (SDL_strncmp(name, "dreambox ir keyboard", 20) == 0)
			{
				if((kbd_fd[++kbd_index]=open(device, O_RDONLY|O_NONBLOCK)) == -1)
				{
					SDL_SetError("Unable to open %s",name);
					exit(0);
				}

				else {
					kbd = kbd_fd[kbd_index];
#ifdef DEBUG_KEYBOARD
					fprintf(stderr,"'%s' found %s\n",name,device);
#endif
				}
			}
			else
			{
				if((kbd_fd[++kbd_index]=open(device, O_RDONLY|O_NONBLOCK)) == -1)
				{
					SDL_SetError("Unable to open %s",name);
					exit(0);
				}
#ifdef DEBUG_KEYBOARD
				else {
					fprintf(stderr,"'USB: %s' found %s\n",name,device);
				}
#endif
			}
		}
	}
#ifdef DEBUG_KEYBOARD
	for (i = 0; i < 32; i++) {
		if (kbd_fd[i] > -1) {
			fprintf(stderr,"kbd_fd[%d] = %d\n",i,kbd_fd[i]);
		}
	}
#endif /* DEBUG_KEYBOARD */

 	return(kbd_fd[0]);
}

void BCMFB_CloseMouse(_THIS)
{
	int i;
	for (i = 0; i < 32; i++) {
		if (mice_fd[i] != -1) {
			close(mice_fd[i]);
		}
		mice_fd[i] = -1;
	}
	m_index = -1;
}

int BCMFB_OpenMouse(_THIS)
{
	unsigned char mask[EV_MAX/8 + 1];
	char device[64], name[128];
	int type=0;
	int fd, i, j;
	
	for(i=0; i<32; i++ )
	{
		snprintf(device, sizeof(device), "/dev/input/event%i", i);
		if(access(device, R_OK)) continue;
		fd = open(device, O_RDONLY);
		if(fd<0)continue;

		type=0;
		name[0] = 0;

		ioctl(fd, EVIOCGNAME(sizeof(name)), name);
		ioctl(fd, EVIOCGBIT(0, sizeof(mask)), mask);
		close(fd);

		for(j=0; j<EV_MAX; j++)
		{
			if(mask[(j)/8] & (1 << ((j)%8)))
			{
				switch(j)
				{
						case EV_KEY: type |= 1; break;
						case EV_REP: type |= 2;	break;
						case EV_REL: type |= 4; break;
				}
			}
		}
		if(type==5)
		{
			if(SDL_strncmp(name, "dreambox ir mouse", 17) != 0 )
			{
				if((mice_fd[++m_index]=open(device, O_RDONLY|O_NONBLOCK)) == -1)
				{
					SDL_SetError("Unable to open %s",name);
						exit(0);
				}
#ifdef DEBUG_MOUSE
				else fprintf(stderr,"'%s' found %s\n",name,device);
#endif
			}
			else {
				if((mice_fd[++m_index]=open(device, O_RDONLY|O_NONBLOCK)) == -1)
				{
					SDL_SetError("Unable to open %s",name);
					exit(0);
				}
#ifdef DEBUG_MOUSE
				else fprintf(stderr,"'%s' found %s\n",name,device);
#endif
			}
		}
	}
#ifdef DEBUG_MOUSE
	for (i = 0; i < 32; i++) {
		if (mice_fd[i] > -1) {
			fprintf(stderr,"mice_fd[%d] = %d\n",i,mice_fd[i]);
		}
	}
#endif /* DEBUG_MOUSE */
	return(mice_fd[0]);
}

static void handle_mouse(_THIS, int m_fd)
{
	if (m_fd < 0)
		return;
	
	struct input_event buffer[32];
	int i;
	int n = 0;
	int m_x = 0;
	int m_y = 0;
	Uint8 button;
	Uint8 state;

	while(1) {
		int bytesRead = read(m_fd, (char *)(buffer) + n, sizeof(buffer) - n);
		if (bytesRead == 0) {
			fprintf(stderr,"Warning: Got EOF from the input device.\n");
			return;
		}
		if (bytesRead == -1) {
			if (errno != EAGAIN)
				fprintf(stderr,"Warning: Could not read from input device: %s\n", strerror(errno));
			break;
		}

		n += bytesRead;
		if (n % sizeof(buffer[0]) == 0)
			break;
	}
	n /= sizeof(buffer[0]);

	for (i = 0; i < n; ++i) {
		struct input_event *data = &buffer[i];

		SDL_bool unknown = SDL_FALSE;
		if (data->type == EV_ABS) {
			if (data->code == ABS_X) {
				m_x = (signed char)data->value;
			} 
			else if (data->code == ABS_Y) {
				m_y = (signed char)data->value;
			} 
			else {
				unknown = SDL_TRUE;
			}
		} 
		else if (data->type == EV_REL) {
			if (data->code == REL_X) {
				m_x += (signed char)data->value;
			} 
			else if (data->code == REL_Y) {
				m_y += (signed char)data->value;
			} 
			else {
				unknown = SDL_TRUE;
			}
		} 
		else if (data->type == EV_KEY && data->code == BTN_TOUCH) {
			button = SDL_BUTTON_RIGHT;
			state = SDL_PRESSED;
#ifdef DEBUG_MOUSE
			fprintf(stderr,"HID mouse: BTN_TOUCH\n");
#endif
		} 
		else if (data->type == EV_KEY) {
			switch (data->code) {
			case BTN_LEFT:
				button = SDL_BUTTON_LEFT;
#ifdef DEBUG_MOUSE		
				fprintf(stderr,"HID mouse: BTN_LEFT\n");
#endif
				break;
			case BTN_MIDDLE:
				button = SDL_BUTTON_MIDDLE;
#ifdef DEBUG_MOUSE
				fprintf(stderr,"HID mouse: BTN_MIDDLE\n");
#endif
				break;
			case BTN_RIGHT:
				button = SDL_BUTTON_RIGHT;
#ifdef DEBUG_MOUSE
				fprintf(stderr,"HID mouse: BTN_RIGHT\n");
#endif
				break;
			case BTN_WHEEL:
				button = 0;
#ifdef DEBUG_MOUSE
				fprintf(stderr,"HID mouse: BTN_WHEEL\n");
#endif
				break;
			default:
				button = 0;
#ifdef DEBUG_MOUSE
				fprintf(stderr,"HID mouse: Unknown BTN event\n");
#endif
			}

			if (data->value) {
				state = SDL_PRESSED;
			} 
			else {
				state = SDL_RELEASED;
			}
		} 
		else if (data->type == EV_SYN && data->code == SYN_REPORT) {
			posted += SDL_PrivateMouseMotion(0, 1, m_x, m_y);
			if (button > 0) {
				posted += SDL_PrivateMouseButton(state, button, 0, 0);
			}
			
#ifdef DEBUG_MOUSE
			fprintf(stderr,"HID dream mouse: x=%d y=%d button=%d\n",m_x, m_y, button);
#endif
		} 
		else if (data->type == EV_MSC && data->code == MSC_SCAN) {
			// kernel encountered an unmapped key - just ignore it
			continue;
		} 
		else {
			unknown = SDL_TRUE;
		}
		if (unknown) {
			fprintf(stderr,"Warning: unknown mouse event type=%x, code=%x, value=%x", data->type, data->code, data->value);
		}
	}
	return;
}

static void handle_keyboard(_THIS, int k_fd)
{
	if (k_fd < 0) {
		return;
	}
	int x, pressed;
	int caps=0;
	unsigned short code = 0xee;
	struct input_event ev;
	
	SDL_keysym keysym;
	SDLMod mode = KMOD_NONE;
	
	x = read( k_fd, &ev, sizeof(struct input_event) );
	if(x > 0)
	{
		SDL_memcpy(&code,&ev.code,sizeof(ev.code));
		if ((code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT) && ev.value == 1 && ev.type == EV_KEY)
			mode=KMOD_SHIFT;
		if ((code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT) && ev.value == 0 && ev.type == EV_KEY)
			mode=KMOD_NONE;
		if ((code == KEY_LEFTCTRL || code == KEY_RIGHTCTRL) && ev.value == 1 && ev.type == EV_KEY)
			mode=KMOD_CTRL;
		if ((code == KEY_LEFTCTRL || code == KEY_RIGHTCTRL) && ev.value == 0 && ev.type == EV_KEY)
			mode=KMOD_NONE;
		if ((code == KEY_LEFTALT) && ev.value == 1 && ev.type == EV_KEY)
			mode=KMOD_LALT;
		if ((code == KEY_LEFTALT) && ev.value == 0 && ev.type == EV_KEY)
			mode=KMOD_NONE;
		if ((code == KEY_RIGHTALT) && ev.value == 1 && ev.type == EV_KEY)
			mode=KMOD_RALT;
		if ((code == KEY_RIGHTALT) && ev.value == 0 && ev.type == EV_KEY)
			mode=KMOD_NONE;
		if ((code == KEY_CAPSLOCK) && ev.value == 1 && ev.type == EV_KEY)
		{
			if (caps == 1) {caps=0;} else {caps=1;}
		}

		if (mode == KMOD_NONE && caps == 1) {mode=KMOD_RSHIFT;}
		
		if ((code == KEY_TV) && ev.value == 1 && ev.type == EV_KEY) {exit(0);}
		
		if ((ev.value == 0 || ev.value == 1 || ev.value == 2) && ev.type == EV_KEY)
		{
			if (k_fd == kbd) {
				keysym.sym = KeyTranslate(code,1);
#ifdef DEBUG_KEYBOARD
				fprintf(stderr,"HID dreambox ir keyboard\n");
#endif
			}
			else if (k_fd == arc) {
				keysym.sym = KeyTranslate(code,0);
#ifdef DEBUG_KEYBOARD
				fprintf(stderr,"HID dreambox advanced remote control\n");
#endif
			}
			else if (k_fd == rc) {
				keysym.sym = KeyTranslate(code,0);
#ifdef DEBUG_KEYBOARD
				fprintf(stderr,"HID dreambox remote control\n");
#endif
			}
			else {
				keysym.sym = KeyTranslate(code,0);
#ifdef DEBUG_KEYBOARD
				fprintf(stderr,"HID kbd code %d\n",code);
#endif
			}
			if (keysym.sym == SDLK_UNKNOWN) {
#ifdef DEBUG_KEYBOARD
				fprintf(stderr,"SDLKey: SDLK_UNKNOWN\n");
#endif
				return;
			}
			fprintf(stderr,"SDLKey: %d\n", keysym.sym);
			if ( ev.value > 0) {
				pressed = SDL_PRESSED;
			}
			else {
				pressed = SDL_RELEASED;
			}
			
			keysym.scancode = K(KT_ASCII,13);
			keysym.mod = mode;
			keysym.unicode = 0;
			posted += SDL_PrivateKeyboard(pressed, &keysym);
		}
		if (mode == KMOD_RSHIFT && caps == 1) {mode=KMOD_NONE;}
			ev.value=-1;
	}
}

void BCMFB_PumpEvents(_THIS)
{
	fd_set fdset;
	int max_fd, i;
	static struct timeval zero;

	do {
		posted = 0;
		FD_ZERO(&fdset);
		max_fd = 0;
		for( i = 0; i <= kbd_index; i++) {
			if ( kbd_fd[i] >= 0 ) {
				FD_SET(kbd_fd[i], &fdset);
				if ( max_fd < kbd_fd[i] ) {
					max_fd = kbd_fd[i];
				}
			}
		}
		for( i = 0; i <= m_index; i++) {
			if ( mice_fd[i] >= 0 ) {
				FD_SET(mice_fd[i], &fdset);
				if ( max_fd < mice_fd[i] ) {
					max_fd = mice_fd[i];
				}
			}
		}
		if ( select(max_fd+1, &fdset, NULL, NULL, &zero) > 0 ) {
			for( i = 0; i <= kbd_index; i++) {
				if ( kbd_fd[i] >= 0 ) {
					if ( FD_ISSET(kbd_fd[i], &fdset) ) {
						handle_keyboard(this, kbd_fd[i]);
					}
				}
			}
			for( i = 0; i <= m_index; i++) {
				if ( mice_fd[i] >= 0 ) {
					if ( FD_ISSET(mice_fd[i], &fdset) ) {
						handle_mouse(this, mice_fd[i]);
					}
				}
			}
		}
	} while ( posted );
}

static SDLKey KeyTranslate(unsigned short code, int type) {
	
	// dreambox keyboard
	if (type == 1) {
		switch (code) {
			case KEY_F1:		return SDLK_CARET;
			case KEY_F2:		return SDLK_F1;
			case KEY_F3:		return SDLK_F2;
			case KEY_BACK:		return SDLK_F3;
			case KEY_FORWARD:	return SDLK_F4;
			case KEY_F6:		return SDLK_F5;
			case KEY_F7:		return SDLK_F6;
			case KEY_F8:		return SDLK_F7;
			case KEY_RECORD:	return SDLK_F8;
			case KEY_STOP:		return SDLK_F9;
			case KEY_PAUSE:		return SDLK_F10;
			case KEY_PREVIOUSSONG:	return SDLK_F11;
			case KEY_PLAY:		return SDLK_F12;
			case KEY_FASTFORWARD:	return SDLK_SYSREQ;;
			case KEY_NEXTSONG:	return SDLK_PAUSE;
		}
	}
	// default keyboard
	switch (code) {
		case KEY_0:		return SDLK_0;
		case KEY_1:		return SDLK_1;
		case KEY_2:		return SDLK_2;
		case KEY_3:		return SDLK_3;
		case KEY_4:		return SDLK_4;
		case KEY_5:		return SDLK_5;
		case KEY_6:		return SDLK_6;
		case KEY_7:		return SDLK_7;
		case KEY_8:		return SDLK_8;
		case KEY_9:		return SDLK_9;
		case KEY_A:		return SDLK_a;
		case KEY_B:		return SDLK_b;
		case KEY_C:		return SDLK_c;
		case KEY_D:		return SDLK_d;
		case KEY_E:		return SDLK_e;
		case KEY_F:		return SDLK_f;
		case KEY_G:		return SDLK_g;
		case KEY_H:		return SDLK_h;
		case KEY_I:		return SDLK_i;
		case KEY_J:		return SDLK_j;
		case KEY_K:		return SDLK_k;
		case KEY_L:		return SDLK_l;
		case KEY_M:		return SDLK_m;
		case KEY_N:		return SDLK_n;
		case KEY_O:		return SDLK_o;
		case KEY_P:		return SDLK_p;
		case KEY_Q:		return SDLK_q;
		case KEY_R:		return SDLK_r;
		case KEY_S:		return SDLK_s;
		case KEY_T:		return SDLK_t;
		case KEY_U:		return SDLK_u;
		case KEY_V:		return SDLK_v;
		case KEY_W:		return SDLK_w;
		case KEY_X:		return SDLK_x;
		case KEY_Y:		return SDLK_y;
		case KEY_Z:		return SDLK_z;
		case KEY_APOSTROPHE:	return SDLK_QUOTEDBL;
		case KEY_SEMICOLON:	return SDLK_SEMICOLON;
		case KEY_LEFTBRACE:	return SDLK_LEFTBRACKET;

		case KEY_MINUS:		return SDLK_MINUS;
		case KEY_EQUAL:		return SDLK_EQUALS;
		case KEY_SLASH:		return SDLK_SLASH;
		case KEY_BACKSPACE:	return SDLK_BACKSPACE;

		case KEY_TAB:		return SDLK_TAB;
		case KEY_RIGHTBRACE:	return SDLK_RIGHTBRACKET;
		case KEY_BACKSLASH:	return SDLK_BACKSLASH;

		case KEY_OK:
		case KEY_ENTER:		return SDLK_RETURN;

		case KEY_COMMA:		return SDLK_COMMA;
		case KEY_DOT:		return SDLK_PERIOD;

		case KEY_GRAVE:		return SDLK_CARET;

		case KEY_LEFT:		return SDLK_LEFT;
		case KEY_RIGHT:		return SDLK_RIGHT;
		case KEY_UP:		return SDLK_UP;
		case KEY_DOWN:		return SDLK_DOWN;

		case KEY_102ND:		return SDLK_LESS;

		case KEY_F1:		return SDLK_F1;
		case KEY_F2:		return SDLK_F2;
		case KEY_F3:		return SDLK_F3;
		case KEY_F4:		return SDLK_F4;
		case KEY_F5:		return SDLK_F5;
		case KEY_F6:		return SDLK_F6;
		case KEY_F7:		return SDLK_F7;
		case KEY_F8:		return SDLK_F8;
		case KEY_F9:		return SDLK_F9;
		case KEY_F10:		return SDLK_F10;
		case KEY_F11:		return SDLK_F11;
		case KEY_F12:		return SDLK_F12;
		case KEY_F13:		return SDLK_F13;
		case KEY_F14:		return SDLK_F14;
		case KEY_F15:		return SDLK_F15;
		
		// Dream F4, F5, ...
		case KEY_BACK:		return SDLK_F4;
		case KEY_FORWARD:	return SDLK_F5;
		
		case KEY_RECORD:	return SDLK_F9;
		case KEY_PAUSE:		return SDLK_PAUSE;
#if 0
		case KEY_:		return SDLK_ASTERISK;
		case KEY_:		return SDLK_PLUS;
		case KEY_STOP:		return SDLK_UNKNOWN;
		case KEY_PREVIOUSSONG:	return SDLK_UNKNOWN;
		case KEY_REWIND:	return SDLK_UNKNOWN;
		case KEY_PLAY:		return SDLK_UNKNOWN;
		case KEY_FASTFORWARD:	return SDLK_UNKNOWN;
		case KEY_NEXTSONG:	return SDLK_UNKNOWN;
		
		case KEY_MUTE:		return SDLK_UNKNOWN;
		case KEY_VOLUMEUP:	return SDLK_UNKNOWN;
		case KEY_VOLUMEDOWN:	return SDLK_UNKNOWN;
		
		case KEY_RED:		return SDLK_UNKNOWN;
		case KEY_GREEN:		return SDLK_UNKNOWN;
		case KEY_YELLOW:	return SDLK_UNKNOWN;
		case KEY_BLUE:		return SDLK_UNKNOWN;
		
		case KEY_INFO:		return SDLK_UNKNOWN;
		case KEY_MENU:		return SDLK_UNKNOWN;
		case KEY_VIDEO:		return SDLK_UNKNOWN;
		case KEY_AUDIO:		return SDLK_UNKNOWN;
		
		case KEY_COMPOSE:	return SDLK_UNKNOWN;
		case KEY_PROG1:		return SDLK_UNKNOWN;
#endif
		case KEY_CHANNELUP:	return SDLK_PAGEUP;
		case KEY_CHANNELDOWN:	return SDLK_PAGEDOWN;
		
		case KEY_TV:		return SDLK_ESCAPE;
		case KEY_RADIO:		return SDLK_ESCAPE;

		case KEY_SPACE:		return SDLK_SPACE;
		case KEY_DELETE:	return SDLK_DELETE;
		case KEY_ESC:		return SDLK_ESCAPE;
		case KEY_INSERT:	return SDLK_INSERT;
			
		case KEY_HELP:		return SDLK_HELP;
		case KEY_HOME:		return SDLK_HOME;
		case KEY_MENU:		return SDLK_MENU;
		case KEY_EXIT:		return SDLK_ESCAPE;
		case KEY_END:		return SDLK_END;
		
		case KEY_NUMLOCK:	return SDLK_NUMLOCK;
		case KEY_CAPSLOCK:	return SDLK_CAPSLOCK;
		case KEY_SCROLLLOCK:	return SDLK_SCROLLOCK;
		case KEY_RIGHTSHIFT:	return SDLK_RSHIFT;
		case KEY_LEFTSHIFT:	return SDLK_LSHIFT;
		case KEY_RIGHTCTRL:	return SDLK_RCTRL;
		case KEY_LEFTCTRL:	return SDLK_LCTRL;
		case KEY_RIGHTALT:	return SDLK_RALT;
		case KEY_LEFTALT:	return SDLK_LALT;
		case KEY_RIGHTMETA:	return SDLK_RMETA;
		case KEY_LEFTMETA:	return SDLK_LMETA;
// SDLK_PRINT;
		case KEY_SYSRQ:		return SDLK_SYSREQ;
// SDLK_BREAK;
		case KEY_POWER:		return SDLK_POWER;
// SDLK_EURO;
		
		default:		return SDLK_UNKNOWN;
	}
}
