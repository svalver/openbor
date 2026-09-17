/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */
#if ANDROID

// CRxTRDude - changed the directory for neatness.
#include "android/app/jni/openbor/video.c"

#else

#include "sdlport.h"
#include <math.h>
#include "types.h"
#include "video.h"
#include "vga.h"
#include "screen.h"
#include "opengl.h"
#include "savedata.h"
#include "gfxtypes.h"
#include "gfx.h"
#include "pngdec.h"
#include "videocommon.h"
#include "timer.h"
#include "../resources/OpenBOR_Icon_32x32_png.h"

SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

/*
* Saving Private Pla: one game image across several displays.
*
* A two-screen cabinet (Darius, Warrior Blade, the X-Men six-player board)
* is not two games - it is one wide picture cut in half, which is exactly
* what this module already renders: the widescreen layout draws 1280x480
* into a single surface. So nothing about the game changes. Only the
* PRESENT changes: each display gets its own window showing its own slice
* of the same texture.
*
* Screen 0 keeps the original `window`/`renderer`/`texture` globals,
* because the OpenGL path in opengl.c uses `window` directly and is left
* alone. Multi-screen forces the SDL renderer path (see video_set_mode).
*
* Configured from the command line, using the same vocabulary as the MAME
* multi-screen scripts this was modelled on:
*
*     -numscreens 2 -screen0 2 -screen1 1
*
* where the numbers are SDL display indices - `tools/list_displays` prints
* them, and they change when you plug or unplug a monitor.
*/
int spp_screen_count = 1;
int spp_screen_display[SPP_MAX_SCREENS] = { -1, -1, -1, -1 };

static SDL_Window   *spp_window[SPP_MAX_SCREENS]   = { NULL };
static SDL_Renderer *spp_renderer[SPP_MAX_SCREENS] = { NULL };
static SDL_Texture  *spp_texture[SPP_MAX_SCREENS]  = { NULL };
static SDL_Rect      spp_src[SPP_MAX_SCREENS];

/* Put a window on its display and fill it. */
static void spp_place(SDL_Window *w, int display)
{
	if(!w) return;
	if(display >= 0 && display < SDL_GetNumVideoDisplays())
	{
		SDL_SetWindowPosition(w,
			SDL_WINDOWPOS_CENTERED_DISPLAY(display),
			SDL_WINDOWPOS_CENTERED_DISPLAY(display));
	}
	if(savedata.fullscreen) SDL_SetWindowFullscreen(w, SDL_WINDOW_FULLSCREEN_DESKTOP);
}

/* Tear down the extra screens; screen 0 is the engine's own. */
static void spp_release_extra_screens(void)
{
	int i;
	for(i = 1; i < SPP_MAX_SCREENS; i++)
	{
		if(spp_texture[i])  { SDL_DestroyTexture(spp_texture[i]);   spp_texture[i]  = NULL; }
		if(spp_renderer[i]) { SDL_DestroyRenderer(spp_renderer[i]); spp_renderer[i] = NULL; }
		if(spp_window[i])   { SDL_DestroyWindow(spp_window[i]);     spp_window[i]   = NULL; }
	}
}
s_videomodes stored_videomodes;
yuv_video_mode stored_yuv_mode;
int yuv_mode = 0;
char windowTitle[MAX_LABEL_LEN] = {"OpenBOR"};
int stretch = 0;
int opengl = 0; // OpenGL backend currently in use?
int nativeWidth, nativeHeight; // monitor resolution used in fullscreen mode
int brightness = 0;

void initSDL()
{
	SDL_DisplayMode video_info;
	int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC;

	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

	if(SDL_Init(init_flags) < 0)
	{
		printf("SDL Failed to Init!!!! (%s)\n", SDL_GetError());
		borExit(0);
	}
	/*
	* Saving Private Pla: stock OpenBOR hides the mouse pointer unconditionally.
	* That makes sense for a fullscreen cabinet, but in a window it means the
	* pointer vanishes whenever it crosses the game and you cannot find it
	* again. Hide it only in fullscreen. (Project preference, not a bug fix.)
	*/
	SDL_ShowCursor(savedata.fullscreen ? SDL_DISABLE : SDL_ENABLE);

#ifdef LOADGL
	if(SDL_GL_LoadLibrary(NULL) < 0)
	{
		printf("Warning: couldn't load OpenGL library (%s)\n", SDL_GetError());
	}
#endif

	SDL_GetCurrentDisplayMode(0, &video_info);
	nativeWidth = video_info.w;
	nativeHeight = video_info.h;
	printf("debug:nativeWidth, nativeHeight, bpp, Hz  %d, %d, %d, %d\n", nativeWidth, nativeHeight, SDL_BITSPERPIXEL(video_info.format), video_info.refresh_rate);
}

void video_set_window_title(const char* title)
{
	if(window) SDL_SetWindowTitle(window, title);
	strncpy(windowTitle, title, sizeof(windowTitle)-1);
}

static unsigned pixelformats[4] = {SDL_PIXELFORMAT_INDEX8, SDL_PIXELFORMAT_BGR565, SDL_PIXELFORMAT_BGR888, SDL_PIXELFORMAT_ABGR8888};

int SetVideoMode(int w, int h, int bpp, bool gl)
{
	int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS;
	static bool last_gl = false;
	static int last_x = SDL_WINDOWPOS_UNDEFINED;
	static int last_y = SDL_WINDOWPOS_UNDEFINED;

	if(savedata.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	/*
	* Saving Private Pla: with several screens each window shows one slice of
	* the picture, so each window is one slice WIDE. Sizing them to the whole
	* image instead stretches a 640x480 half across a 1280x480 window - the
	* game looks right and is twice as wide as it should be, which is the
	* sort of wrong that takes a while to name.
	*/
	if(spp_screen_count > 1) w /= spp_screen_count;

	if(!(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP))
		SDL_GetWindowPosition(window, &last_x, &last_y);

	if (gl)
	{
		// The SDL video backend doesn't support high-quality (sharp bilinear) scaling,
		// so it will produce bad results if it tries to scale by a fractional factor.
		// The results are worse than what we get from upscaling with nearest-neighbor
		// and letting the windowing system upscale the result. So only use the high-DPI
		// flag with the OpenGL backend. Unfortunately, we're stuck with it on Windows,
		// where high-DPI support is controlled by a global hint and the ALLOW_HIGHDPI
		// flag is ignored.
		flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
	}

	if(window && gl != last_gl)
	{
		SDL_DestroyWindow(window);
		window = NULL;
	}
	last_gl = gl;

	spp_release_extra_screens();
	if(renderer) SDL_DestroyRenderer(renderer);
	if(texture)  SDL_DestroyTexture(texture);
	renderer = NULL;
	texture = NULL;

	if(window)
	{
		if(savedata.fullscreen)
		{
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else
		{
#ifndef WIN // hiding and showing the window is problematic on Windows
			if(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP)
				SDL_HideWindow(window);
#endif
			SDL_SetWindowFullscreen(window, 0);
			SDL_SetWindowSize(window, w, h);
			SDL_SetWindowPosition(window, last_x, last_y);
			SDL_ShowWindow(window);
		}
	}
	else
	{
		window = SDL_CreateWindow(windowTitle, last_x, last_y, w, h, flags);
		if(!window)
		{
			printf("Error: failed to create window: %s\n", SDL_GetError());
			return 0;
		}
		
		// Kratus (11-2022) Disabled the native OpenBOR icon
		// SDL_Surface* icon = (SDL_Surface*)pngToSurface((void*)openbor_icon_32x32_png.data);
		// SDL_SetWindowIcon(window, icon);
		// SDL_FreeSurface(icon);
		if(!savedata.fullscreen) SDL_GetWindowPosition(window, &last_x, &last_y);
	}

	if(!gl)
	{
		renderer = SDL_CreateRenderer(window, -1, savedata.fpslimit == 1 ? SDL_RENDERER_PRESENTVSYNC : 0);
		if(!renderer)
		{
			printf("Error: failed to create renderer: %s\n", SDL_GetError());
			return 0;
		}

		/*
		* Saving Private Pla: the other displays.
		*
		* Screen 0 is the window above; screens 1..n-1 get their own. Only
		* screen 0's renderer syncs to vblank - two windows both waiting for
		* their own vblank halves the frame rate on displays that are not in
		* phase, which is the first thing that goes wrong with two monitors.
		*/
		spp_window[0]   = window;
		spp_renderer[0] = renderer;
		spp_place(window, spp_screen_display[0]);

		{
			int i;
			for(i = 1; i < spp_screen_count; i++)
			{
				spp_window[i] = SDL_CreateWindow(windowTitle,
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, flags);
				if(!spp_window[i])
				{
					printf("Error: failed to create window for screen %d: %s\n", i, SDL_GetError());
					spp_screen_count = i;
					break;
				}
				spp_renderer[i] = SDL_CreateRenderer(spp_window[i], -1, 0);
				if(!spp_renderer[i])
				{
					printf("Error: failed to create renderer for screen %d: %s\n", i, SDL_GetError());
					SDL_DestroyWindow(spp_window[i]);
					spp_window[i] = NULL;
					spp_screen_count = i;
					break;
				}
				spp_place(spp_window[i], spp_screen_display[i]);
			}
		}
	}

	return 1;
}

int video_set_mode(s_videomodes videomodes)
{
	stored_videomodes = videomodes;
	yuv_mode = 0;

	if(videomodes.hRes==0 && videomodes.vRes==0)
	{
		return 0;
	}

	videomodes = setupPreBlitProcessing(videomodes);

	// 8-bit color should be transparently converted to 32-bit
	assert(videomodes.pixel == 2 || videomodes.pixel == 4);

	/*
	* Saving Private Pla: OpenGL is single-window here. opengl.c keeps one
	* context bound to `window`, and giving every display its own context is
	* a much bigger change than splitting a texture. So asking for more than
	* one screen selects the SDL renderer path, which gets multi-window
	* almost for free. Said out loud because it is a quality trade, not an
	* implementation detail: if the scaling looks worse on a real monitor,
	* the answer is a context per window, not a shrug.
	*/
	if(spp_screen_count > 1 && savedata.usegl)
	{
		printf("Multi-screen (%d): using the SDL renderer, not OpenGL.\n", spp_screen_count);
	}

	// try OpenGL initialization first
	if(spp_screen_count <= 1 && savedata.usegl && video_gl_set_mode(videomodes)) return 1;
	else opengl = 0;

	if(!SetVideoMode(videomodes.hRes * videomodes.hScale,
	                 videomodes.vRes * videomodes.vScale,
	                 videomodes.pixel * 8, false))
	{
		return 0;
	}

	if(savedata.hwfilter ||
	   (videomodes.hScale == 1 && videomodes.vScale == 1 && !savedata.fullscreen))
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	else
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	texture = SDL_CreateTexture(renderer,
	                            pixelformats[videomodes.pixel-1],
	                            SDL_TEXTUREACCESS_STREAMING,
	                            videomodes.hRes, videomodes.vRes);

	/*
	* Saving Private Pla: give every screen a texture of its own (a texture
	* belongs to one renderer) and work out which slice of the picture it
	* shows. Two screens cut a 1280x480 widescreen level into two 640x480
	* halves - the same split a two-CRT cabinet has, with the seam down the
	* middle.
	*/
	{
		int i, slice = videomodes.hRes / (spp_screen_count > 0 ? spp_screen_count : 1);

		spp_texture[0] = texture;
		for(i = 0; i < spp_screen_count; i++)
		{
			spp_src[i].x = i * slice;
			spp_src[i].y = 0;
			spp_src[i].w = slice;
			spp_src[i].h = videomodes.vRes;

			if(i > 0 && spp_renderer[i])
			{
				spp_texture[i] = SDL_CreateTexture(spp_renderer[i],
				                                   pixelformats[videomodes.pixel-1],
				                                   SDL_TEXTUREACCESS_STREAMING,
				                                   videomodes.hRes, videomodes.vRes);
				SDL_SetRenderDrawBlendMode(spp_renderer[i], SDL_BLENDMODE_BLEND);
			}
		}
	}

	/*
	* Saving Private Pla: stock OpenBOR hides the mouse pointer unconditionally.
	* That makes sense for a fullscreen cabinet, but in a window it means the
	* pointer vanishes whenever it crosses the game and you cannot find it
	* again. Hide it only in fullscreen. (Project preference, not a bug fix.)
	*/
	SDL_ShowCursor(savedata.fullscreen ? SDL_DISABLE : SDL_ENABLE);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	video_stretch(savedata.stretch);

	return 1;
}

int video_restore_mode(void)
{
	return video_set_mode(stored_videomodes);
}

void video_fullscreen_flip()
{
	int restore_yuv = yuv_mode;
	savedata.fullscreen ^= 1;
	if(window) video_set_mode(stored_videomodes);
	if(restore_yuv) video_setup_yuv_overlay(&stored_yuv_mode);
}

void blit()
{
	/*
	* Saving Private Pla: present every screen.
	*
	* The single-screen case is what it always was - the whole texture into
	* the whole window, so `NULL` for both rects. With more than one screen
	* each window takes its own slice instead, and the brightness overlay is
	* applied per window so a fade covers the whole cabinet rather than half
	* of it.
	*/
	int i;

	for(i = 0; i < spp_screen_count; i++)
	{
		SDL_Renderer *r = spp_renderer[i];
		SDL_Texture  *t = spp_texture[i];
		SDL_Rect     *src = (spp_screen_count > 1) ? &spp_src[i] : NULL;

		if(!r || !t) continue;

		SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
		SDL_RenderClear(r);
		SDL_RenderCopy(r, t, src, NULL);

		if (brightness > 0)
			SDL_SetRenderDrawColor(r, 255, 255, 255, brightness-1);
		else if (brightness < 0)
			SDL_SetRenderDrawColor(r, 0, 0, 0, (-brightness)-1);
		SDL_RenderFillRect(r, NULL);

		SDL_RenderPresent(r);
	}
}

void FramerateDelay()
{
    static u64 last_time = 0;

    if (savedata.fpslimit < 2 || savedata.fpslimit > 3) return;
    int fps_limit = savedata.fpslimit == 3 ? 500 : 200;

    u64 target_time = last_time + 1000000/fps_limit;
    u64 current_time = timer_uticks();
    while (current_time < target_time)
    {
        usleep(target_time - current_time);
        current_time = timer_uticks();
    }
    last_time = current_time;
}

int video_copy_screen(s_screen* src)
{
	// do any needed scaling and color conversion
	s_videosurface *surface = getVideoSurface(src);

	if(opengl) return video_gl_copy_screen(surface);

	/*
	* Every screen's texture holds the WHOLE picture; the slice is chosen at
	* copy time, not upload time. Uploading the full image to each is a few
	* hundred KB a frame per extra screen, which is nothing next to the
	* alternative of tracking sub-rectangle uploads.
	*/
	{
		int i;
		for(i = 0; i < spp_screen_count; i++)
			if(spp_texture[i])
				SDL_UpdateTexture(spp_texture[i], NULL, surface->data, surface->pitch);
	}
	blit();

	if (savedata.fpslimit >= 2) FramerateDelay();

	return 1;
}

void video_clearscreen()
{
	if(opengl) { video_gl_clearscreen(); return; }

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

void video_stretch(int enable)
{
	stretch = enable;
	if(window && !opengl)
	{
		if(stretch)
			SDL_RenderSetLogicalSize(renderer, 0, 0);
		else
			SDL_RenderSetLogicalSize(renderer, stored_videomodes.hRes, stored_videomodes.vRes);
	}
}

void video_set_color_correction(int gm, int br)
{
	brightness = br;
	if(opengl) video_gl_set_color_correction(gm, br);
}

int video_setup_yuv_overlay(const yuv_video_mode *mode)
{
	stored_yuv_mode = *mode;
	yuv_mode = 1;
	if(opengl) return video_gl_setup_yuv_overlay(mode);

	SDL_DestroyTexture(texture);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	texture = SDL_CreateTexture(renderer,
	                            SDL_PIXELFORMAT_YV12,
	                            SDL_TEXTUREACCESS_STREAMING,
	                            mode->width, mode->height);
	if(!stretch)
		SDL_RenderSetLogicalSize(renderer, mode->display_width, mode->display_height);
	return texture ? 1 : 0;
}

int video_prepare_yuv_frame(yuv_frame *src)
{
	if(opengl) return video_gl_prepare_yuv_frame(src);

	SDL_UpdateYUVTexture(texture, NULL, src->lum, stored_yuv_mode.width,
	        src->cr, stored_yuv_mode.width/2, src->cb, stored_yuv_mode.width/2);
	return 1;
}

int video_display_yuv_frame(void)
{
	if(opengl) return video_gl_display_yuv_frame();

	blit();
	return 1;
}

int video_current_refresh_rate()
{
    SDL_DisplayMode display_mode;
    if (SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &display_mode) != 0)
        return 60;
    return display_mode.refresh_rate;
}

void vga_vwait(void)
{
	static int prevtick = 0;
	int now = SDL_GetTicks();
	int wait = 1000/60 - (now - prevtick);
	if (wait>0)
	{
		SDL_Delay(wait);
	}
	else SDL_Delay(1);
	prevtick = now;

	/*
	* Saving Private Pla: service the platform event queue.
	*
	* Several engine loops - fade_out() and fade_in() above all - spin on
	* vga_vwait() for dozens of frames without ever calling the input update,
	* so nothing polls SDL events for a second or more at a time. macOS treats
	* an application that stops servicing its event queue as hung and puts up
	* the spinning beachball, which is exactly what happened on the transition
	* out of a finished stage (two fades back to back).
	*
	* SDL_PumpEvents() is the right call here rather than SDL_PollEvent():
	* it moves events from the OS into SDL's queue and tells the window server
	* the process is alive, but it does NOT dequeue anything, so input still
	* arrives intact at whatever reads it next.
	*/
	SDL_PumpEvents();
}

#endif
