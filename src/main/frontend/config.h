/***************************************************************************
    XML Configuration File Handling.

    Load Settings.
    Load & Save Hi-Scores.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once


#include "stdint.h"


typedef struct
{
    int enabled;
    char title[128];
    char filename[512];
} custom_music_t;

typedef struct
{
    int laps;
    int traffic;
    uint16_t best_times[15];
} ttrial_settings_t;

typedef struct 
{
    int enabled;
    int road_scroll_speed;
} menu_settings_t;

enum
{
    VIDEO_MODE_WINDOW,
    VIDEO_MODE_FULL,
    VIDEO_MODE_STRETCH
};

typedef struct
{
    int mode;
    int scale;
    int scanlines;
    int widescreen;
    int fps;
    int fps_count;
    int hires;
    int filtering;
#if defined (_AMIGA_) 
    int detailLevel;
    int clipPlane;
#endif    
} video_settings_t;

typedef struct
{
    int enabled;
    int advertise;
    int preview;
    int fix_samples;
    custom_music_t custom_music[4];
#if defined (_AMIGA_)    
    int amiga_midi;
    int amiga_mods;
    int amiga_fx;
#endif    
} sound_settings_t;

enum
{
    CONTROLS_GEAR_BUTTON,
    CONTROLS_GEAR_PRESS,     // For cabinets
    CONTROLS_GEAR_SEPARATE,  // Separate button presses
    CONTROLS_GEAR_AUTO
};

typedef struct
{
    int gear;
    int steer_speed;   // Steering Digital Speed
    int pedal_speed;   // Pedal Digital Speed
    int padconfig[8];  // Joypad Button Config
    int keyconfig[12]; // Keyboard Button Config
    int pad_id;        // Use the N'th joystick on the system.
    int analog;        // Use analog controls
    int axis[3];       // Analog Axis
    int asettings[3];  // Analog Settings

    int haptic;        // Force Feedback Enabled
    int max_force;
    int min_force;
    int force_duration;
} controls_settings_t;

enum
{
    CABINET_MOVING  = 0,
    CABINET_UPRIGHT = 1,
    CABINET_MINI    = 2
};

typedef struct
{
    int enabled;      // CannonBall used in conjunction with CannonBoard in arcade cabinet
    char port[64]; // Port Name
    int baud;         // Baud Rate
    int debug;        // Display Debug Information
    int cabinet;      // Cabinet Type
} cannonboard_settings_t;

typedef struct
{
    int dip_time;
    int dip_traffic;
    Boolean freeplay;
    Boolean freeze_timer;
    Boolean disable_traffic;
    int jap;
    int prototype;
    int randomgen;
    int level_objects;
    Boolean fix_bugs;
    Boolean fix_bugs_backup;
    Boolean fix_timer;
    Boolean layout_debug;
    int new_attract;
} engine_settings_t;

extern menu_settings_t        Config_menu;
extern video_settings_t       Config_video;
extern sound_settings_t       Config_sound;
extern controls_settings_t    Config_controls;
extern engine_settings_t      Config_engine;
extern ttrial_settings_t      Config_ttrial;
extern cannonboard_settings_t Config_cannonboard;

// Internal screen width and height
extern uint16_t Config_s16_width;
extern uint16_t Config_s16_height;

// Internal screen x offset
extern uint16_t Config_s16_x_off;

// 30 or 60 fps
extern int Config_fps;

// Original game ticks sprites at 30fps but background scroll at 60fps
extern int Config_tick_fps;

// Continuous Mode: Traffic Setting
extern int Config_cont_traffic;
    

void Config_init();
void Config_load(const char* filename);
Boolean Config_save(const char* filename);
void Config_load_scores(const char* filename);
void Config_save_scores(const char* filename);
void Config_load_tiletrial_scores();
void Config_save_tiletrial_scores();
Boolean Config_clear_scores();
void Config_set_fps(int fps);
   
