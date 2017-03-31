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

#include "SDL_dreambox.h"
#include "SDL_dreambox_gles.h"

#define DREAMBOX_DEBUG

/* Being a null driver, there's no event stream. We just define stubs for
   most of the API. */

/*****************************************************************************/
/* SDL OpenGL/OpenGL ES functions                                            */
/*****************************************************************************/
int
DREAM_GL_LoadLibrary(_THIS, const char *path)
{
	int ret;
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	
	/* Check if OpenGL ES library is specified for GF driver */
	if (path == NULL) {
		path = SDL_getenv("SDL_OPENGL_LIBRARY");
		if (path == NULL) {
			path = SDL_getenv("SDL_OPENGLES_LIBRARY");
		}
	}

	/* Check if default library loading requested */
	if (path == NULL) {
		/* Already linked with GF library which provides egl* subset of	*/
		/* functions, use Common profile of OpenGL ES library by default */
		path = "/usr/lib/libGLESv2.so";
	}
	
	ret = SDL_EGL_LoadLibrary(_this, path, (NativeDisplayType) phdata->egl_display);
	
#ifdef DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: SDL_EGL_LoadLibrary ret=%d\n",ret);
#endif
	
	DREAM_PumpEvents(_this);
	
	return ret;
}

SDL_GLContext
DREAM_GL_CreateContext(_THIS, SDL_Window * window)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;
	EGLBoolean status;
	EGLint configs;
	uint32_t attr_pos;
	EGLint attr_value;
	EGLint cit;
	
#ifdef DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: GL_CreateContext\n");
#endif

	/* Check if EGL was initialized */
	if (phdata->egl_initialized != SDL_TRUE) {
		SDL_SetError("DREAM: EGL initialization failed, no OpenGL ES support");
		return NULL;
	}

	/* Prepare attributes list to pass them to OpenGL ES */
	attr_pos = 0;

	wdata->gles_attributes[attr_pos++] = EGL_SURFACE_TYPE;
	wdata->gles_attributes[attr_pos++] = EGL_KHR_surfaceless_context ? EGL_DONT_CARE : EGL_PBUFFER_BIT;
	wdata->gles_attributes[attr_pos++] = EGL_RED_SIZE;
	wdata->gles_attributes[attr_pos++] = 8;
	wdata->gles_attributes[attr_pos++] = EGL_GREEN_SIZE;
	wdata->gles_attributes[attr_pos++] = 8;
	wdata->gles_attributes[attr_pos++] = EGL_BLUE_SIZE;
	wdata->gles_attributes[attr_pos++] = 8;
	wdata->gles_attributes[attr_pos++] = EGL_ALPHA_SIZE;
	wdata->gles_attributes[attr_pos++] = 8;
	wdata->gles_attributes[attr_pos++] = EGL_DEPTH_SIZE;
	wdata->gles_attributes[attr_pos++] = 24;
	wdata->gles_attributes[attr_pos++] = EGL_BUFFER_SIZE;
	wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;
	wdata->gles_attributes[attr_pos++] = EGL_RENDERABLE_TYPE;
	wdata->gles_attributes[attr_pos++] = EGL_OPENGL_ES2_BIT;
	
	/* Setup stencil bits */
	if (_this->gl_config.stencil_size) {
#ifdef DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: GL_CreateContext found stencil_size: %d\n", _this->gl_config.buffer_size);
#endif
		wdata->gles_attributes[attr_pos++] = EGL_STENCIL_SIZE;
		wdata->gles_attributes[attr_pos++] = _this->gl_config.buffer_size;
	} else {
		wdata->gles_attributes[attr_pos++] = EGL_STENCIL_SIZE;
		wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;
	}

	/* Set number of samples in multisampling */
	if (_this->gl_config.multisamplesamples) {
#ifdef DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: GL_CreateContext found multisamplesamples: %d\n", _this->gl_config.multisamplesamples);
#endif
		wdata->gles_attributes[attr_pos++] = EGL_SAMPLES;
		wdata->gles_attributes[attr_pos++] =
			_this->gl_config.multisamplesamples;
	}

	/* Multisample buffers, OpenGL ES 1.0 spec defines 0 or 1 buffer */
	if (_this->gl_config.multisamplebuffers) {
#ifdef DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: GL_CreateContext found multisamplebuffers: %d\n", _this->gl_config.multisamplebuffers);
#endif
		wdata->gles_attributes[attr_pos++] = EGL_SAMPLE_BUFFERS;
		wdata->gles_attributes[attr_pos++] =
			_this->gl_config.multisamplebuffers;
	}

	/* Finish attributes list */
	wdata->gles_attributes[attr_pos] = EGL_NONE;

	/* Request first suitable framebuffer configuration */
	status = eglChooseConfig(phdata->egl_display, wdata->gles_attributes,
							 wdata->gles_configs, 1, &configs);
	if (status != EGL_TRUE) {
		SDL_SetError("DREAM: Can't find closest configuration for OpenGL ES");
		return NULL;
	}

	/* Check if nothing has been found, try "don't care" settings */
	if (configs == 0) {
		int32_t it;
		int32_t jt;
		GLint depthbits[4] = { 32, 24, 16, EGL_DONT_CARE };

		for (it = 0; it < 4; it++) {
			for (jt = 16; jt >= 0; jt--) {
				/* Don't care about color buffer bits, use what exist */
				/* Replace previous set data with EGL_DONT_CARE		 */
				attr_pos = 0;
				wdata->gles_attributes[attr_pos++] = EGL_SURFACE_TYPE;
				wdata->gles_attributes[attr_pos++] = EGL_WINDOW_BIT;
				wdata->gles_attributes[attr_pos++] = EGL_RED_SIZE;
				wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;
				wdata->gles_attributes[attr_pos++] = EGL_GREEN_SIZE;
				wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;
				wdata->gles_attributes[attr_pos++] = EGL_BLUE_SIZE;
				wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;
				wdata->gles_attributes[attr_pos++] = EGL_ALPHA_SIZE;
				wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;
				wdata->gles_attributes[attr_pos++] = EGL_BUFFER_SIZE;
				wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;

				/* Try to find requested or smallest depth */
				if (_this->gl_config.depth_size) {
					wdata->gles_attributes[attr_pos++] = EGL_DEPTH_SIZE;
					wdata->gles_attributes[attr_pos++] = depthbits[it];
				} else {
					wdata->gles_attributes[attr_pos++] = EGL_DEPTH_SIZE;
					wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;
				}

				if (_this->gl_config.stencil_size) {
					wdata->gles_attributes[attr_pos++] = EGL_STENCIL_SIZE;
					wdata->gles_attributes[attr_pos++] = jt;
				} else {
					wdata->gles_attributes[attr_pos++] = EGL_STENCIL_SIZE;
					wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;
				}

				wdata->gles_attributes[attr_pos++] = EGL_SAMPLES;
				wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;
				wdata->gles_attributes[attr_pos++] = EGL_SAMPLE_BUFFERS;
				wdata->gles_attributes[attr_pos++] = EGL_DONT_CARE;
				wdata->gles_attributes[attr_pos] = EGL_NONE;

				/* Request first suitable framebuffer configuration */
				status =
					eglChooseConfig(phdata->egl_display,
									wdata->gles_attributes,
									wdata->gles_configs, 1, &configs);

				if (status != EGL_TRUE) {
					SDL_SetError
						("DREAM: Can't find closest configuration for OpenGL ES");
					return NULL;
				}
				if (configs != 0) {
					break;
				}
			}
			if (configs != 0) {
				break;
			}
		}

		/* No available configs */
		if (configs == 0) {
			SDL_SetError("DREAM: Can't find any configuration for OpenGL ES");
			return NULL;
		}
	}
#ifdef DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: GL_CreateContext now configs=%d\n", configs);
#endif
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
#ifdef DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: GL_CreateContext now cit=%d\n", cit);
#endif
	wdata->gles_config = cit;

	/* Create OpenGL ES context */
	wdata->gles_context =
		eglCreateContext(phdata->egl_display,
						 wdata->gles_configs[wdata->gles_config], NULL, NULL);
	if (wdata->gles_context == EGL_NO_CONTEXT) {
		SDL_SetError("DREAM: OpenGL ES context creation has been failed");
		return NULL;
	}

	wdata->gles_surface =
		eglCreateWindowSurface(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			(NativeWindowType) 0, NULL);


	if (wdata->gles_surface == 0) {
		SDL_SetError("Error : eglCreateWindowSurface failed;");
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
		SDL_SetError("DREAM: Can't set OpenGL ES context on creation");
		return NULL;
	}

	_this->gl_config.accelerated = 1;

	/* Always clear stereo enable, since OpenGL ES do not supports stereo */
	_this->gl_config.stereo = 0;

	/* Get back samples and samplebuffers configurations. Rest framebuffer */
	/* parameters could be obtained through the OpenGL ES API */
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_SAMPLES, &attr_value);
	if (status == EGL_TRUE) {
#ifdef DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: GL_CreateContext now multisamplesamples=%d\n", attr_value);
#endif
		_this->gl_config.multisamplesamples = attr_value;
	}
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_SAMPLE_BUFFERS, &attr_value);
	if (status == EGL_TRUE) {
#ifdef DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: GL_CreateContext now multisamplebuffers=%d\n", attr_value);
#endif
		_this->gl_config.multisamplebuffers = attr_value;
	}

	/* Get back stencil and depth buffer sizes */
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_DEPTH_SIZE, &attr_value);
	if (status == EGL_TRUE) {
#ifdef DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: GL_CreateContext now depth_size=%d\n", attr_value);
#endif
		_this->gl_config.depth_size = attr_value;
	}
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_STENCIL_SIZE, &attr_value);
	if (status == EGL_TRUE) {
#ifdef DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: GL_CreateContext now stencil_size=%d\n", attr_value);
#endif
		_this->gl_config.stencil_size = attr_value;
	}
	status =
		eglGetConfigAttrib(phdata->egl_display,
			wdata->gles_configs[wdata->gles_config],
			EGL_RENDERABLE_TYPE, &attr_value);
	if (status == EGL_TRUE) {
#ifdef DREAMBOX_DEBUG
		fprintf(stderr, "DREAM: GL_CreateContext now EGL_RENDERABLE_TYPE=0x%04x\n", attr_value);
#endif
	}

	/* Under DREAM OpenGL ES output can't be double buffered */
	_this->gl_config.double_buffer = 0;

	/* GL ES context was successfully created */
	return wdata->gles_context;
}

void
DREAM_GL_DeleteContext(_THIS, SDL_GLContext context)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	EGLBoolean status;
	
#ifdef DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: DeleteContext\n");
#endif
	
	if (phdata->egl_initialized != SDL_TRUE) {
		SDL_SetError("DREAM: GLES initialization failed, no OpenGL ES support");
		return;
	}

	/* Check if OpenGL ES connection has been initialized */
	if (phdata->egl_display != EGL_NO_DISPLAY) {
		if (context != EGL_NO_CONTEXT) {
			status = eglDestroyContext(phdata->egl_display, context);
			if (status != EGL_TRUE) {
				/* Error during OpenGL ES context destroying */
				SDL_SetError("DREAM: OpenGL ES context destroy error");
				return;
			}
		}
	}

	return;
}

int
DREAM_GL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	SDL_WindowData *wdata;
	EGLBoolean status;
	
#ifdef DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: GL_MakeCurrent\n");
#endif
	
	if (phdata->egl_initialized != SDL_TRUE) {
		return SDL_SetError("DREAM: GF initialization failed, no OpenGL ES support");
	}

	if ((window == NULL) && (context == NULL)) {
		status =
			eglMakeCurrent(phdata->egl_display, EGL_NO_SURFACE,
							 EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (status != EGL_TRUE) {
			/* Failed to set current GL ES context */
			return SDL_SetError("DREAM: Can't set OpenGL ES context");
		}
	} else {
		wdata = (SDL_WindowData *) window->driverdata;
		if (wdata->gles_surface == EGL_NO_SURFACE) {
			return SDL_SetError
				("DREAM: OpenGL ES surface is not initialized for this window");
		}
		if (wdata->gles_context == EGL_NO_CONTEXT) {
			return SDL_SetError
				("DREAM: OpenGL ES context is not initialized for this window");
		}
		if (wdata->gles_context != context) {
			return SDL_SetError
				("DREAM: OpenGL ES context is not belong to this window");
		}
		status =
			eglMakeCurrent(phdata->egl_display, wdata->gles_surface,
							 wdata->gles_surface, wdata->gles_context);
		if (status != EGL_TRUE) {
			/* Failed to set current GL ES context */
			return SDL_SetError("DREAM: Can't set OpenGL ES context");
		}
	}
	return 0;
}

int
DREAM_GL_SwapWindow(_THIS, SDL_Window * window)
{
	SDL_VideoData *phdata = (SDL_VideoData *) _this->driverdata;
	SDL_WindowData *wdata = (SDL_WindowData *) window->driverdata;
	
#ifdef DREAMBOX_DEBUG
	fprintf(stderr, "DREAM: GL_SwapWindow\n");
#endif
	
	if (phdata->egl_initialized != SDL_TRUE) {
		return SDL_SetError("DREAM: GLES initialization failed, no OpenGL ES support");
	}

	/* Many applications do not uses glFinish(), so we call it for them */
	glFinish();

	/* Wait until OpenGL ES rendering is completed */
	eglWaitGL();
	
	DREAM_WaitForSync();
	
	eglSwapBuffers(phdata->egl_display, wdata->gles_surface);
	return 0;
}

#endif /* SDL_VIDEO_DRIVER_DREAMBOX */

/* vi: set ts=4 sw=4 expandtab: */
