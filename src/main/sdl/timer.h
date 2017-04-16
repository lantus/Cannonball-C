#pragma once

#include "stdint.h"
#include <SDL.h>

typedef struct
{
    //The clock time when the timer started
    int startTicks;

    //The ticks stored when the timer was paused
    int pausedTicks;

    //The timer status
    Boolean paused;
    Boolean started;
} Timer;


//The various clock actions
void Timer_init(Timer* timer);
void Timer_start(Timer* timer);
void Timer_stop(Timer* timer);
void Timer_pause(Timer* timer);
void Timer_unpause(Timer* timer);

//Gets the timer's time
int Timer_get_ticks(Timer* timer);

//Checks the status of the timer
Boolean Timer_is_started(Timer* timer);
Boolean Timer_is_paused(Timer* timer);