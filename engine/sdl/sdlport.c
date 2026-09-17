/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#include "sdlport.h"
#include "packfile.h"
#include "ram.h"
#include "video.h"
#include "menu.h"
#include <time.h>
#include <unistd.h>

#undef usleep

#ifdef DARWIN
#include <CoreFoundation/CoreFoundation.h>
#elif WIN
#undef main
#endif

char packfile[MAX_FILENAME_LEN] = {"bor.pak"};
#if ANDROID
#include <unistd.h>
char rootDir[MAX_BUFFER_LEN] = {""};
#endif
char paksDir[MAX_FILENAME_LEN] = {"Paks"};
char savesDir[MAX_FILENAME_LEN] = {"Saves"};
char logsDir[MAX_FILENAME_LEN] = {"Logs"};
char screenShotsDir[MAX_FILENAME_LEN] = {"ScreenShots"};

// sleeps for the given number of microseconds
#if _POSIX_C_SOURCE >= 199309L
void _usleep(u32 usec)
{
    struct timespec sleeptime;
    sleeptime.tv_sec = usec / 1000000LL;
    sleeptime.tv_nsec = (usec % 1000000LL) * 1000;
    nanosleep(&sleeptime, NULL);
}
#endif

#if ANDROID
char* AndroidRoot(char *relPath)
{
	static char filename[MAX_FILENAME_LEN];
	strcpy(filename, rootDir);
	strcat(filename, relPath);
	return filename;
}
#endif

void borExit(int reset)
{
	/*
	* Saving Private Pla: was SDL_Delay(1000) - a full second of dead time on
	* every quit, with the window already unresponsive. Nothing depends on it:
	* SDL_Quit() tears down its own subsystems, and the audio thread is stopped
	* by the shutdown path before we get here.
	*/
	SDL_Quit(); // call this instead of atexit(SDL_Quit); It's best practice!
    exit(reset);
}

int main(int argc, char *argv[])
{
#ifndef SKIP_CODE
	char pakname[MAX_FILENAME_LEN] = {""};
#endif
#ifdef CUSTOM_SIGNAL_HANDLER
	struct sigaction sigact;
#endif

#ifdef DARWIN
	char resourcePath[PATH_MAX] = {""};
	CFBundleRef mainBundle;
	CFURLRef resourcesDirectoryURL;
	mainBundle = CFBundleGetMainBundle();
	resourcesDirectoryURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	if(!CFURLGetFileSystemRepresentation(resourcesDirectoryURL, true, (UInt8 *) resourcePath, PATH_MAX))
	{
		borExit(0);
	}
	CFRelease(resourcesDirectoryURL);
	chdir(resourcePath);
#endif

#ifdef CUSTOM_SIGNAL_HANDLER
	sigact.sa_sigaction = handleFatalSignal;
	sigact.sa_flags = SA_RESTART | SA_SIGINFO;

	if(sigaction(SIGSEGV, &sigact, NULL) != 0)
	{
		printf("Error setting signal handler for %d (%s)\n", SIGSEGV, strsignal(SIGSEGV));
		borExit(EXIT_FAILURE);
	}
#endif

	setSystemRam();
	initSDL();

	packfile_mode(0);

#ifdef ANDROID

        strcpy(rootDir, SDL_AndroidGetExternalStoragePath());
        strcat(rootDir, "/");
        strcpy(paksDir, SDL_AndroidGetExternalStoragePath());
        strcat(paksDir, "/Paks");
        strcpy(savesDir, SDL_AndroidGetExternalStoragePath());
        strcat(savesDir, "/Saves");
        strcpy(logsDir, SDL_AndroidGetExternalStoragePath());
        strcat(logsDir, "/Logs");
        strcpy(screenShotsDir, SDL_AndroidGetExternalStoragePath());
        strcat(screenShotsDir, "/ScreenShots");
        
	dirExists(rootDir, 1);
    chdir(rootDir);
#endif

	dirExists(paksDir, 1);
	dirExists(savesDir, 1);
	dirExists(logsDir, 1);
	dirExists(screenShotsDir, 1);

   // Test command line argument to launch MOD
   int romArg = 0;
   /*
   * Saving Private Pla: upstream tests `argc == 2`, which means the engine
   * cannot accept ANY argument beside the module - passing one silently drops
   * you into the module browser instead of the game, with nothing in the log
   * to say why. Take the first argument that is an existing file and let flags
   * sit beside it.
   */
   {
      int a;
      for(a = 1; a < argc && !romArg; a++) {
         if(argv[a][0] == '-') {
            continue;   /* a flag, not a module */
         }
         if(fileExists(argv[a])) {
            memset(packfile, 0, sizeof(packfile));
            memcpy(packfile, argv[a], strlen(argv[a]));
            romArg = 1;
         }
      }
   }

   /*
   * Saving Private Pla: which displays to spread the picture over.
   *
   * Deliberately the same spelling as the MAME multi-screen launch scripts
   * this was modelled on, so one set of display numbers and one habit works
   * for both:
   *
   *     -numscreens 2 -screen0 2 -screen1 1
   *
   * The value is an SDL display index and may be written "2" or "screen2";
   * tools/list_displays prints them, and they change when a monitor is
   * plugged or unplugged.
   */
   {
      int a;
      for(a = 1; a < argc; a++) {
         if(!strcmp(argv[a], "-numscreens") && a + 1 < argc) {
            spp_screen_count = atoi(argv[++a]);
            if(spp_screen_count < 1) spp_screen_count = 1;
            if(spp_screen_count > SPP_MAX_SCREENS) spp_screen_count = SPP_MAX_SCREENS;
         } else if(!strncmp(argv[a], "-screen", 7) && argv[a][7] >= '0' && argv[a][7] <= '9'
                   && a + 1 < argc) {
            int which = argv[a][7] - '0';
            const char *val = argv[++a];
            if(!strncmp(val, "screen", 6)) val += 6;
            if(which >= 0 && which < SPP_MAX_SCREENS) spp_screen_display[which] = atoi(val);
         }
      }
      if(spp_screen_count > 1) {
         printf("Screens: %d", spp_screen_count);
         for(a = 0; a < spp_screen_count; a++) printf("  screen%d -> display %d", a, spp_screen_display[a]);
         printf("\n");
      }
   }

   if(!romArg) {
       Menu();
   }

#ifndef SKIP_CODE
	getPakName(pakname, -1);
	video_set_window_title(pakname);
#endif
	openborMain(argc, argv);
	borExit(0);
	return 0;
}

