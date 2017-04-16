/***************************************************************************
    Interface to Ported Z80 Code.
    Handles the interface between 68000 program code and Z80.

    Also abstracted here, so the more complex OSound class isn't exposed
    to the main code directly
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "engine/outrun.h"
#include "engine/audio/OSound.h"
#include "engine/audio/OSoundInt.h"


// SoundChip: Sega Custom Sample Generator
Boolean OSoundInt_has_booted = FALSE;
uint8_t OSoundInt_engine_data[8];

// 4 MHz
static const uint32_t SOUND_CLOCK = 4000000;

// Reference to 0xFF bytes of PCM Chip RAM
uint8_t OSOoundInt_pcm_ram[OSoundInt_PCM_RAM_SIZE];

// Controls what type of sound we're going to process in the interrupt routine
uint8_t sound_counter;

#define QUEUE_LENGTH 0x1F
uint8_t queue[QUEUE_LENGTH + 1];

// Number of sounds queued
uint8_t sounds_queued;

// Positions in the queue
uint8_t sound_head, sound_tail;


uint8_t OSoundInt_created_pcm = 0;
uint8_t OSoundInt_created_ym = 0;

void OSoundInt_init()
{
    uint16_t i;
    if (!OSoundInt_created_pcm)
    {
        SegaPCM_Create(SOUND_CLOCK, &Roms_pcm, OSOoundInt_pcm_ram, SEGAPCM_BANK_512);
        OSoundInt_created_pcm = 1;
    }

    if (!OSoundInt_created_ym)
    {
       YM_Create(0.5f, SOUND_CLOCK);
        OSoundInt_created_ym = 1;
    }

    SegaPCM_init(Config_fps);
    YM_init(44100, Config_fps);

    OSoundInt_reset();

    // Clear PCM Chip RAM
    for (i = 0; i < OSoundInt_PCM_RAM_SIZE; i++)
        OSOoundInt_pcm_ram[i] = 0;

    for (i = 0; i < 8; i++)
        OSoundInt_engine_data[i] = 0;

    OSound_init(OSOoundInt_pcm_ram);
}

// Clear sound queue
// Source: 0x5086
void OSoundInt_reset()
{
    sound_counter = 0;
    sound_head    = 0;
    sound_tail    = 0;
    sounds_queued = 0;
}

void OSoundInt_tick()
{
    if (Config_fps == 30)
    {
        OSoundInt_play_queued_sound(); // Process audio commands from main program code
        OSound_tick();
        OSoundInt_play_queued_sound();
        OSound_tick();
        OSoundInt_play_queued_sound();
        OSound_tick();
        OSoundInt_play_queued_sound();
        OSound_tick();
    }
    else if (Config_fps == 60)
    {
        OSoundInt_play_queued_sound(); // Process audio commands from main program code
        OSound_tick();
        OSound_tick();
    }
}

// ----------------------------------------------------------------------------
// Sound Queuing Code
// ----------------------------------------------------------------------------

// Play Queued Sounds & Send Engine Noise Commands to Z80
// Was called by horizontal interrupt routine
// Source: 0x564E
void OSoundInt_play_queued_sound()
{
    int counter;
    if (!OSoundInt_has_booted)
    {
        sound_head = 0;
        sounds_queued = 0;
        return;
    }

    // Process the lot in one go. 
    for (counter = 0; counter < 8; counter++)
    {
        // Process queued sound
        if (counter == 0)
        {
            if (sounds_queued != 0)
            {
                OSound_command_input = queue[sound_head];
                sound_head = (sound_head + 1) & QUEUE_LENGTH;
                sounds_queued--;
            }
            else
            {
                OSound_command_input = sound_RESET;
            }
        }
        // Process player engine sounds and passing traffic
        else
        {
            OSound_engine_data[counter] = OSoundInt_engine_data[counter];
        }
    }
}

void OSoundInt_add_to_queue(uint8_t snd)
{
    // Add sound to the tail end of the queue
    queue[sound_tail] = snd;
    sound_tail = (sound_tail + 1) & QUEUE_LENGTH;
    sounds_queued++;
}

void OSoundInt_queue_clear()
{
    sound_tail = 0;
    sounds_queued = 0;
}

// Queue a sound in service mode
// Used to trigger both sound effects and music
// Source: 0x56C6
void OSoundInt_queue_sound_service(uint8_t snd)
{
    if (OSoundInt_has_booted)
        OSoundInt_add_to_queue(snd);
    else
        OSoundInt_queue_clear();
}

// Queue a sound in-game
// Note: This version has an additional check, so that certain sounds aren't played depending on game mode
// Source: 0x56D4
void OSoundInt_queue_sound(uint8_t snd)
{
    
#if !defined (_AMIGA_)    
    if (OSoundInt_has_booted)
    {
        if (Outrun_game_state == GS_ATTRACT)
        {
            // Return if we are not playing sound in attract mode
            if (!Config_sound.advertise && snd != sound_COIN_IN) return;

            // Do not play music in attract mode, even if attract sound enabled
            if (snd == SOUND_MUSIC_BREEZE || snd == SOUND_MUSIC_MAGICAL ||
                snd == SOUND_MUSIC_SPLASH || snd == SOUND_MUSIC_LASTWAVE)
                return;
        }
        OSoundInt_add_to_queue(snd);
    }
    else
        OSoundInt_queue_clear();
#endif
        
}

