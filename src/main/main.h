#pragma once

#include "globals.h"

#ifdef COMPILE_SOUND_CODE
#include "sdl/audio.h"
#endif



// Frame counter
extern int cannonball_frame;

// Tick Logic. Used when running at non-standard > 30 fps
extern Boolean cannonball_tick_frame;

// Millisecond Time Per Frame
extern double cannonball_frame_ms;

// FPS Counter
extern int cannonball_fps_counter;

// Engine Master State
extern int cannonball_state;
    
enum
{
    STATE_BOOT,
    STATE_INIT_MENU,
    STATE_MENU,
    STATE_INIT_GAME,
    STATE_GAME,
    STATE_QUIT
};
