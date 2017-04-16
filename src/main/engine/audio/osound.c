/***************************************************************************
    Ported Z80 Sound Playing Code.
    Controls Sega PCM and YM2151 Chips.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

/*
TODO:

- Double check passing car tones, especially when going through checkpoint areas
X Finish Ferrari sound code
- Engine tones seem to jump channels. How do we stop this? Is it a bug in the original?
X More cars seem to be high pitched than on MAME. (Fixed - engine channel setup)

*/

#include <string.h>
#include "engine/audio/osound.h"
#include <stdio.h>

uint8_t OSound_command_input;

// [+0] Unused
// [+1] Engine pitch high
// [+2] Engine pitch low
// [+3] Engine pitch vol
// [+4] Traffic data #1 
// [+5] Traffic data #2 
// [+6] Traffic data #3 
// [+7] Traffic data #4
uint8_t OSound_engine_data[8];


#define PCM_RAM_SIZE  0x100
#define CHAN_RAM_SIZE 0x800

// Internal channel format
uint8_t chan_ram[CHAN_RAM_SIZE];

// Size of each internal channel entry
const static uint8_t CHAN_SIZE = 0x20;

// ------------------------------------------------------------------------------------------------
// Format of Data in PCM RAM
// ------------------------------------------------------------------------------------------------

// RAM DESCRIPTION ===============
//
// 0x00 - 0x07, 0x80 - 0x87 : CHANNEL #1  
// 0x08 - 0x0F, 0x88 - 0x8F : CHANNEL #2
// 0x10 - 0x17, 0x90 - 0x97 : CHANNEL #3  
// 0x18 - 0x1F, 0x98 - 0x9F : CHANNEL #4
// 0x20 - 0x27, 0xA0 - 0xA7 : CHANNEL #5  
// 0x28 - 0x2F, 0xA8 - 0xAF : CHANNEL #6
// 0x30 - 0x37, 0xB0 - 0xB7 : CHANNEL #7  
// 0x38 - 0x3F, 0xB8 - 0xBF : CHANNEL #8
// 0x40 - 0x47, 0xC0 - 0xC7 : CHANNEL #9  
// 0x48 - 0x4F, 0xC8 - 0xCF : CHANNEL #10
// 0x50 - 0x57, 0xD0 - 0xD7 : CHANNEL #11 
// 0x58 - 0x5F, 0xD8 - 0xDF : CHANNEL #12
// 0x60 - 0x67, 0xE0 - 0xE7 : CHANNEL #13 
// 0x68 - 0x6F, 0xE8 - 0xEF : CHANNEL #14
// 0x70 - 0x77, 0xF0 - 0xF7 : CHANNEL #15 
// 0x78 - 0x7F, 0xF8 - 0xFF : CHANNEL #16
//
//
// CHANNEL DESCRIPTION ===================
//  
// OFFS | BITS     | DESCRIPTION 
// -----+----------+---------------------------------
// 0x00 | -------- | (unknown) <- scratch space for pitch engine 1 noise or vol
// 0x01 | -------- | (unknown) <- scratch space for pitch engine 2 noise or vol
// 0x02 | vvvvvvvv | Volume LEFT 
// 0x03 | vvvvvvvv | Volume RIGHT 
// 0x04 | aaaaaaaa | Wave Start Address LOW 8 bits 
// 0x05 | aaaaaaaa | Wave Start Address HIGH 8 bits 
// 0x06 | eeeeeeee | Wave End Address HIGH 8 bits 
// 0x07 | dddddddd | Delta (pitch) 
// 0x80 | -------- | (unknown) Traffic Volume Boost & Pitch Table Entry (Distance of Traffic)
//      |          |           OR for Engine Channels: Offset into engine_adr_table [Start addresses]
// 0x81 | -------- | (unknown) Traffic Panning Table Entry
// 0x82 | -------- | (unknown) <- scratch space. 
//      |          |           bit 5: Mute channel (engine sounds)
//      |          |           bit 4: pitch slide.
//      |          |           bit 3: set to enable traffic sound
//      |          |           bit 2: set to denote wave start/end address setup. 
//      |          |           bit 1: set to denote traffic pan is unchanged. unset denotes change.
//      |          |           bit 0: set to denote traffic vol is unchanged. unset denotes change.
//      |          |
//      |          |           OR for Engine Channels:
//      |          |           bit 2: clear to denote offset into engine_adr_table has been reset. 
// 0x83 | -------- | (unknown) Traffic Volume Multiplier (read from table specified by 0x80).
// 0x84 | llllllll | Wave Loop Address LOW 8 bits
// 0x85 | llllllll | Wave Loop Address HIGH 8 bits 
// 0x86 | ------la | Flags: a = active (0 = active, 1 = inactive) 
//      |          |        l = loop   (0 = enabled, 1 = disabled)

// Reference to 0xFF bytes of PCM Chip RAM
uint8_t* pcm_ram;

// Bit 0: Set denotes car stationary do rev sample when revs high enough
// Bit 1: Set to denote PCM sound effect triggered.
uint8_t sound_props;

// Stored Command
uint8_t command_index;

// F810 - F813
uint8_t counter1, counter2, counter3, counter4;

// Position in sequence [de]
uint16_t pos;

// Store last command to assist program flow
uint8_t cmd_prev;

// Store last chan ID
uint16_t chanid_prev;

// PCM Channel Commands in RAM to send
const static uint16_t CH09_CMDS1 = 0x570; // 0xFD70;
const static uint16_t CH09_CMDS2 = 0x578;
const static uint16_t CH11_CMDS1 = 0x580; // 0xFD80;
const static uint16_t CH11_CMDS2 = 0x588;

// Panning flags
#define PAN_LEFT  0x40
#define PAN_RIGHT 0x80
#define PAN_CENTRE (PAN_LEFT | PAN_RIGHT)

// ------------------------------------------------------------------------
// ENGINE TONE CODE
// ------------------------------------------------------------------------
// Used to skip the engine code 1/2 times
uint8_t engine_counter;

// Engine Channel: Selects Channel at offset 0xF800 for engine tones
uint8_t engine_channel;

uint8_t OSound_pcm_r(uint16_t adr);
void    OSound_pcm_w(uint16_t adr, uint8_t v);
uint16_t OSound_r16(uint8_t* adr);
void     OSound_w16(uint8_t* adr, uint16_t v);
void OSound_process_command();
void OSound_pcm_backup();
void OSound_check_fm_mapping();
void OSound_process_channels();
void OSound_process_channel(uint16_t chan_id);
void OSound_process_section(uint8_t* chan);
void OSound_calc_end_marker(uint8_t* chan);
void OSound_do_command(uint8_t* chan, uint8_t cmd);
void OSound_new_command();
void OSound_play_pcm_index(uint8_t* chan, uint8_t cmd);
void OSound_setvol(uint8_t* chan);
void OSound_pcm_setpitch(uint8_t* chan);
void OSound_set_loop_adr();
void OSound_do_loop(uint8_t* chan);
void OSound_pcm_finalize(uint8_t* chan);
//void pcm_send_cmds(uint16_t src, uint16_t pcm_adr);
void OSound_init_sound(uint8_t cmd, uint16_t src, uint16_t dst);
void OSound_process_pcm(uint8_t* chan);
void OSound_pcm_send_cmds(uint8_t* chan, uint16_t pcm_adr, uint8_t channel_pair);
void OSound_fm_dotimera();
void OSound_fm_reset();
void OSound_fm_write_reg_c(uint8_t ix0, uint8_t reg, uint8_t value);
void OSound_fm_write_reg(uint8_t reg, uint8_t value);
void OSound_fm_write_block(uint8_t ix0, uint16_t adr, uint8_t chan);
void OSound_ym_set_levels();
void OSound_ym_set_block(uint8_t* chan);
uint16_t OSound_ym_lookup_data(uint8_t cmd, uint8_t offset, uint8_t block);
void OSound_ym_set_connect(uint8_t* chan, uint8_t pan);
void OSound_ym_finalize(uint8_t* chan);
void OSound_read_mod_table(uint8_t* chan);
void OSound_write_seq_adr(uint8_t* chan);

// ------------------------------------------------------------------------
// ENGINE TONE FUNCTIONS
// ------------------------------------------------------------------------
void OSound_engine_process();
void OSound_engine_process_chan(uint8_t* chan, uint8_t* pcm);
void OSound_vol_thicken(uint16_t* pos, uint8_t* chan, uint8_t* pcm);
uint8_t OSound_get_adjusted_vol(uint16_t* pos, uint8_t* chan);
void OSound_engine_set_pitch(uint16_t* pos, uint8_t* pcm);
void OSound_engine_mute_channel(uint8_t* chan, uint8_t* pcm, Boolean do_check);
void OSound_unk78c7(uint8_t* chan, uint8_t* pcm);
void OSound_ferrari_vol_pan(uint8_t* chan, uint8_t* pcm);
uint16_t OSound_engine_get_table_adr(uint8_t* chan, uint8_t* pcm);
void OSound_engine_adjust_volume(uint8_t* chan);
uint16_t OSound_engine_set_adr(uint16_t* pos, uint8_t* chan, uint8_t* pcm);
void OSound_engine_set_adr_end(uint16_t* pos, uint16_t loop_adr, uint8_t* chan, uint8_t* pcm);
void OSound_engine_set_pan(uint16_t* pos, uint8_t* chan, uint8_t* pcm);
void OSound_engine_read_data(uint8_t* chan, uint8_t* pcm);

// ----------------------------------------------------------------------------
//                               PASSING TRAFFIC FX 
// ----------------------------------------------------------------------------
void OSound_traffic_process();
void OSound_traffic_process_chan(uint8_t* pcm);
void OSound_traffic_process_entry(uint8_t* pcm);
void OSound_traffic_disable(uint8_t* pcm);
void OSound_traffic_set_vol(uint8_t* pcm);
void OSound_traffic_set_pan(uint8_t* pcm);
uint8_t OSound_traffic_get_vol(uint16_t pos, uint8_t* pcm);
void OSound_traffic_note_changes(uint8_t new_vol, uint8_t* pcm);
void OSound_traffic_read_data(uint8_t* pcm);


// Use YM2151 Timing
#define TIMER_CODE 1

// Enable Unused code block warnings
//#define UNUSED_WARNINGS 1


void OSound_init(uint8_t* _pcm_ram)
{
    uint16_t i;
    int8_t j;
    pcm_ram = _pcm_ram;

    OSound_command_input = 0;
    sound_props   = 0;
    pos           = 0;
    counter1      = 0;
    counter2      = 0;
    counter3      = 0;
    counter4      = 0;

    engine_counter= 0;

    // Clear AM RAM 0xF800 - 0xFFFF
    for (i = 0; i < CHAN_RAM_SIZE; i++)
        chan_ram[i] = 0;

    // Enable all PCM channels by default
    for (j = 0; j < 16; j++)
        pcm_ram[0x86 + (j * 8)] = 1; // Channel Active

    OSound_init_fm_chip();
}

// Initialize FM Chip. Initalize and start Timer A.
// Source: 0x79
void OSound_init_fm_chip()
{
   OSound_command_input = sound_RESET;

   // Initialize the FM Chip with the set of default commands
   OSound_fm_write_block(0, z80_adr_YM_INIT_CMDS, 0);

   // Start Timer A & enable its IRQ, and do an IRQ reset (%00110101)
   OSound_fm_write_reg(0x14, 0x35);
}

void OSound_tick()
{
    OSound_fm_dotimera();          // FM: Process Timer A. Stop Timer B
    OSound_process_command();      // Process Command sent by main program code (originally the main 68k processor)
    OSound_process_channels();     // Run logic on individual sound channel (both YM & PCM channels)
    OSound_engine_process();       // Ferrari Engine Tone & Traffic Noise
    OSound_traffic_process();      // Traffic Volume/Panning & Pitch
}

// PCM RAM Read/Write Helper Functions
uint8_t OSound_pcm_r(uint16_t adr)
{
    return pcm_ram[adr & 0xFF];
}

void OSound_pcm_w(uint16_t adr, uint8_t v)
{
    pcm_ram[adr & 0xFF] = v;
}

// RAM Read/Write Helper Functions
uint16_t OSound_r16(uint8_t* adr)
{
    return ((*(adr+1) << 8) | *adr);
}

void OSound_w16(uint8_t* adr, uint16_t v)
{
    *adr     = v & 0xFF;
    *(adr+1) = v >> 8;
}

// Process Command Sent By 68K CPU
// Source: 0x74F
void OSound_process_command()
{
    if (OSound_command_input == sound_RESET)
        return;
    // Clear Z80 Command
    else if (OSound_command_input < 0x80 || OSound_command_input >= 0xFF)
    {
        // Reset FM Chip
        if (OSound_command_input == sound_FM_RESET || OSound_command_input == 0xFF)
            OSound_fm_reset();

        OSound_command_input = sound_RESET;
        OSound_new_command();
    }
    else
    {
        uint8_t cmd   = OSound_command_input;
        OSound_command_input = sound_RESET;

        switch (cmd)
        {
            case sound_RESET:
                break;

            case SOUND_MUSIC_BREEZE:
                sound_props |= BIT_0; // Trigger rev effect
            case SOUND_MUSIC_BREEZE2:
                cmd = SOUND_MUSIC_BREEZE;
                OSound_fm_reset();
                OSound_init_sound(cmd, z80_adr_DATA_BREEZE, channel_YM1);
                break;

            case SOUND_MUSIC_SPLASH:
                sound_props |= BIT_0; // Trigger rev effect
            case SOUND_MUSIC_SPLASH2:
                cmd = SOUND_MUSIC_SPLASH;
                OSound_fm_reset();
                OSound_init_sound(cmd, z80_adr_DATA_SPLASH, channel_YM1);
                break;

            case sound_COIN_IN:
                OSound_init_sound(cmd, z80_adr_DATA_COININ, channel_YM_FX1);
                break;

            case SOUND_MUSIC_MAGICAL:
                sound_props |= BIT_0; // Trigger rev effect
            case SOUND_MUSIC_MAGICAL2:
                cmd = SOUND_MUSIC_MAGICAL;
                OSound_fm_reset();
                OSound_init_sound(cmd, z80_adr_DATA_MAGICAL, channel_YM1);
                break;

            case sound_YM_CHECKPOINT:
                OSound_init_sound(cmd, z80_adr_DATA_CHECKPOINT, channel_YM_FX1);
                break;

            case sound_INIT_SLIP:
                OSound_init_sound(cmd, z80_adr_DATA_SLIP, channel_PCM_FX3);
                break;

            case sound_INIT_CHEERS:
                OSound_init_sound(cmd, z80_adr_DATA_CHEERS, channel_PCM_FX1);
                break;

            case sound_STOP_CHEERS:
                chan_ram[channel_PCM_FX1] = 0;
                chan_ram[channel_PCM_FX2] = 0;
                OSound_pcm_w(0xF08E, 1); // Set inactive flag on channels
                OSound_pcm_w(0xF09E, 1);
                break;

            case sound_CRASH1:
                OSound_init_sound(cmd, z80_adr_DATA_CRASH1, channel_PCM_FX5);
                break;

            case sound_REBOUND:
                OSound_init_sound(cmd, z80_adr_DATA_REBOUND, channel_PCM_FX5);
                break;

            case sound_CRASH2:
                OSound_init_sound(cmd, z80_adr_DATA_CRASH2, channel_PCM_FX5);
                break;

            case sound_NEW_COMMAND:
                OSound_new_command();
                break;

            case sound_SIGNAL1:
                OSound_init_sound(cmd, z80_adr_DATA_SIGNAL1, channel_YM_FX1);
                break;

            case sound_SIGNAL2:
                sound_props &= ~BIT_0; // Clear rev effect
                OSound_init_sound(cmd, z80_adr_DATA_SIGNAL2, channel_YM_FX1);
                break;

            case sound_INIT_WEIRD:
                OSound_init_sound(cmd, z80_adr_DATA_WEIRD, channel_PCM_FX5);
                break;

            case sound_STOP_WEIRD:
                chan_ram[channel_PCM_FX5] = 0;
                chan_ram[channel_PCM_FX6] = 0;
                OSound_pcm_w(0xF0CE, 1); // Set inactive flag on channels
                OSound_pcm_w(0xF0DE, 1);
                break;

            case sound_REVS:
                OSound_fm_reset();
                sound_props |= BIT_0; // Trigger rev effect
                break;

            case sound_BEEP1:
                OSound_init_sound(cmd, z80_adr_DATA_BEEP1, channel_YM_FX1);
                break;

            case sound_UFO:
                OSound_fm_reset();
                OSound_init_sound(cmd, z80_adr_DATA_UFO, channel_YM_FX1);
                break;

            case sound_BEEP2:
                OSound_fm_reset();
                OSound_init_sound(cmd, z80_adr_DATA_BEEP2, channel_YM1);
                break;

            case sound_INIT_CHEERS2:
                OSound_init_sound(cmd, z80_adr_DATA_CHEERS2, channel_PCM_FX1);
                break;

            case sound_VOICE_CHECKPOINT:
                OSound_pcm_backup();
                OSound_init_sound(cmd, z80_adr_DATA_VOICE1, channel_PCM_FX7);
                break;

            case sound_VOICE_CONGRATS:
                OSound_pcm_backup();
                OSound_init_sound(cmd, z80_adr_DATA_VOICE2, channel_PCM_FX7);
                break;

            case sound_VOICE_GETREADY:
                OSound_pcm_backup();
                OSound_init_sound(cmd, z80_adr_DATA_VOICE3, channel_PCM_FX7);
                break;

            case sound_INIT_SAFETYZONE:
                OSound_init_sound(cmd, z80_adr_DATA_SAFETY, channel_PCM_FX3);
                break;

            case sound_STOP_SLIP:
            case sound_STOP_SAFETYZONE:
                chan_ram[channel_PCM_FX3] = 0;
                chan_ram[channel_PCM_FX4] = 0;
                OSound_pcm_w(0xF0AE, 1); // Set inactive flag on channels
                OSound_pcm_w(0xF0BE, 1);
                break;

            case sound_YM_SET_LEVELS:
                OSound_ym_set_levels();
                break;

            case sound_PCM_WAVE:
                OSound_init_sound(cmd, z80_adr_DATA_WAVE, channel_PCM_FX1);
                break;

            case SOUND_MUSIC_LASTWAVE:
                OSound_init_sound(cmd, z80_adr_DATA_LASTWAVE, channel_YM1);
                break;

            #ifdef UNUSED_WARNINGS
            default:
                fprintf(stderr, "Missing command: %d\n", cmd);
                break;
            #endif
        }
    }
}

// Called before initalizing a new command (PCM standalone sample, FM sample, New Music)
// Source: 0x833
void OSound_new_command()
{
    uint8_t i;
    // ------------------------------------------------------------------------
    // FM Sound Effects Only (Increase Volume)
    // ------------------------------------------------------------------------
    if (chan_ram[channel_YM_FX1] & BIT_7)
    {
        chan_ram[channel_YM_FX1] = 0;

        uint16_t adr = z80_adr_YM_LEVEL_CMDS2;

        // Send four level commands
        for (i = 0; i < 4; i++)
        {
            uint8_t reg = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
            uint8_t val = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
            OSound_fm_write_reg(reg, val);
        }
    }

    // Clear channel memory area used for internal format of PCM sound data
    for (i = channel_PCM_FX1; i < channel_YM_FX1; i++)
        chan_ram[i] = 0;

    // PCM Channel Enable
    uint16_t pcm_enable = 0xF088 + 6;

    // Disable 6 PCM Channels
    for (i = 0; i < 6; i++)
    {
        OSound_pcm_w(pcm_enable, OSound_pcm_r(pcm_enable) | BIT_0); 
        pcm_enable += 0x10; // Advance to next channel
    }
}

// Copy PCM Channel RAM contents to Channel RAM
// Source: 0x961
void OSound_pcm_backup()
{
    // Return if PCM Channel contents already backed up
    if (sound_props & BIT_1)
        return;

    memcpy(chan_ram + CH09_CMDS1, pcm_ram + 0x40, 8); // Channel 9 Blocks
    memcpy(chan_ram + CH09_CMDS2, pcm_ram + 0xC0, 8);
    memcpy(chan_ram + CH11_CMDS1, pcm_ram + 0x50, 8); // Channel 11 Blocks
    memcpy(chan_ram + CH11_CMDS1, pcm_ram + 0xD0, 8);

    sound_props |= BIT_1; // Denote contents backed up
}

// Check whether FM Channel is in use
// Map back to corresponding music channel
//
// Source: 0x95
void OSound_check_fm_mapping()
{
    uint8_t c;
    uint16_t chan_id = channel_MAP1;

    // 8 Channels
    for (c = 0; c < 8; c++)
    {
        // Map back to corresponding music channel
        if (chan_ram[chan_id] & BIT_7)    
            chan_ram[chan_id - 0x2C0] |= BIT_2;
        chan_id += CHAN_SIZE;
    }
}

// Process and update all sound channels
//
// Source: 0xB3
void OSound_process_channels()
{
    uint8_t c;
    // Allows FM Music & FM Effect to be played simultaneously
    OSound_check_fm_mapping();

    // Channel to process
    uint16_t chan_id = channel_YM1;

    for (c = 0; c < 30; c++)
    {
        // If channel is enabled, process the channel
        if (chan_ram[chan_id] & BIT_7)
            OSound_process_channel(chan_id);

        chan_id += CHAN_SIZE; // Advance to next channel in memory
    }
}

// Update Individual Channel.
//
// Inputs:
// chan_id = Channel ID to process
//
// Source: 0xCD
void OSound_process_channel(uint16_t chan_id)
{
    chanid_prev = chan_id;

    // Get correct offset in RAM
    uint8_t* chan = &chan_ram[chan_id];

    // Increment sequence position
    pos = OSound_r16(&chan[ch_SEQ_POS]) + 1;
    OSound_w16(&chan[ch_SEQ_POS], pos);

    // Sequence end marker
    uint16_t seq_end = OSound_r16(&chan[ch_SEQ_END]);

    if (pos == seq_end)
    {
        pos = OSound_r16(&chan[ch_SEQ_CMD]);
        OSound_process_section(chan);
        
        // Hack to return when last command was a PCM/YM finalize
        // This is here to facilitate program flow. 
        // The Z80 code does loads of funky (or nasty depending on your point of view) stuff with the stack. 
        if (cmd_prev == 0x84 || cmd_prev == 0x99) 
            return;
    }

    // Return if not FM channel
    if (chan[ch_FM_FLAGS] & BIT_6) return;

    // ------------------------------------------------------------------------
    // FM CHANNELS
    // ------------------------------------------------------------------------

    #ifdef UNUSED_WARNINGS
    if (chan[ch_CTRL])
        fprintf(stderr, "process_channel - unimplemented code 0x167\n");

    if (chan[ch_FLAGS] & BIT_5)
        fprintf(stderr, "process_channel - unimplemented code 0x21A\n");
    #endif

    // 0xF9:  
    uint8_t reg;
    uint8_t chan_index = chan[ch_FM_FLAGS] & 7;

    // Use Phase and Amplitude Modulation Sensitivity Table?
    if (chan[ch_FM_PHASETBL])
    {
        OSound_read_mod_table(chan);
    }

    // If note set to 0xFF, turn off the channel output
    if (chan[ch_FM_NOTE] == 0xFF)
    {
        OSound_fm_write_reg_c(chan[ch_FLAGS], 8, chan_index);
        return;
    }

    // FM Noise Channel
    if (chan[ch_FLAGS] & BIT_1)
    {
        reg = 0xF; // Register = Noise Enable & Frequency
    }
    // Set Volume or Mute if PHASETBL not setup
    else
    {
        // 0x119
        // Register = Phase and Amplitude Modulation Sensitivity ($38-$3F)
        OSound_fm_write_reg_c(chan[ch_FLAGS], 0x30 + chan_index, chan[ch_FM_PHASETBL] ? chan[ch_FM_PHASE_AMP] : 0);

        // Register = Create Note ($28-$2F)
        reg = 0x28 + chan_index;
    }

    // set_octave_note: 
    OSound_fm_write_reg_c(chan[ch_FLAGS], reg, chan[ch_FM_NOTE]);

    // Check position in sequence. If expired, set channels on/off
    if (OSound_r16(&chan[ch_SEQ_POS])) return;

    // Turn channels off
    OSound_fm_write_reg_c(chan[ch_FLAGS], 8, chan_index);

    // Turn modulator and carry channels off
    OSound_fm_write_reg_c(chan[ch_FLAGS], 8, chan_index | 0x78);
}

// Process Channel Section
//
// Source: 0x2E1
void OSound_process_section(uint8_t* chan)
{
    uint8_t cmd = RomLoader_read8IncP_addr16(&Roms_z80, &pos);
    cmd_prev = cmd;
    if (cmd >= 0x80)
    {
        OSound_do_command(chan, cmd);
        return;
    }

    // ------------------------------------------------------------------------
    // FM Only Code From Here Onwards
    // ------------------------------------------------------------------------

    #ifdef UNUSED_WARNINGS
    // Not sure, unused?
    if (chan[ch_FLAGS] & BIT_5)
    {
        fprintf(stderr, "Warning: process_channel - unimplemented code 0x36D\n");
        return;
    }
    
    // Is FM Noise Channel?
    if (chan[ch_FLAGS] & BIT_1)
    {
        fprintf(stderr, "Warning: process_channel - unimplemented code 2\n");
        return;
    }
    #endif
    
    // 0x30d set_note_octave
    // Command is an offset into the Note Offset table in ROM.
    if (cmd)
    {
        uint16_t adr = z80_adr_YM_NOTE_OCTAVE + (cmd - 1 + (int8_t) chan[ch_NOTE_OFFSET]);
        chan[ch_FM_NOTE] = RomLoader_read8_addr16(&Roms_z80, adr);
    }
    // If Channel is mute, clear note information.
    else if (!(chan[ch_FM_FLAGS] & BIT_6))
    {
        chan[ch_FM_NOTE] = 0xFF;
    }

    OSound_calc_end_marker(chan);
}

// Source: 0x31C
void OSound_calc_end_marker(uint8_t* chan)
{
    uint16_t end_marker = RomLoader_read8_addr16(&Roms_z80, pos);
    
    // 32d
    if (chan[ch_FM_MARKER] & BIT_1)
    {
        // Mask on high bit
        if (chan[ch_FM_MARKER] & BIT_0)
        {
            chan[ch_FM_MARKER] &= ~BIT_0;
            end_marker += (RomLoader_read8_addr16(&Roms_z80, ++pos) << 8); 
        }
    }
    // 325
    else
    {
        end_marker = chan[ch_END_MARKER] * end_marker;
    }
  
    OSound_w16(&chan[ch_SEQ_END], end_marker); // Set End Marker    
    OSound_w16(&chan[ch_SEQ_CMD], ++pos);      // Set Next Sequence Command
    OSound_w16(&chan[ch_UNKNOWN], 0);
    OSound_w16(&chan[ch_SEQ_POS], 0);
}


// Trigger New Command From Sound Data.
//
// Source: 0x3A8
void OSound_do_command(uint8_t* chan, uint8_t cmd)
{
    // Play New PCM Sample
    if (cmd >= 0xBF)
    {
        OSound_play_pcm_index(chan, cmd);
        return;
    }

    // Trigger New Command On Channel
    switch (cmd & 0x3F)
    {
        // YM & PCM: Set Volume
        case 0x02:
            OSound_setvol(chan);
            break;

        case 0x04:
            OSound_ym_finalize(chan);
            return;

        // YM: Enable/Disable Modulation table
        case 0x07:
            chan[ch_FM_PHASETBL] = RomLoader_read8_addr16(&Roms_z80, pos);
            break;

        // Write Sequence Address (Used by PCM Drum Samples)
        case 0x08:
            OSound_write_seq_adr(chan);
            break;

        // Set Next Sequence Address [pos] (Used by PCM Drum Samples)
        case 0x09:
            pos = OSound_r16(&chan[chan[ch_MEM_OFFSET]]);
            chan[ch_MEM_OFFSET] += 2;
            break;

        case 0x0A:
            OSound_set_loop_adr();
            break;

        // YM: Set Note/Octave Offset
        case 0x0B:
            chan[ch_NOTE_OFFSET] += RomLoader_read8_addr16(&Roms_z80, pos);
            break;

        case 0x0C:
            OSound_do_loop(chan);
            break;

        case 0x11:
            OSound_ym_set_block(chan);
            break;

        case 0x13:
            OSound_pcm_setpitch(chan);
            break;

        // FM: End Marker - Do not calculate, use value from data
        case 0x14:
            chan[ch_FM_MARKER] |= BIT_1;
            pos--;
            break;

        // FM: End Marker - Set High Byte From Data
        case 0x15:
            chan[ch_FM_MARKER] |= BIT_0;
            pos--;
            break;

        // FM: Connect Channel to Right Speaker
        case 0x16:
            OSound_ym_set_connect(chan, PAN_RIGHT);
            break;

        // FM: Connect Channel to Left Speaker
        case 0x17:
            OSound_ym_set_connect(chan, PAN_LEFT);
            break;

        // FM: Connect Channel to Both Speakers
        case 0x18:
            OSound_ym_set_connect(chan, PAN_CENTRE);
            break;

        case 0x19:
            OSound_pcm_finalize(chan);
            return;
        
        #ifdef UNUSED_WARNINGS
        default:
            fprintf(stderr, "do_command(...) Unsupported command: %x : %d\n", cmd, (cmd & 0x3f));
            break;
        #endif
    }

    pos++;
    OSound_process_section(chan);
}

// Set Volume (Left & Right Channels)
// Source: 0x3D7
void OSound_setvol(uint8_t* chan)
{  
    uint8_t vol_l = RomLoader_read8_addr16(&Roms_z80, pos);

    // PCM Percussion Sample
    if (chan[ch_FM_FLAGS] & BIT_6)
    {
        const uint8_t VOL_MAX = 0x40;

        // Set left volume
        chan[ch_VOL_L] = vol_l > VOL_MAX ? 0 : vol_l;

        // Set right volume
        uint8_t vol_r = RomLoader_read8_addr16(&Roms_z80, ++pos);
        chan[ch_VOL_R] = vol_r > VOL_MAX ? 0 : vol_r;
    }
    // YM
    else
    {
        chan[ch_FM_MARKER] = vol_l;
    }
}

// PCM Command: Set Pitch
// Source: 0x3C4
void OSound_pcm_setpitch(uint8_t* chan)
{
    // PCM Channel
    if (chan[ch_FM_FLAGS] & BIT_6)
        chan[ch_PCM_PITCH] = RomLoader_read8_addr16(&Roms_z80, pos);
}

// Sets Loop Address For FM & PCM Commands.
// For example, used by Checkpoint FM sample and Crowd PCM Sample
// Source: 0x446
void OSound_set_loop_adr()
{
    pos = RomLoader_read16_addr16(&Roms_z80, pos) - 1;
}

// Set Loop Counter For FM & PCM Data
// Source: 0x454
void OSound_do_loop(uint8_t* chan)
{
    uint8_t offset = RomLoader_read8_addr16(&Roms_z80, pos++) + 0x18;
    uint8_t a = chan[offset];

    // Reload counter
    if (a == 0)
    {
        chan[offset] = RomLoader_read8_addr16(&Roms_z80, pos);
    }

    pos++;
    if (--chan[offset] != 0)
        OSound_set_loop_adr();
    else
        pos++;
}

// Write Commands to PCM Channel (For Individual Sound Effects)
// Source: 0x483
void OSound_pcm_finalize(uint8_t* chan)
{
    sound_props &= ~BIT_1; // Clear PCM sound effect triggered

    memcpy(pcm_ram + 0x40, chan_ram + CH09_CMDS1, 8); // Channel 9 Blocks
    memcpy(pcm_ram + 0xC0, chan_ram + CH09_CMDS2, 8);
    memcpy(pcm_ram + 0x50, chan_ram + CH11_CMDS1, 8); // Channel 11 Blocks
    memcpy(pcm_ram + 0xD0, chan_ram + CH11_CMDS1, 8);
    
    OSound_ym_finalize(chan);
}

// Percussion sample properties
// Sample Start Address, Sample End Address (High Byte Only), Pitch, Sample Flags
const static uint16_t PCM_PERCUSSION [] =
{
    0x17C0, 0x42, 0x84, 0xD6,
    0x302F, 0x3A, 0x84, 0xC6,
    0x0090, 0x0B, 0x84, 0xC2,
    0x00F0, 0x08, 0x84, 0xD2,
    0x49DE, 0x4C, 0x84, 0xD2,
    0x437C, 0x48, 0x84, 0xD2,
    0x29AB, 0x2E, 0x84, 0xC2,
    0x1C03, 0x28, 0x84, 0xC2,
    0x0951, 0x16, 0x84, 0xD2,
    0x3BE6, 0x7F, 0x90, 0xC2,
    0x5830, 0x5F, 0x84, 0xD2,
    0x4DA0, 0x57, 0x84, 0xD2,
    0x0C1D, 0x1B, 0x78, 0xC2,
    0x6002, 0x7F, 0x84, 0xD2,
    0x0C1D, 0x1B, 0x40, 0xC2,
};

// Play PCM Sample.
// This routine is used both for standalone samples, and samples used by the music.
//
// +0 [Word]: Start Address In Bank
// +1 [Byte]: End Address (<< 8 + 1)
// +2 [Byte]: Sample Flags
//
// Bank 0: opr10193.66
// Bank 1: opr10192.67
// Bank 2: opr10191.68
//
// Source: 0x57A
void OSound_play_pcm_index(uint8_t* chan, uint8_t cmd)
{
    if (cmd == 0)
    {
        OSound_calc_end_marker(chan);
        return;
    }

    // ------------------------------------------------------------------------
    // Initalize PCM Effects
    // ------------------------------------------------------------------------
    if (cmd >= 0xD0)
    {
        // First sample is at index 0xD0, so reset to 0
        uint16_t pcm_index = z80_adr_PCM_INFO + ((cmd - 0xD0) << 2);

        chan[ch_PCM_ADR1L] = RomLoader_read8IncP_addr16(&Roms_z80, &pcm_index);           // Wave Start Address
        chan[ch_PCM_ADR1H] = RomLoader_read8IncP_addr16(&Roms_z80, &pcm_index);
        chan[ch_PCM_ADR2]  = RomLoader_read8IncP_addr16(&Roms_z80, &pcm_index);           // Wave End Address
        chan[ch_CTRL]      = RomLoader_read8IncP_addr16(&Roms_z80, &pcm_index);           // Sample Flags
    }
    // ------------------------------------------------------------------------
    // Initalize PCM Percussion Samples
    // ------------------------------------------------------------------------
    else
    {
        uint16_t pcm_index = (cmd - 0xC0) << 2;
        OSound_w16(&chan[ch_PCM_ADR1L], PCM_PERCUSSION[pcm_index]);        // Wave Start Address
        chan[ch_PCM_ADR2]  = (uint8_t) PCM_PERCUSSION[pcm_index+1]; // Wave End Address
        chan[ch_PCM_PITCH] = (uint8_t) PCM_PERCUSSION[pcm_index+2]; // Wave Pitch
        chan[ch_CTRL]      = (uint8_t) PCM_PERCUSSION[pcm_index+3]; // Sample Flags
    }

    OSound_process_pcm(chan);
}

// Initalize Sound from ROM to RAM. Used for both PCM and YM sounds.
//
// Inputs: 
// a = Command, de = dst, hl = src
//
// Source: 0x9E8
void OSound_init_sound(uint8_t cmd, uint16_t src, uint16_t dst)
{
    int ch, i;
    uint16_t dst_backup = dst;

    command_index = cmd - 0x81;
    
    // Get offset to channel setup
    src = RomLoader_read16_addr16(&Roms_z80, src);

    // Get number of channels
    uint8_t channels = RomLoader_read8IncP_addr16(&Roms_z80, &src);

    // next_channel
    for ( ch = 0; ch < channels; ch++)
    {
        // Address of default channel setup data
        uint16_t adr = RomLoader_read16IncP_addr16(&Roms_z80, &src);

        // Copy default setup code for block of sound (14 bytes)
        for (i = 0; i < 0xE; i++)
            chan_ram[dst++] = RomLoader_read8IncP_addr16(&Roms_z80, &adr);

        // Write command byte (at position 0xE)
        chan_ram[dst++] = command_index;

        // Write zero 17 times (essentially pad out the 0x20 byte entry)
        // This empty space is updated later
        for (i = 0xF; i < CHAN_SIZE; i++)
            chan_ram[dst++] = 0;
    }
}

// Source: 0xCC3
void OSound_process_pcm(uint8_t* chan)
{
    int i;
    // ------------------------------------------------------------------------
    // PCM Music Sample (Drums)
    // ------------------------------------------------------------------------
    if (chan[ch_CTRL] & BIT_7)
    {
        const uint16_t BASE_ADR = 0xF088; // Channel 2 Base Address
        const uint16_t CHAN_SIZE = 0x10;  // Size of each channel entry (2 Channel Increment)

        uint16_t adr = BASE_ADR; 

        // Check Wave End Address
        if (chan[ch_CTRL] & BIT_2)
        {
            // get_chan_adr2:
            for (i = 0; i < 6; i++)
            {
                uint8_t channel_pair = OSound_pcm_r(adr + 6);

                // If channel active, play sample
                if ((channel_pair & 0x84) == 0x84 && (channel_pair & BIT_0) == 0)
                {
                    OSound_pcm_send_cmds(chan, adr, channel_pair);
                    return;
                }
                // d79
                adr += CHAN_SIZE; // Advance to next channel
            }
            adr = BASE_ADR;
        }
        // get_chan_adr3:
        for (i = 0; i < 6; i++)
        {
            uint8_t channel_pair = OSound_pcm_r(adr + 6);

            if (channel_pair & BIT_0)
            {
                OSound_pcm_send_cmds(chan, adr, channel_pair);
                return;
            }

            adr += CHAN_SIZE; // Advance to next channel
        }

        adr = BASE_ADR;
        // get_chan_adr4:
        for (i = 0; i < 6; i++)
        {
            uint8_t channel_pair = OSound_pcm_r(adr + 6);

            if (channel_pair & BIT_7)
            {
                OSound_pcm_send_cmds(chan, adr, channel_pair);
                return;
            }
            adr += CHAN_SIZE; // Advance to next channel
        }

        // No need to restore positioning info from stack, as stored in 'pos' variable
        OSound_calc_end_marker(chan);
    }

    // ------------------------------------------------------------------------
    // Standard PCM Samples
    // ------------------------------------------------------------------------
    else
    {
        // Mask on channel pair select
        uint8_t channel_pair = chan[ch_CTRL] & 0xC;
        // Channel selected [b]
        uint8_t selected = 0;

        // select_ch_8or10:
        if (channel_pair < 4)
        {
            // Denote PCM sound effect triggered.
            sound_props |= BIT_1;
            if (++counter1 & BIT_0)
            {
                selected = 10;      
                OSound_pcm_w(0xF0D6, channel_pair = 1); // Set flags for channel 10 (active, loop disabled)
            }
            else
            {
                selected = 8;
                OSound_pcm_w(0xF0C6, channel_pair = 1); // Set flags for channel 10 (active, loop disabled)
            }
        }
        // select_ch_1or3
        else if (channel_pair == 4)
        {
            if (++counter2 & BIT_0)
                selected = 3;      
            else
                selected = 1;
        }
        // select_ch_9or11
        else if (channel_pair == 8)
        {
            if (++counter3 & BIT_0)
                selected = 11;      
            else
                selected = 9;
        }
        // select_ch_5or7
        else
        {
            if (++counter4 & BIT_0)
                selected = 7;      
            else
                selected = 5;
        }

        // Channel Address = Channel 1 Base Address 
        uint16_t pcm_adr = 0xF080 + (selected * 8);

        OSound_pcm_send_cmds(chan, pcm_adr, channel_pair);
    }
}

// Source: 0xDA8
void OSound_pcm_send_cmds(uint8_t* chan, uint16_t pcm_adr, uint8_t channel_pair)
{
    OSound_pcm_w(pcm_adr + 0x80, channel_pair);        // Write channel pair selected value
    OSound_pcm_w(pcm_adr + 0x82, chan[ch_VOL_L]);     // Volume left
    OSound_pcm_w(pcm_adr + 0x83, chan[ch_VOL_R]);     // Volume Right
    OSound_pcm_w(pcm_adr + 0x84, chan[ch_PCM_ADR1L]); // PCM Start Address Low
    OSound_pcm_w(pcm_adr + 0x4,  chan[ch_PCM_ADR1L]); // PCM Loop Address Low
    OSound_pcm_w(pcm_adr + 0x85, chan[ch_PCM_ADR1H]); // PCM Start Address High
    OSound_pcm_w(pcm_adr + 0x5,  chan[ch_PCM_ADR1H]); // PCM Loop Address High
    OSound_pcm_w(pcm_adr + 0x86, chan[ch_PCM_ADR2]);  // PCM End Address High
    OSound_pcm_w(pcm_adr + 0x87, chan[ch_PCM_PITCH]); // PCM Pitch
    OSound_pcm_w(pcm_adr + 0x6,  chan[ch_CTRL]);      // PCM Flags

    // No need to restore positioning info from stack, as stored in 'pos' variable
    OSound_calc_end_marker(chan);
}

void OSound_fm_dotimera()
{
    #ifdef TIMER_CODE
    // Return if YM2151 is busy
    if (!(YM_read_status() & BIT_0))
        return;
    #endif
    // Set Timer A, Enable its IRQ and also reset its IRQ
    YM_write_reg(0x14, 0x15); // %10101
}

// Reset Yamaha YM2151 Chip.
// Called before inititalizing a new music track, or when z80 has just initalized and has no specific command.
// Source: 0x561
void OSound_fm_reset()
{
    uint16_t i;
    
    // Clear YM & Drum Channels in RAM (0xF820 - 0xF9DF)
    for (i = channel_YM1; i < channel_PCM_FX1; i++)
        chan_ram[i] = 0;

    OSound_fm_write_block(0, z80_adr_YM_INIT_CMDS, 0);
}

// Write to FM Register With Check
// Source: 0xA70
void OSound_fm_write_reg_c(uint8_t ix0, uint8_t reg, uint8_t value)
{
    // Is corresponding music channel enabled?
    if (ix0 & BIT_2)
        return;

    OSound_fm_write_reg(reg, value);
}

// Write to FM Register
// Source: 0xA75
void OSound_fm_write_reg(uint8_t reg, uint8_t value)
{
    #ifdef TIMER_CODE
    // Return if YM2151 is busy
    if (YM_read_status() & BIT_7)
        return;
    #endif
    YM_write_reg(reg, value);
}

// Write Block of FM Data From ROM
//
// Inputs:
//
// adr  = Address of commands and data to write to FM
// chan = Register offset
//
// Format:
// 2 = End of data block
// 3 = Next word specifies next address in memory
//
// Source: 0xA84
void OSound_fm_write_block(uint8_t ix0, uint16_t adr, uint8_t chan)
{
    uint8_t cmd = RomLoader_read8IncP_addr16(&Roms_z80, &adr);

    // Return if end of data block
    if (cmd == 2) return;

    // Next word specifies next address in memory
    if (cmd == 3)
    {
        adr = RomLoader_read16_addr16(&Roms_z80, adr);
    }
    else
    {
        uint8_t reg = cmd + chan;
        uint8_t val = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
        OSound_fm_write_reg_c(0, reg, val);
    }

    OSound_fm_write_block(ix0, adr, chan);
}

// Write level info to YM Channels
// Source: 0x91A
void OSound_ym_set_levels()
{
    uint16_t i;
    // Clear YM & Drum Channels in RAM (0xF820 - 0xF9DF)
    for (i = channel_YM1; i < channel_PCM_FX1; i++)
        chan_ram[i] = 0;

    // FM Sound Effects: Write fewer levels
    uint8_t entries = (chan_ram[channel_YM_FX1] & BIT_7) ? 28 : 32;
    uint16_t adr = z80_adr_YM_LEVEL_CMDS1;

    // Write Level Info
    for (i = 0; i < entries; i++)
    {
        uint8_t reg = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
        uint8_t val = RomLoader_read8IncP_addr16(&Roms_z80, &adr);
        OSound_fm_write_reg(reg, val);
    }
}

const static uint16_t FM_DATA_TABLE[] =
{
    z80_adr_DATA_BREEZE,
    z80_adr_DATA_SPLASH,
    0,
    z80_adr_DATA_COININ,
    z80_adr_DATA_MAGICAL,
    z80_adr_DATA_CHECKPOINT,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    z80_adr_DATA_SIGNAL1,
    z80_adr_DATA_SIGNAL2,
    0,
    0,
    0,
    z80_adr_DATA_BEEP1,
    z80_adr_DATA_UFO,
    z80_adr_DATA_BEEP2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    z80_adr_DATA_LASTWAVE,
};

// This is called first, when setting up YM Samples.
// The global 'pos' variable stores the location of the block table.
// Source: 0x515
void OSound_ym_set_block(uint8_t* chan)
{
    // Set block address
    chan[ch_FM_BLOCK] = RomLoader_read8_addr16(&Roms_z80, pos);
    
    if (!chan[ch_FM_BLOCK])
        return;

    uint16_t adr = OSound_ym_lookup_data(chan[ch_COMMAND], 3, chan[ch_FM_BLOCK]); // Use Routine 3.
    OSound_fm_write_block(chan[ch_FLAGS], adr, chan[ch_FM_FLAGS] & 7);
}

// Source: 0xAAA
uint16_t OSound_ym_lookup_data(uint8_t cmd, uint8_t offset, uint8_t block)
{
    block = (block - 1) << 1;
    
    // Address of data for FM routine
    uint16_t adr = RomLoader_read16_addr16(&Roms_z80, (uint16_t) (FM_DATA_TABLE[cmd] + (offset << 1)));
    return RomLoader_read16_addr16(&Roms_z80, (uint16_t) (adr + block));
}

// "Connect" Channels To Play Out of Left/Right Speakers.
// Source: 0x534
void OSound_ym_set_connect(uint8_t* chan, uint8_t pan)
{
    uint8_t block = chan[ch_FM_BLOCK];                          // FM Routine To Choose from Data Block
    uint16_t adr = OSound_ym_lookup_data(chan[ch_COMMAND], 3, block);  // Send Block of FM Commands
    adr += 0x33;

    // c = Channel Control Register (0x20 - 0x27)
    uint8_t chan_ctrl_reg = (chan[ch_FM_FLAGS] & 7) + 0x20;

    // Register Value
    uint8_t reg_value = (RomLoader_read8_addr16(&Roms_z80, adr) & 0x3F) | pan;

    pos--;

    // Write Register
    OSound_fm_write_reg_c(chan[ch_FLAGS], chan_ctrl_reg, reg_value);
}

// iy = chan_ram
//
// Source: 0x4BF
void OSound_ym_finalize(uint8_t* chan)
{
    // Get channel number
    uint8_t chan_index = chan[ch_FM_FLAGS] & 7;

    // Write block of release commands
    OSound_fm_write_block(chan[ch_FLAGS], z80_adr_YM_RELEASE_RATE, chan_index);
    
    // Register: KEY ON Turns on and off output from each operator of each channel. (Disable in this case)
    OSound_fm_write_reg(0x8, chan_index);
    // Register: noise mode enable, noise period (Disable in this case)
    OSound_fm_write_reg(0xf, 0);

    chan[ch_FLAGS] = 0;

    // Check whether YM channel is also playing music
    if (chanid_prev < channel_MAP1)
    {
        // pop and return
        return;
    }
    
    *(chan -= 0x2C0); // = corresponding music channel

    // Return if no sound playing on corresponding channel
    if (!(chan[ch_FLAGS] & BIT_7))
        return;

    // ------------------------------------------------------------------------
    // FM Sound effect is playing & Music is playing simultaneously
    // ------------------------------------------------------------------------

    chan[ch_FLAGS] &= ~BIT_2;

    // Write remaining FM Data block, if specified
    uint8_t block = chan[ch_FM_BLOCK];

    if (!block)
        return;

    uint16_t adr = OSound_ym_lookup_data(chan[ch_COMMAND], 3, block);
    OSound_fm_write_block(chan[ch_FLAGS], adr, chan[ch_FM_FLAGS] & 7);
}

// Use Phase and Amplitude Modulation Sensitivity Table
// Enabled with FM_PHASETBL flag.
// Source: 0x1D6
void OSound_read_mod_table(uint8_t* chan)
{
    uint16_t adr = OSound_ym_lookup_data(chan[ch_COMMAND], 2, chan[ch_FM_PHASETBL]); // Use Routine 2.

    while (TRUE)
    {
        uint16_t offset = chan[ch_FM_PHASEOFF];
        uint8_t table_entry = RomLoader_read8_addr16(&Roms_z80, (uint16_t) (adr + offset));

        // Reset table position
        if (table_entry == 0xFD)
            chan[ch_FM_PHASEOFF] = 0;
        // Decrement table position
        else if (table_entry == 0xFE)
            chan[ch_FM_PHASEOFF]--;
        // Unused special case
        else if (table_entry == 0xFC)
        {
            #ifdef UNUSED_WARNINGS
            // Missing code here
            fprintf(stderr, "read_mod_table: table_entry 0xFC not supported\n");
            #endif
        }
        // Increment table position
        else
        {
            chan[ch_FM_PHASEOFF]++;
            uint8_t carry = (table_entry < 0xFC) ? 2 : 0;
            // rotate table_entry left through 9-bits twice
            chan[ch_FM_PHASE_AMP] = ((table_entry << 2) + carry) + ((table_entry & 0x80) >> 7);
            return;
        }
    }
}

// 1/ bc = Read next value in sequence (from address in de) 
// 2/ Store value on stack
// 3/ Decrement internal pointer 
// 4/ hl =  Address in block + bc
// 5/ Increment and store next 'de' value in sequence within 0x20 block
// Source: 0x418
void OSound_write_seq_adr(uint8_t* chan)
{
    uint16_t value = RomLoader_read16_addr16(&Roms_z80, pos);
    pos++;

    chan[ch_MEM_OFFSET]--;
    uint8_t offset = chan[ch_MEM_OFFSET];
    chan[ch_MEM_OFFSET]--;

    chan[offset]   = pos >> 8;
    chan[offset-1] = pos & 0xFF;

    pos = value - 1;
}

// ----------------------------------------------------------------------------
//                                ENGINE TONE CODE 
// ----------------------------------------------------------------------------

// Process Ferrari Engine Tone & Traffic Sound Effects
//
// Original addresses used: 
// 0xFC00: Engine Channel - Player's Car
// 0xFC20: Engine Channel - Traffic 1
// 0xFC40: Engine Channel - Traffic 2
// 0xFC60: Engine Channel - Traffic 3
// 0xFC80: Engine Channel - Traffic 4
//
// counter = Increments every tick (0 - 0xFF)
//
// Source: 0x7501
void OSound_engine_process()
{
    // DEBUG
    // bpset 7501,1,{b@0xf801 = 0xe; b@0xf802 = 0xf1; b@f803 = 0x3f;}
    // engine_data[sound_ENGINE_PITCH_H] = 0xE;
    // engine_data[sound_ENGINE_PITCH_L] = 0xF1;
    // engine_data[sound_ENGINE_VOL]     = 0x3F;
    // END DEBUG

    // Return 1 in 2 times when this routine is called
    if ((++engine_counter & 1) == 0)
        return;

    uint16_t ix = 0;                    // PCM Channel RAM Address
    uint16_t iy = channel_ENGINE_CH1;  // Internal Channel RAM Address

    for (engine_channel = 6; engine_channel > 0; engine_channel--)
    {
        OSound_engine_process_chan(&chan_ram[iy], &pcm_ram[ix]);
        ix += 0x10;
        iy += CHAN_SIZE;
    }
}

// Source: 0x7531
void OSound_engine_process_chan(uint8_t* chan, uint8_t* pcm)
{
    // Return if PCM Sample Being Played On Channel
    if (engine_channel < 3)
    {
        if (sound_props & BIT_1)
            return;
    }

    // Read Engine Data that has been sent by 68K CPU
    OSound_engine_read_data(chan, pcm);

    // ------------------------------------------------------------------------
    // Car is stationary, do rev effect at start line.
    // This bit is set whenever the music start is triggered
    // ------------------------------------------------------------------------
    if (sound_props & BIT_0)
    {
        // 0x7663
        uint16_t revs = OSound_r16(pcm); // Read Revs/Pitch which has just been stored by engine_read_data

        // No revs, mute engine channel and get out of here
        if (revs == 0)
        {
            OSound_engine_mute_channel(chan, pcm, TRUE);
            return;
        }
        // 0x766E
        // Do High Pitched Rev Sound When Car Is At Starting Line
        // Set Volume & Pitch, Then Return
        if (revs >= 0xFA)
        {
            // 0x7682
            // Return if channel already active
            if (!(pcm[0x86] & BIT_0))
                return;

            if (engine_channel < 3)
            {
                pcm[2] = 0x20; // l
                pcm[3] = 0;    // r
                pcm[7] = 0x41; // pitch
            }
            else if (engine_channel < 5)
            {
                pcm[2] = 0x10; // l
                pcm[3] = 0x10; // r
                pcm[7] = 0x42; // pitch
            }
            else
            {
                pcm[2] = 0x20; // l
                pcm[3] = 0;    // r
                pcm[7] = 0x40; // pitch
            }

            pcm[4] = pcm[0x84] = 0;    // start/loop low
            pcm[5] = pcm[0x85] = 0x36; // start/loop high
            pcm[6] = 0x55;             // end address high
            pcm[0x86] = 2;             // Flags: Enable loop, set active

            return;
        }
        // Some code relating to 0xFD08 that I don't think is used
    }
    // 0x754A
    if (engine_channel & BIT_0)
    {
        OSound_unk78c7(chan, pcm);
    }

    // Check engine volume and mute channel if disabled
    if (!chan[ch_engines_VOL0])
    {
        OSound_engine_mute_channel(chan, pcm, TRUE);
        return;
    }

    // Set Engine Volume
    if (chan[ch_engines_VOL0] == chan[ch_engines_VOL1])
    {
        // Denote volume already set
        chan[ch_engines_FLAGS] |= BIT_1; 
    }
    else
    {
        chan[ch_engines_FLAGS] &= ~BIT_1; 
        chan[ch_engines_VOL1] = chan[ch_engines_VOL0];
    }

    // 0x755C
    // Check we have some revs
    uint16_t revs = OSound_r16(pcm);
    if (revs == 0)
    {
        OSound_engine_mute_channel(chan, pcm, TRUE);
        return;
    }

    // Rev Change Setup
    // 0x774E routine rolled in here
    uint16_t old_revs = OSound_r16(pcm + 0x80);

    // Revs Unchanged
    if (revs == old_revs)
    {
        chan[ch_engines_FLAGS] |= BIT_0; // denotes start address / end address has been set
    }
    // Revs Changed
    else 
    {
        if (revs - old_revs < 0)
            chan[ch_engines_FLAGS] &= ~BIT_2; // loop enabled
        else
            chan[ch_engines_FLAGS] |= BIT_2;  // loop disabled

        chan[ch_engines_FLAGS] &= ~BIT_0;     // Start end address not set
        OSound_w16(pcm + 0x80, revs);                 // Write new revs value
    }

    // 0x756A
    chan[ch_engines_ACTIVE] &= ~BIT_0; // Mute

    // Return if addresses have already been set
    if ((chan[ch_engines_FLAGS] & BIT_0) && chan[ch_engines_FLAGS] & BIT_1)
        return;

    // 0x757A
    // PLAYER'S CAR
    if (engine_channel >= 5)
    {
        int16_t off = OSound_r16(pcm) - 0x30;

        if (off >= 0)
        {
            if (chan[ch_engines_FLAGS] & BIT_4)
            {
                chan[ch_engines_FLAGS] &= ~BIT_3; // Loop Address Not Set
                chan[ch_engines_FLAGS] &= ~BIT_4;
            }

            OSound_ferrari_vol_pan(chan, pcm);
            return;
        }
        else
        {
            if (!(chan[ch_engines_FLAGS] & BIT_4))
            {
                chan[ch_engines_FLAGS] &= ~BIT_3; // Loop Address Not Set
                chan[ch_engines_FLAGS] |=  BIT_4;
            }
        }
    }
    // 0x75B2
    uint16_t engine_pos = OSound_engine_get_table_adr(chan, pcm); // hl
    
    // Mute Engine Channel
    if (chan[ch_engines_FLAGS] & BIT_5)
    {
        OSound_engine_mute_channel(chan, pcm, FALSE);
        return;
    }

    // 0x75bc Has Start Address Been Set Already?
    // Used on start line
    if (chan[ch_engines_FLAGS] & BIT_0)
    {
        engine_pos += 2;
    }
    else
    {
        uint16_t start_adr = OSound_engine_set_adr(&engine_pos, chan, pcm); // Set Start Address
        OSound_engine_set_adr_end(&engine_pos, start_adr, chan, pcm);       // Set End Address
    }

    OSound_vol_thicken(&engine_pos, chan, pcm); // Thicken engine effect by panning left/right dependent on channel.
    OSound_engine_set_pitch(&engine_pos, pcm);  // Set Engine Pitch from lookup table specified by hl
    pcm[0x86] = 0;                      // Set Active & Loop Enabled
}

// Only called for odd number channels
// I've not really worked out what this does yet
// Source: 0x78C7
void OSound_unk78c7(uint8_t* chan, uint8_t* pcm)
{
    uint16_t adr; // Channel address in RAM

    if (engine_channel == 1)
    {
        adr = 0xFD10;
    }
    else if (engine_channel == 3)
    {
        adr = 0xFD30;
    }
    else
    {
        adr = 0xFD50;
    }

    // STORE: Calculate offset into Channel Block To Store Data At
    uint16_t adr_offset = adr + (chan[ch_engines_OFFSET] * 3);
    adr_offset &= 0x7FF;
    chan_ram[adr_offset++] = pcm[0x0];               // Copy Engine Pitch Low
    chan_ram[adr_offset++] = pcm[0x1];               // Copy Engine Pitch High
    chan_ram[adr_offset++] = chan[ch_engines_VOL0]; // Copy Engine Volume

    // Wrap around block of three entries
    if (++chan[ch_engines_OFFSET] >= 8)
    {
        chan[ch_engines_OFFSET] = 0;
        adr_offset = adr & 0x7FF;
    }

    // RESTORE: 7915
    pcm[0x0] = chan_ram[adr_offset++];
    pcm[0x1] = chan_ram[adr_offset++];
    chan[ch_engines_VOL0] = chan_ram[adr_offset++];
}

// Source: 0x75DA
void OSound_ferrari_vol_pan(uint8_t* chan, uint8_t* pcm)
{
    // Adjust Engine Volume and write to new memory area (0x6)
    OSound_engine_adjust_volume(chan);

    // Set Pitch Table Details
    int16_t pitch_table_index = OSound_r16(pcm + 0x80) - 0x30;

    if (pitch_table_index < 0)
    {
        OSound_engine_mute_channel(chan, pcm, FALSE);
        return;
    }

    OSound_w16(chan + ch_engines_PITCH_L, pitch_table_index);

    // Set PCM Sample Addresses
    uint16_t pos = z80_adr_ENGINE_ADR_TABLE;
    OSound_engine_set_adr(&pos, chan, pcm);

    // Set PCM Sample End Address
    pcm[0x6] = RomLoader_read8_addr16(&Roms_z80, ++pos);

    // Set Volume Pan
    OSound_engine_set_pan(&pos, chan, pcm);

    // Set Pitch
    pos = z80_adr_ENGINE_ADR_TABLE + 4; // Set position to pitch offset
    uint16_t pitch = RomLoader_read8_addr16(&Roms_z80, pos); // bc
    pitch += OSound_r16(chan + ch_engines_PITCH_L) >> 1;
    if (pitch > 0xFF) pitch = 0xFF;

    // Tweak pitch slightly based on channel id
    if (engine_channel & BIT_0)
        pitch -= 2;

    pcm[0x7] = (uint8_t) pitch;
    pcm[0x86] = 0x10; // Set channel active and enabled
}

// Set Table Index For Engine Sample Start / End Addresses
// Table starts at ENGINE_ADR_TABLE offset in ROM.
// Source: 0x7819
uint16_t OSound_engine_get_table_adr(uint8_t* chan, uint8_t* pcm)
{
    int16_t off = OSound_r16(pcm + 0x80) - 0x52;
    int16_t table_offset;

    if (off < 0)
    {
        chan[ch_engines_FLAGS] &= ~BIT_5; // Unmute Engine Sounds
        table_offset = OSound_r16(pcm + 0x80);
        OSound_w16(pcm + 0x82, 0);
    }
    else
    {
        chan[ch_engines_FLAGS] |= BIT_5; // Mute Engine Sounds
        table_offset = 1;
        OSound_w16(pcm + 0x82, off);
    }
    // get_adr:
    table_offset--;

    const static uint8_t ENTRY = 5; // bytes per entry
    return (z80_adr_ENGINE_ADR_TABLE + ENTRY) + (table_offset * ENTRY); // table has 54 entries
}

// Setup engine addresses from table (START, LOOP)
// Source: 0x77AD

//bpset 77b0,1,{printf "start adr:%02x pos:=%02x",bc, hl; g}
uint16_t OSound_engine_set_adr(uint16_t* pos, uint8_t* chan, uint8_t* pcm)
{
    uint16_t start_adr = RomLoader_read16_addr16(&Roms_z80, (*pos)++);
    OSound_w16(pcm + 0x4, start_adr); // Set Wave Start Address

    // TRAFFIC
    if (engine_channel < 5)
    {
        if (chan[ch_engines_FLAGS] & BIT_5) // Mute
        {
            if (chan[ch_engines_FLAGS] & BIT_6)
            {
                chan[ch_engines_FLAGS] |= BIT_6;               
                chan[ch_engines_FLAGS] |= BIT_3; // Denote loop address set                
                OSound_w16(pcm + 0x84, start_adr);       // Set default loop address to start address
                return start_adr;
            }
        }
        else
        {
            chan[ch_engines_FLAGS] &= ~BIT_6;
        }
    }

    // Return if loop address already set
    if (chan[ch_engines_FLAGS] & BIT_3)
        return start_adr;

    chan[ch_engines_FLAGS] |= BIT_3; // Denote loop address set  
    OSound_w16(pcm + 0x84, start_adr);       // Set default loop address to start address
    return start_adr;
}

// Source: 0x7853
void OSound_engine_set_adr_end(uint16_t* pos, uint16_t loop_adr, uint8_t* chan, uint8_t* pcm)
{
    // Set wave end address from table
    pcm[0x6] = RomLoader_read8_addr16(&Roms_z80, ++(*pos));

    // Loop Disabled
    if (chan[ch_engines_FLAGS] & BIT_2)
        return;

    // Loop Address >= End Address
    if (pcm[0x6] >= pcm[0x85])
        return;

    // Set loop address
    OSound_w16(pcm + 0x84, loop_adr);
}

// Thicken engine effect by panning left/right dependent on channel.
// Source: 0x77EA
void OSound_vol_thicken(uint16_t* pos, uint8_t* chan, uint8_t* pcm)
{
    (*pos)++; // Address of volume multiplier

    // Odd Channels: Pan Left
    if (engine_channel & BIT_0)
    {
        pcm[0x2] = pcm[0x82] & BIT_5 ? 0 : OSound_get_adjusted_vol(pos, chan); // left (if enabled)
        pcm[0x3] = 0; // right
    }
    // Even Channels: Pan Right
    else
    {
        pcm[0x2] = 0; // left
        pcm[0x3] = pcm[0x83] & BIT_5 ? 0 : OSound_get_adjusted_vol(pos, chan); // right (if enabled)
    }
}

// Get Adjusted Volume
// Source: 0x78A7
uint8_t OSound_get_adjusted_vol(uint16_t* pos, uint8_t* chan)
{
    uint8_t multiply =  RomLoader_read8IncP_addr16(&Roms_z80, pos);
    uint16_t vol = (chan[ch_engines_VOL1] * multiply) >> 6;

    if (vol > 0x3F)
        vol = 0x3F;

    return (uint8_t) vol;
}

// bpset 7877,1,{printf "bc=%02x hl=%02x",bc, hl; g}
// Set Engine Pitch From Table
// Source: 0x7870
void OSound_engine_set_pitch(uint16_t* pos, uint8_t* pcm)
{
    (*pos)++; // Increment to pitch entry in table

    uint16_t bc = OSound_r16(pcm + 0x82);
    bc >>= 2;

    if (bc & 0xFF00)
        bc = (bc & 0xFF00) | 0xFF;

    uint16_t pitch = RomLoader_read8IncP_addr16(&Roms_z80, pos);

    //std::cout << std::hex << (*pos) << std::endl;

    // Read pitch from table
    if (bc)
    {
        pitch += (bc & 0xFF);
        if (pitch > 0xFF)
            pitch = 0xFC;
    }

    // Adjust the pitch slightly dependent on the channel selected
    if (engine_channel & BIT_0)
        pcm[0x7] = (uint8_t) pitch;
    else
        pcm[0x7] = (uint8_t) pitch + 3;
}

// Mute an engine channel
// Source: 0x7639
void OSound_engine_mute_channel(uint8_t* chan, uint8_t* pcm, Boolean do_check)
{
    // Return if already muted
    if (do_check && (chan[ch_engines_ACTIVE] & BIT_0))
        return;

    // Denote channel muted
    chan[ch_engines_ACTIVE] |= BIT_0;

    pcm[0x02] = 0;      // Clear volume left
    pcm[0x03] = 0;      // Clear volume right
    pcm[0x07] = 0;      // Clear pitch
    pcm[0x86] |= BIT_0; // Denote not active

    // Clear some stuff
    chan[ch_engines_VOL0]    = 0;
    chan[ch_engines_VOL1]    = 0;
    chan[ch_engines_FLAGS]   = 0;
    chan[ch_engines_PITCH_L] = 0;
    chan[ch_engines_PITCH_H] = 0;
    chan[ch_engines_VOL6]    = 0;
}

// Adjust engine volume and write to new memory area
// Source: 0x76D7
void OSound_engine_adjust_volume(uint8_t* chan)
{
    uint16_t vol = (chan[ch_engines_VOL1] * 0x18) >> 6;

    if (vol > 0x3F)
        vol = 0x3F;

    chan[ch_engines_VOL6] = (uint8_t) vol;
}   

// Set engine pan. 
// Adjust Volume and write to new memory area
// Also write to ix (PCM Channel RAM)
// Source: 0x76FD
void OSound_engine_set_pan(uint16_t* pos, uint8_t* chan, uint8_t* pcm)
{
    uint16_t pitch = OSound_r16(chan + ch_engines_PITCH_L) >> 1;
    pitch += RomLoader_read8_addr16(&Roms_z80, ++(*pos));

    uint16_t vol = (chan[ch_engines_VOL1] * pitch) >> 6;

    if (vol > 0x3F)
        vol = 0x3F;

    if (vol >= chan[ch_engines_VOL6])
        vol = chan[ch_engines_VOL6];

    // Pan Left
    if (engine_channel & BIT_0)
    {
        pcm[0x2] = (uint8_t) vol;      // left
        pcm[0x3] = (uint8_t) vol >> 1; // right
    }
    // Pan Right
    else
    {
        pcm[0x2] = (uint8_t) vol >> 1; // left
        pcm[0x3] = (uint8_t) vol;      // right;
    }
}

// Read Engine Data & Store the engine pitch and volume to PCM Channel RAM
// Source: 0x778D
void OSound_engine_read_data(uint8_t* chan, uint8_t* pcm)
{
    uint16_t pitch = (OSound_engine_data[sound_ENGINE_PITCH_H] << 8) + OSound_engine_data[sound_ENGINE_PITCH_L];

    pitch = (pitch >> 5) & 0x1FF;
    
    // Store pitch in scratch space of channel (due to mirroring this wraps round to 0x00 in the channel)
    pcm[0x0] = pitch & 0xFF;
    pcm[0x1] = (pitch >> 8) & 0xFF;
    chan[ch_engines_VOL0] = OSound_engine_data[sound_ENGINE_VOL];
}

// ----------------------------------------------------------------------------
//                               PASSING TRAFFIC FX 
// ----------------------------------------------------------------------------

// should be 0x9b 0x9b 0xe3 0xe3 for starting traffic

// Process Passing Traffic Channels
// Source: 0x7AFB
void OSound_traffic_process()
{
    if ((engine_counter & 1) == 0)
        return;

    uint16_t pcm_adr = 0x60; // Channel 13: PCM Channel RAM Address

    // Iterate PCM Channels 13 to 16
    for (engine_channel = 4; engine_channel > 0; engine_channel--)
    {
        OSound_traffic_process_chan(&pcm_ram[pcm_adr]);
        pcm_adr += 0x8; // Advance to next channel
    }

}

// Process Single Channel Of Traffic Sounds
// Source: 0x7B1F
void OSound_traffic_process_chan(uint8_t* pcm)
{
    // No slide/pitch reduction applied yet
    if (!(pcm[0x82] & BIT_4))
    {
        OSound_traffic_read_data(pcm); // Read Traffic Data that has been sent by 68K CPU
        
        uint8_t vol = pcm[0x00];
        
        // vol on
        if (vol)
        {
            OSound_traffic_note_changes(vol, pcm); // Record changes to traffic volume and panning
            uint8_t flags = pcm[0x82];

            // Change in Volume or Panning: Set volume, panning & pitch based on distance of traffic.
            if (!(flags & BIT_0) || !(flags & BIT_1))
            {
                OSound_traffic_process_entry(pcm);
                return;
            }
            // Return if start/end position of wave is already setup
            else if (flags & BIT_2)
                return;

            // Set volume, panning & pitch based on distance of traffic.
            OSound_traffic_process_entry(pcm); 
            return;
        }
        // vol off: decrease pitch
        else
        {
            // Check whether to instantly disable channel
            if (!(pcm[0x82] & BIT_3))
            {
                OSound_traffic_disable(pcm);
                return;
            }

            pcm[0x82] |= BIT_4; // Denote pitch reduction

            // Adjust pitch
            if (pcm[0x07] < 0x81)
                pcm[0x07] -= 4;
            else
                pcm[0x07] -= 6;
        }
    }

    // Reduce Volume Right Channel
    if (pcm[0x03])
        pcm[0x03]--;

    // Reduce Volume Left Channel
    if (pcm[0x02])
        pcm[0x02]--;

    // Once both channels have been reduced to zero, disable the sample completely
    if (!pcm[0x02] && !pcm[0x03])
        OSound_traffic_disable(pcm);
}

const uint8_t TRAFFIC_PITCH_H[] = { 0, 2, 4, 4, 0, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8 };

// Process traffic entry. Set volume, panning & pitch based on distance of traffic.
// Source: 0x7B82
void OSound_traffic_process_entry(uint8_t* pcm)
{
    // Wave Start/End Address has not been setup yet
    if (!(pcm[0x82] & BIT_2))
    {
        pcm[0x82] |= BIT_2;           // Denote set       
        pcm[0x04] = pcm[0x84] = 0x82; // Set Wave Start & Loop Addresses (Word)
        pcm[0x05] = pcm[0x85] = 0x00;
        pcm[0x06] = 0x6;              // Set Wave End    
    }
    // do_pan_vol
    OSound_traffic_set_vol(pcm); // Set Traffic Volume Multiplier
    OSound_traffic_set_pan(pcm); // Set Traffic Volume / Panning on each channel

    int8_t vol_boost = pcm[0x80] - 0x16;  
    uint8_t pitch = 0;

    if (vol_boost >= 0)
        pitch = TRAFFIC_PITCH_H[vol_boost];

    pitch += (engine_channel & 1) ? 0x60 : 0x80;
    
    // set_pitch2:
    pcm[0x07] = pitch; // Set Pitch
    pcm[0x86] = 0x10;  // Set Active & Enabled

    //std::cout << std::hex << (uint16_t) pitch << std::endl;
}

// Disable Traffic PCM Channel
// Source: 0x7BDC
void OSound_traffic_disable(uint8_t* pcm)
{
    pcm[0x86] |= BIT_0; // Disable sound
    pcm[0x82] = 0;      // Clear Flags
    pcm[0x02] = 0;      // Clear Volume Left
    pcm[0x03] = 0;      // Clear Volume Right
    pcm[0x07] = 0;      // Clear Delta (Pitch)
    pcm[0x00] = 0;      // Clear New Vol Index
    pcm[0x80] = 0;      // Clear Old Vol Index
    pcm[0x01] = 0;      // Clear New Pan Index
    pcm[0x81] = 0;      // Clear Old Pan Index
}

// Set Traffic Volume Multiplier
// Source: 0x7C28
void OSound_traffic_set_vol(uint8_t* pcm)
{
    // Return if volume index is not set
    uint8_t vol_entry = pcm[0x80];

    if (!vol_entry)
        return;

    uint16_t multiply = z80_adr_TRAFFIC_VOL_MULTIPLY + vol_entry - 1;

    // Set traffic volume multiplier
    pcm[0x83] = RomLoader_read8_addr16(&Roms_z80, multiply);

    if (pcm[0x83] < 0x10)
        pcm[0x82] &= ~BIT_3; // Disable Traffic Sound
    else
        pcm[0x82] |= BIT_3;  // Enable Traffic Sound
}

// Traffic Panning Indexes are as follows:
// 0 = Both
// 1 = Pan Left
// 2 = Pan Left More
// 3 = Hard Left Pan
//
// 5 = Both
// 6 = Hard Pan Right
// 7 = Pan Right More
// 8 = Pan Right

const uint8_t TRAFFIC_PANNING[] = 
{ 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x08, 0x0D, // Right Channel
    0x10, 0x0D, 0x08, 0x00, 0x10, 0x10, 0x10, 0x10  // Left Channel
};

// Set Traffic Panning On Channel From Table
// Source: 0x7BFA
void OSound_traffic_set_pan(uint8_t* pcm)
{
    pcm[0x03] = OSound_traffic_get_vol(pcm[0x81] + 0, pcm); // Set Volume Right
    pcm[0x02] = OSound_traffic_get_vol(pcm[0x81] + 8, pcm); // Set Volume Left
}

// Read Traffic Volume Value from Table And Multiply Appropriately
// Source: 0x7C16
uint8_t OSound_traffic_get_vol(uint16_t pos, uint8_t* pcm)
{
    // return volume from table * multiplier
    return (TRAFFIC_PANNING[pos] * pcm[0x83]) >> 4;
}

// Has Traffic Volume or Pitch changed?
// Set relevant flags when it has to denote the fact.
// Source: 0x7C48
void OSound_traffic_note_changes(uint8_t new_vol, uint8_t* pcm)
{
    // Denote no volume entry change
    if (new_vol == pcm[0x80])
        pcm[0x82] |= BIT_0;
    // Record entry change
    else
    {
        pcm[0x82] &= ~BIT_0;
        pcm[0x80] = new_vol;
    }

    // Denote no pan entry change
    if (pcm[0x01] == pcm[0x81])
        pcm[0x82] |= BIT_1;
    else
    {
        pcm[0x82] &= ~BIT_1;
        pcm[0x81] = pcm[0x01];
    }
}

// Read Traffic Data that has been sent by 68K CPU
void OSound_traffic_read_data(uint8_t* pcm)
{
    // Get volume of traffic for channel
    uint8_t vol = OSound_engine_data[sound_ENGINE_VOL + engine_channel];
    //std::cout << std::hex << "ch: " << (int16_t) engine_channel << " vol: " << (int16_t) vol << std::endl;    
    pcm[0x01] = vol & 7;  // Put bottom 3 bits in 01    (pan entry)
    pcm[0x00] = vol >> 3; // And remaining 5 bits in 00 (used as vol entry)
}
