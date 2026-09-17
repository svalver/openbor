/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef	CONTROL_H
#define	CONTROL_H

// Generic control stuff (keyboard+joystick).
#include <SDL_keycode.h>
#include "joysticks.h"

#define	CONTROL_ESC                 SDL_SCANCODE_ESCAPE
#define	CONTROL_DEFAULT1_START		SDL_SCANCODE_RETURN
#define	CONTROL_DEFAULT1_UP         SDL_SCANCODE_UP
#define	CONTROL_DEFAULT1_DOWN		SDL_SCANCODE_DOWN
#define	CONTROL_DEFAULT1_LEFT		SDL_SCANCODE_LEFT
#define	CONTROL_DEFAULT1_RIGHT		SDL_SCANCODE_RIGHT
#define	CONTROL_DEFAULT1_FIRE1		SDL_SCANCODE_A
/*
* Saving Private Pla: FIRE2 and FIRE5 are swapped from stock (S and D). Stock
* maps A=attack, S=attack2, D=jump, which puts jump two keys away from fire on
* a game whose primary verb is the trigger. The arcade layout is the two
* actions side by side, so here A=fire and S=jump.
*
* This matters more than it would upstream: our title menu has no Options
* screen, so these defaults are the only bindings a player gets.
* (Project preference, not a bug fix.)
*/
#define	CONTROL_DEFAULT1_FIRE2		SDL_SCANCODE_D
#define	CONTROL_DEFAULT1_FIRE3		SDL_SCANCODE_Z
#define	CONTROL_DEFAULT1_FIRE4		SDL_SCANCODE_X
#define	CONTROL_DEFAULT1_FIRE5		SDL_SCANCODE_S
#define	CONTROL_DEFAULT1_FIRE6		SDL_SCANCODE_F
#define	CONTROL_DEFAULT1_SCREENSHOT	SDL_SCANCODE_F12
#define	CONTROL_DEFAULT1_ESC        SDL_SCANCODE_ESCAPE

/*
* Saving Private Pla: game controller layout, measured against a real pad.
*
* Player 1 uses joystick port 0, player 2 port 1. The keyboard still works for
* player 1: control_update() falls back to default_control (the untouched
* CONTROL_DEFAULT1_* keyboard set) whenever these produce nothing.
*
* THE INDICES ARE DEVICE-SPECIFIC. PC_GetJoystickKeyName() lays a port out as
*
*     1 .. NumButtons                     buttons 0..n-1
*     next 2 * NumAxes                    each axis, negative then positive
*     next 4 * NumHats                    each hat: up, right, down, left
*
* so "d-pad up" is not a constant - it moves with the device's button and axis
* counts. These values are for an Xbox One S pad, which the engine reports as
* 11 buttons, 6 axes, 1 hat, giving:
*
*     1..11    A B X Y LB RB View Menu LS RS Guide
*     12..23   axes; left stick is 12 X- 13 X+ 14 Y- 15 Y+
*     24..27   d-pad up, right, down, left
*
* Stock OpenBOR used 1-4 as the directions, which suits an arcade stick or a
* handheld whose d-pad is wired as four buttons - but on a gamepad that is
* A/B/X/Y, so "up" fired the gun. That was the mis-mapping.
*
* Movement is on the D-PAD, not the left stick: digital, no deadzone, and it is
* what the cabinet will actually have. To use the left stick instead, swap the
* four direction values for 14 (up), 13 (right), 15 (down), 12 (left).
*/
#define	CONTROL_PAD1_UP			(JOY_LIST_FIRST + 24)
#define	CONTROL_PAD1_RIGHT		(JOY_LIST_FIRST + 25)
#define	CONTROL_PAD1_DOWN		(JOY_LIST_FIRST + 26)
#define	CONTROL_PAD1_LEFT		(JOY_LIST_FIRST + 27)
#define CONTROL_PAD1_FIRE1		(JOY_LIST_FIRST + 1)    /* A     - fire      */
#define CONTROL_PAD1_FIRE2		(JOY_LIST_FIRST + 3)    /* X     - attack 2  */
#define	CONTROL_PAD1_FIRE3		(JOY_LIST_FIRST + 4)    /* Y     - attack 3  */
#define	CONTROL_PAD1_FIRE4		(JOY_LIST_FIRST + 5)    /* LB    - attack 4  */
#define	CONTROL_PAD1_FIRE5		(JOY_LIST_FIRST + 2)    /* B     - jump      */
#define	CONTROL_PAD1_FIRE6		(JOY_LIST_FIRST + 6)    /* RB    - special   */
#define CONTROL_PAD1_START		(JOY_LIST_FIRST + 8)    /* Menu  - start     */
#define CONTROL_PAD1_SCREENSHOT	(JOY_LIST_FIRST + 7)    /* View             */

#define	CONTROL_DEFAULT2_UP		((JOY_LIST_FIRST + 24) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_RIGHT		((JOY_LIST_FIRST + 25) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_DOWN		((JOY_LIST_FIRST + 26) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_LEFT		((JOY_LIST_FIRST + 27) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_FIRE1		((JOY_LIST_FIRST + 1) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_FIRE2		((JOY_LIST_FIRST + 3) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_FIRE3		((JOY_LIST_FIRST + 4) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_FIRE4		((JOY_LIST_FIRST + 5) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_FIRE5		((JOY_LIST_FIRST + 2) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_FIRE6		((JOY_LIST_FIRST + 6) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_START		((JOY_LIST_FIRST + 8) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_SCREENSHOT		((JOY_LIST_FIRST + 7) + JOY_MAX_INPUTS)
#define	CONTROL_DEFAULT2_ESC        ((JOY_LIST_FIRST + 15) + JOY_MAX_INPUTS)

#define	CONTROL_DEFAULT3_UP			((JOY_LIST_FIRST + 1) + (JOY_MAX_INPUTS * 2))
#define	CONTROL_DEFAULT3_RIGHT		((JOY_LIST_FIRST + 2) + (JOY_MAX_INPUTS * 2))
#define	CONTROL_DEFAULT3_DOWN		((JOY_LIST_FIRST + 3) + (JOY_MAX_INPUTS * 2))
#define	CONTROL_DEFAULT3_LEFT		((JOY_LIST_FIRST + 4) + (JOY_MAX_INPUTS * 2))
#define CONTROL_DEFAULT3_FIRE1		((JOY_LIST_FIRST + 5) + (JOY_MAX_INPUTS * 2))
#define CONTROL_DEFAULT3_FIRE2		((JOY_LIST_FIRST + 6) + (JOY_MAX_INPUTS * 2))
#define	CONTROL_DEFAULT3_FIRE3		((JOY_LIST_FIRST + 7) + (JOY_MAX_INPUTS * 2))
#define	CONTROL_DEFAULT3_FIRE4		((JOY_LIST_FIRST + 8) + (JOY_MAX_INPUTS * 2))
#define	CONTROL_DEFAULT3_FIRE5		((JOY_LIST_FIRST + 9) + (JOY_MAX_INPUTS * 2))
#define	CONTROL_DEFAULT3_FIRE6		((JOY_LIST_FIRST + 10) + (JOY_MAX_INPUTS * 2))
#define CONTROL_DEFAULT3_START		((JOY_LIST_FIRST + 11) + (JOY_MAX_INPUTS * 2))
#define CONTROL_DEFAULT3_SCREENSHOT ((JOY_LIST_FIRST + 12) + (JOY_MAX_INPUTS * 2))
#define	CONTROL_DEFAULT3_ESC        ((JOY_LIST_FIRST + 15) + (JOY_MAX_INPUTS * 2))

#define	CONTROL_DEFAULT4_UP			((JOY_LIST_FIRST + 1) + (JOY_MAX_INPUTS * 3))
#define	CONTROL_DEFAULT4_RIGHT		((JOY_LIST_FIRST + 2) + (JOY_MAX_INPUTS * 3))
#define	CONTROL_DEFAULT4_DOWN		((JOY_LIST_FIRST + 3) + (JOY_MAX_INPUTS * 3))
#define	CONTROL_DEFAULT4_LEFT		((JOY_LIST_FIRST + 4) + (JOY_MAX_INPUTS * 2))
#define CONTROL_DEFAULT4_FIRE1		((JOY_LIST_FIRST + 5) + (JOY_MAX_INPUTS * 2))
#define CONTROL_DEFAULT4_FIRE2		((JOY_LIST_FIRST + 6) + (JOY_MAX_INPUTS * 3))
#define	CONTROL_DEFAULT4_FIRE3		((JOY_LIST_FIRST + 7) + (JOY_MAX_INPUTS * 3))
#define	CONTROL_DEFAULT4_FIRE4		((JOY_LIST_FIRST + 8) + (JOY_MAX_INPUTS * 3))
#define	CONTROL_DEFAULT4_FIRE5		((JOY_LIST_FIRST + 9) + (JOY_MAX_INPUTS * 3))
#define	CONTROL_DEFAULT4_FIRE6		((JOY_LIST_FIRST + 10) + (JOY_MAX_INPUTS * 2))
#define CONTROL_DEFAULT4_START		((JOY_LIST_FIRST + 11) + (JOY_MAX_INPUTS * 2))
#define CONTROL_DEFAULT4_SCREENSHOT ((JOY_LIST_FIRST + 12) + (JOY_MAX_INPUTS * 3))
#define	CONTROL_DEFAULT4_ESC        ((JOY_LIST_FIRST + 15) + (JOY_MAX_INPUTS * 2))

#define	CONTROL_NONE				((JOY_LIST_FIRST + 1) + (JOY_MAX_INPUTS * 99)) //Kratus (20-04-21) value used to clear all keys

#define JOYBUTTON(index, btn) (1 + i * JOY_MAX_INPUTS + btn)
#define JOYAXIS(index, axis, dir) (JOYBUTTON(index, joysticks[index].NumButtons) + 2 * axis + dir)

#define SDLK_FIRST SDL_SCANCODE_UNKNOWN
#define SDLK_LAST  SDL_NUM_SCANCODES
#define SDL_GetKeyState SDL_GetKeyboardState
#define SDL_JoystickName(x) SDL_JoystickName(joystick[x])

typedef struct{
	int		settings[JOY_MAX_INPUTS];
	u64		keyflags, newkeyflags;
	int		kb_break;
}s_playercontrols;

void open_joystick(int i);
void close_joystick(int i);
void control_exit();
void control_init(int joy_enable);
int control_usejoy(int enable);
int control_getjoyenabled();

void control_setkey(s_playercontrols * pcontrols, unsigned int flag, int key);
int control_scankey();

void set_default_joystick_keynames(int i);
void reset_joystick_map(int i);
char* get_joystick_name(const char* name);
char *control_getkeyname(unsigned int keycode);
void control_update(s_playercontrols ** playercontrols, int numplayers);
void control_rumble(int port, int ratio, int msec);
int keyboard_getlastkey();

#ifdef ANDROID
#define MAX_POINTERS 30
typedef enum
{
    TOUCH_STATUS_UP,
    TOUCH_STATUS_DOWN
} touch_status;

typedef struct TouchStatus {
    float px[MAX_POINTERS];
    float py[MAX_POINTERS];
    SDL_FingerID pid[MAX_POINTERS];
    touch_status pstatus[MAX_POINTERS];
} TouchStatus;

int is_touchpad_vibration_enabled();
void control_update_android_touch(TouchStatus *touch_info, int maxp, Uint8* keystate, Uint8* keystate_def);
int is_touch_area(float x, float y);
#endif



#endif

