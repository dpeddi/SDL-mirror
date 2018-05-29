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

*  SDL vuplus backend
*  Copyright (C) 2017 Emanuel Strobel
*/

#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_VUPLUS && SDL_VIDEO_OPENGL_EGL

#include "SDL_vuplus.h"
#include "SDL_vuplus_gles.h"

#ifndef VUPLUS_DEBUG
#define VUPLUS_DEBUG 1
#endif

/* Being a null driver, there's no event stream. We just define stubs for
   most of the API. */

/*****************************************************************************/
/* SDL OpenGL/OpenGL ES functions                                            */
/*****************************************************************************/

void *
VU_EGL_GetProcAddress(_THIS, const char *proc)
{
	void * ext;
	/* Try to get function address */
	ext = (void *) eglGetProcAddress(proc);
	if (!ext)
	{
		SDL_SetError("VU: EGL error in - cannot get proc addr of %s", proc);
		return NULL;
	}
	return ext;
}

int
VU_EGL_LoadLibrary(_THIS, const char *path) {
	
	EGLNativeDisplayType *nativeDisplay=NULL;
	nativeDisplay = (EGLNativeDisplayType)0L;
	
	return SDL_EGL_LoadLibrary(_this, path, nativeDisplay);
}

SDL_GLContext
VU_EGL_CreateContext(_THIS, SDL_Window * window)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;
	EGLBoolean status;
	EGLint configs;
	uint32_t attr_pos;
	EGLint attr_value;
	EGLint cit;
	EGLNativeWindowType *nativeWindow=NULL;
	EGLint major_version = _this->gl_config.major_version;
	EGLint contextAttrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	VUGLES_NativeWindowInfo win_info;
	
#if VUPLUS_DEBUG
	fprintf(stderr, "VU: CreateContext\n");
	fprintf(stderr, "VU: GLES version %d.%d\n", major_version, _this->gl_config.minor_version);
#endif

	/* Check if EGL was initialized */

	if (phdata->egl_initialized != SDL_TRUE) {
		SDL_SetError("VU: EGL initialization failed, no OpenGL ES support");
		return NULL;
	}
	
	/* bind EGL_OPENGL_ES_API */
	if (major_version > 1 )
		_this->egl_data->eglBindAPI(EGL_OPENGL_ES_API);
	else
		_this->egl_data->eglBindAPI(EGL_OPENGL_API);

	/* Prepare attributes list to pass them to OpenGL ES */
	attr_pos = 0;

	wdata->gles_attributes[attr_pos++] = EGL_RED_SIZE;
	wdata->gles_attributes[attr_pos++] = (EGLint) 8;
	wdata->gles_attributes[attr_pos++] = EGL_GREEN_SIZE;
	wdata->gles_attributes[attr_pos++] = (EGLint) 8;
	wdata->gles_attributes[attr_pos++] = EGL_BLUE_SIZE;
	wdata->gles_attributes[attr_pos++] = (EGLint) 8;
	wdata->gles_attributes[attr_pos++] = EGL_ALPHA_SIZE;
	wdata->gles_attributes[attr_pos++] = (EGLint) 8;
	wdata->gles_attributes[attr_pos++] = EGL_DEPTH_SIZE;
	wdata->gles_attributes[attr_pos++] = (EGLint) 16;

	/* Setup buffer size */
	if (_this->gl_config.buffer_size) {
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: gl_config.buffer_size: %d\n", _this->gl_config.buffer_size);
#endif
		wdata->gles_attributes[attr_pos++] = EGL_BUFFER_SIZE;
		wdata->gles_attributes[attr_pos++] = (EGLint) _this->gl_config.buffer_size;
	}
	
	/* Setup stencil bits */
	if (_this->gl_config.stencil_size) {
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: gl_config.stencil_size: %d\n", _this->gl_config.stencil_size);
#endif
		wdata->gles_attributes[attr_pos++] = EGL_STENCIL_SIZE;
		wdata->gles_attributes[attr_pos++] = (EGLint) _this->gl_config.stencil_size;
	}
	
	/* Multisample buffers, OpenGL ES 1.0 spec defines 0 or 1 buffer */
	if (_this->gl_config.multisamplebuffers) {
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: gl_config.multisamplebuffers: %d\n", _this->gl_config.multisamplebuffers);
#endif
		wdata->gles_attributes[attr_pos++] = EGL_SAMPLE_BUFFERS;
		wdata->gles_attributes[attr_pos++] = (EGLint) _this->gl_config.multisamplebuffers;
	}
	
	/* Set number of samples in multisampling */
	if (_this->gl_config.multisamplesamples) {
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: gl_config.multisamplesamples: %d\n", _this->gl_config.multisamplesamples);
#endif
		wdata->gles_attributes[attr_pos++] = EGL_SAMPLES;
		wdata->gles_attributes[attr_pos++] = _this->gl_config.multisamplesamples;
	}
	
	wdata->gles_attributes[attr_pos++] = EGL_SURFACE_TYPE;
	wdata->gles_attributes[attr_pos++] = EGL_WINDOW_BIT;
	wdata->gles_attributes[attr_pos++] = EGL_RENDERABLE_TYPE;
	wdata->gles_attributes[attr_pos++] = EGL_OPENGL_ES2_BIT;

	/* Finish attributes list */
	wdata->gles_attributes[attr_pos] = EGL_NONE;

	/* Request first suitable framebuffer configuration */
	status = eglChooseConfig(phdata->egl_display, wdata->gles_attributes,
					wdata->gles_configs, 1, &configs);
	
	if (status != EGL_TRUE) {
		SDL_SetError("VU: Can't find closest configuration for OpenGL ES");
		return NULL;
	}
	/* Initialize config index */
	wdata->gles_config = 0;

	/* Now check each configuration to find out the best */
	for (cit = 0; cit < configs; cit++) {
		uint32_t stencil_found;
		uint32_t depth_found;

		stencil_found = 0;
		depth_found = 0;

		if (_this->gl_config.stencil_size) {
			status =
				eglGetConfigAttrib(phdata->egl_display,
					wdata->gles_configs[cit], EGL_STENCIL_SIZE,
					&attr_value);
			if (status == EGL_TRUE) {
				if (attr_value != 0) {
					stencil_found = 1;
				}
			}
		} else {
			stencil_found = 1;
		}

		if (_this->gl_config.depth_size) {
			status =
				eglGetConfigAttrib(phdata->egl_display,
					wdata->gles_configs[cit], EGL_DEPTH_SIZE,
					&attr_value);
			if (status == EGL_TRUE) {
				if (attr_value != 0) {
					depth_found = 1;
				}
			}
		} else {
			depth_found = 1;
		}

		/* Exit from loop if found appropriate configuration */
		if ((depth_found != 0) && (stencil_found != 0)) {
			break;
		}
	}

	/* If best could not be found, use first */
	if (cit == configs) {
		cit = 0;
	}

	wdata->gles_config = cit;

	/* Create OpenGL ES context */
	wdata->gles_context = 
		eglCreateContext(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config], 
			NULL, ( major_version > 1 ? contextAttrs : NULL ) );
	
	if (wdata->gles_context == EGL_NO_CONTEXT) {
		SDL_SetError("VU: OpenGL ES context creation has been failed");
		return NULL;
	}

    VUGLES_GetDefaultNativeWindowInfo(&win_info);

    win_info.width = 1280;

    win_info.height = 720;

    win_info.x = 0;

    win_info.y = 0;

	//nativeWindow = (EGLNativeWindowType)0L;
	//nativeWindow = GLES_Native_CreateNativeWindow();
	nativeWindow = VUGLES_CreateNativeWindow(&win_info);

	    win_info.clientID = VUGLES_GetClientID(nativeWindow);

    fprintf(stderr, "%26s ---------------\n", "--------------------------");

    fprintf(stderr, "%26s #%d\n", "dvb_client", win_info.clientID);


	
	wdata->gles_surface = 
		eglCreateWindowSurface(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config], 
			nativeWindow, NULL);

	if (wdata->gles_surface == 0) {
		SDL_SetError("VU: eglCreateWindowSurface failed;");
		return NULL;
	}

	/* Make just created context current */
	status =
		eglMakeCurrent(phdata->egl_display, wdata->gles_surface,
				wdata->gles_surface, wdata->gles_context);

	if (status != EGL_TRUE) {
		/* Destroy OpenGL ES surface */
		eglDestroySurface(phdata->egl_display, wdata->gles_surface);
		eglDestroyContext(phdata->egl_display, wdata->gles_context);
		wdata->gles_context = EGL_NO_CONTEXT;
		SDL_SetError("VU: Can't set OpenGL ES context on creation");
		return NULL;
	}

	/* Always clear stereo enable, since OpenGL ES do not supports stereo */
	_this->gl_config.accelerated = 1;
	
	/* Get back samples and samplebuffers configurations. Rest framebuffer */
	/* parameters could be obtained through the OpenGL ES API */
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_SAMPLES, &attr_value);
		
	if (status == EGL_TRUE) {
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: EGL_SAMPLES=%d\n", attr_value);
#endif
		_this->gl_config.multisamplesamples = attr_value;
	}
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_SAMPLE_BUFFERS, &attr_value);
		
	if (status == EGL_TRUE) {
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: EGL_SAMPLE_BUFFERS=%d\n", attr_value);
#endif
		_this->gl_config.multisamplebuffers = attr_value;
	}

	/* Get back stencil and depth buffer sizes */
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_DEPTH_SIZE, &attr_value);
		
	if (status == EGL_TRUE) {
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: EGL_DEPTH_SIZE=%d\n", attr_value);
#endif
		_this->gl_config.depth_size = attr_value;
	}
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_STENCIL_SIZE, &attr_value);
		
	if (status == EGL_TRUE) {
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: EGL_STENCIL_SIZE=%d\n", attr_value);
#endif
		_this->gl_config.stencil_size = attr_value;
	}
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_MAX_SWAP_INTERVAL, &attr_value);
		
	if (status == EGL_TRUE) {
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: EGL_MAX_SWAP_INTERVAL=%d\n", attr_value);
#endif
	}
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_MIN_SWAP_INTERVAL, &attr_value);
		
	if (status == EGL_TRUE) {
		
#if VUPLUS_DEBUG
		fprintf(stderr, "VU: EGL_MIN_SWAP_INTERVAL=%d\nVU:\n", attr_value);
#endif
	}
#if VUPLUS_DEBUG
	fprintf(stderr, "VU: Vendor             : %s\n", glGetString(GL_VENDOR));
	fprintf(stderr, "VU: Renderer           : %s\n", glGetString(GL_RENDERER));
	fprintf(stderr, "VU: Version            : %s\n", glGetString(GL_VERSION));
	if (major_version > 1 )
		fprintf(stderr, "VU: Shading Version    : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	fprintf(stderr, "VU: Extensions         : %s\n", glGetString(GL_EXTENSIONS));
#endif
	VU_EGL_SetSwapInterval(_this, 1);
	/* GL ES context was successfully created */
	return wdata->gles_context;
}

void
VU_EGL_DeleteContext(_THIS, SDL_GLContext context)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	EGLBoolean status;
	
#if VUPLUS_DEBUG
	fprintf(stderr, "VU: DeleteContext\n");
#endif
	
	if (phdata->egl_initialized != SDL_TRUE) {
		SDL_SetError("VU: GLES initialization failed, no OpenGL ES support");
		return;
	}

	/* Check if OpenGL ES connection has been initialized */
	if (phdata->egl_display != EGL_NO_DISPLAY) {
		if (context != EGL_NO_CONTEXT) {
			status = eglDestroyContext(phdata->egl_display, context);
			if (status != EGL_TRUE) {
				/* Error during OpenGL ES context destroying */
				SDL_SetError("VU: OpenGL ES context destroy error");
				return;
			}
		}
	}

	return;
}

int
VU_EGL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	SDL_WindowData *wdata;
	EGLBoolean status;
	
#if VUPLUS_DEBUG
	fprintf(stderr, "VU: GL_MakeCurrent\n");
#endif
	
	if (phdata->egl_initialized != SDL_TRUE) {
		return SDL_SetError("VU: GF initialization failed, no OpenGL ES support");
	}

	if ((window == NULL) && (context == NULL)) {
		status =
			eglMakeCurrent(phdata->egl_display, EGL_NO_SURFACE,
							 EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (status != EGL_TRUE) {
			/* Failed to set current GL ES context */
			return SDL_SetError("VU: Can't set OpenGL ES context");
		}
	} else {
		wdata = (SDL_WindowData *) window->driverdata;
		if (wdata->gles_surface == EGL_NO_SURFACE) {
			return SDL_SetError
				("VU: OpenGL ES surface is not initialized for this window");
		}
		if (wdata->gles_context == EGL_NO_CONTEXT) {
			return SDL_SetError
				("VU: OpenGL ES context is not initialized for this window");
		}
		if (wdata->gles_context != context) {
			return SDL_SetError
				("VU: OpenGL ES context is not belong to this window");
		}
		status =
			eglMakeCurrent(phdata->egl_display, wdata->gles_surface,
							 wdata->gles_surface, wdata->gles_context);
		if (status != EGL_TRUE) {
			/* Failed to set current GL ES context */
			return SDL_SetError("VU: Can't set OpenGL ES context");
		}
	}
	
	return 0;
}

int
VU_EGL_SetSwapInterval(_THIS, int interval)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	EGLBoolean status;

	if (phdata->egl_initialized != SDL_TRUE) {
		return SDL_SetError("PND: EGL initialization failed, no OpenGL ES support");
	}

	/* Check if OpenGL ES connection has been initialized */
	if (phdata->egl_display != EGL_NO_DISPLAY) {
		/* Set swap OpenGL ES interval */
		status = eglSwapInterval(phdata->egl_display, interval);
		if (status == EGL_TRUE) {
			/* Return success to upper level */
#if VUPLUS_DEBUG
			fprintf(stderr, "VU: SetSwapInterval %d\n", interval);
#endif
			phdata->egl_swapinterval = interval;

			return 0;
		}
	}

	/* Failed to set swap interval */
	return SDL_SetError("PND: Cannot set swap interval");
}

int
VU_EGL_GetSwapInterval(_THIS)
{
	return ((SDL_VideoData *) _this->driverdata)->egl_swapinterval;
}

int
VU_EGL_SwapWindow(_THIS, SDL_Window * window)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;
	EGLBoolean status;
	
	if (phdata->egl_initialized != SDL_TRUE) {
		return SDL_SetError("VU: GLES initialization failed, no OpenGL ES support");
	}

	/* Many applications do not uses glFinish(), so we call it for them */
	glFinish();

	/* Wait until OpenGL ES rendering is completed */
	eglWaitGL();
	
	/* FIXME: we need nativ Wait for Vsync */
	vuplus_wait_for_sync();
	
	eglSwapBuffers(phdata->egl_display, wdata->gles_surface);
	
	return 0;
}

#endif /* SDL_VIDEO_DRIVER_VUPLUS */

/* vi: set ts=4 sw=4 expandtab: */
