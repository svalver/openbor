/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef SDLPORT_H
#define SDLPORT_H

#include <SDL.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "globals.h"

#if LINUX
#define stricmp  strcasecmp
#define strnicmp strncasecmp
#endif

/*
* SKIP_CODE's only effect is to suppress setting the window title (see the two
* #ifndef SKIP_CODE blocks in sdlport.c). Android has no window title, so it
* genuinely does not apply there. macOS does, and excluding it meant a macOS
* build could never set its title at all - every game was stuck showing
* "OpenBOR", the default in sdl/video.c. That looks like an oversight rather
* than a deliberate platform difference: video_set_window_title() is safe to
* call before the window exists, since it just fills in the buffer that
* SDL_CreateWindow() later reads.
*/
#if ANDROID
#define SKIP_CODE
#endif

#ifdef ANDROID
#define MAXTOUCHB 13
#endif

//#define MEMTEST 1

#if _POSIX_C_SOURCE >= 199309L
void _usleep(u32 usec);
#define usleep _usleep
#endif

void initSDL();
#ifdef ANDROID
char* AndroidRoot(char *relPath);
extern char rootDir[MAX_BUFFER_LEN];
#endif
void borExit(int reset);
void openborMain(int argc, char** argv);

extern char packfile[MAX_FILENAME_LEN];
extern char paksDir[MAX_FILENAME_LEN];
extern char savesDir[MAX_FILENAME_LEN];
extern char logsDir[MAX_FILENAME_LEN];
extern char screenShotsDir[MAX_FILENAME_LEN];

#endif
