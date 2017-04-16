/***************************************************************************
    SDL Audio Code.
    
    This is the SDL specific audio code.
    If porting to a non-SDL platform, you would need to replace this class.
    
    It takes the output from the PCM and YM chips, mixes them and then
    outputs appropriately.
    
    In order to achieve seamless audio, when audio is enabled the framerate
    is adjusted to essentially sync the video to the audio output.
    
    This is based upon code from the Atari800 emulator project.
    Copyright (c) 1998-2008 Atari800 development team
***************************************************************************/

#include <SDL.h>
#include "sdl/audio.h"
#include "frontend/config.h" // fps
#include "engine/audio/OSoundInt.h"



#ifdef COMPILE_SOUND_CODE

Boolean Audio_sound_enabled;

// Sample Rate. Can't be changed easily for now, due to lack of SDL resampling.
const uint32_t FREQ = 44100;

// Stereo. Could be changed, requires some recoding.
const uint32_t CHANNELS = 2;

// 16-Bit Audio Output. Could be changed, requires some recoding.
const uint32_t BITS = 16;

// Low value  = Responsiveness, chance of drop out.
// High value = Laggy, less chance of drop out.
const uint32_t SAMPLES  = 1024;

// Latency (in ms) and thus target buffer size
static int SND_DELAY = 20;

// allowed "spread" between too many and too few samples in the buffer (ms)
static int SND_SPREAD = 7;
    
// Buffer used to mix PCM and YM channels together
uint16_t* Audio_mix_buffer;

struct wav_t Audio_wavfile;

// Estimated gap
int Audio_gap_est;

// Cumulative audio difference
double Audio_avg_gap;

void Audio_clear_buffers();
void Audio_pause_audio();
void Audio_resume_audio();


/* ----------------------------------------------------------------------------
   SDL Sound Implementation & Callback Function
   ----------------------------------------------------------------------------*/

// Note that these variables are accessed by two separate threads.
uint8_t* dsp_buffer;
static int dsp_buffer_bytes;
static int dsp_write_pos;
static int dsp_read_pos;
static int callbacktick;     // tick at which callback occured
static int bytes_per_sample; // Number of bytes per sample entry (usually 4 bytes if stereo and 16-bit sound)

// SDL Audio Callback Function
extern void fill_audio(void *udata, Uint8 *stream, int len);

// ----------------------------------------------------------------------------

void Audio_init()
{
    if (Config_sound.enabled)
        Audio_start_audio();
}

void Audio_start_audio()
{
    if (!Audio_sound_enabled)
    {
        if(SDL_Init(SDL_INIT_AUDIO) == -1) 
        {
            fprintf(stderr, "Error initalizing audio: %d\n", SDL_GetError());        
            return;
        }

        // SDL Audio Properties
        SDL_AudioSpec desired, obtained;

        desired.freq     = FREQ;
        desired.format   = AUDIO_S16SYS;
        desired.channels = CHANNELS;
        desired.samples  = SAMPLES;
        desired.callback = fill_audio;
        desired.userdata = NULL;

        if (SDL_OpenAudio(&desired, &obtained) == -1)
        {
            fprintf(stderr, "Error initalizing audio: %d\n", SDL_GetError());
            return;
        }

        if (desired.samples != obtained.samples)
        {
            fprintf(stderr, "Error initalizing audio: ample rate not supported.\n");
            return;
        }

        bytes_per_sample = CHANNELS * (BITS / 8);

        // Start Audio
        Audio_sound_enabled = TRUE;

        // how many fragments in the dsp buffer
        const int DSP_BUFFER_FRAGS = 5;
        int specified_delay_samps = (FREQ * SND_DELAY) / 1000;
        int dsp_buffer_samps = SAMPLES * DSP_BUFFER_FRAGS + specified_delay_samps;
        dsp_buffer_bytes = CHANNELS * dsp_buffer_samps * (BITS / 8);
        dsp_buffer = (uint8_t*)malloc(dsp_buffer_bytes);

        // Create Buffer For Mixing
        uint16_t buffer_size = (FREQ / Config_fps) * CHANNELS;
        Audio_mix_buffer = (uint16_t*)malloc(buffer_size * sizeof(uint16_t));

        Audio_clear_buffers();
        Audio_clear_wav();

        SDL_PauseAudio(0);
    }
}

void Audio_clear_buffers()
{
    dsp_read_pos  = 0;
    int specified_delay_samps = (FREQ * SND_DELAY) / 1000;
    dsp_write_pos = (specified_delay_samps+SAMPLES) * bytes_per_sample;
    Audio_avg_gap = 0.0;
    Audio_gap_est = 0;

    for (int i = 0; i < dsp_buffer_bytes; i++)
        dsp_buffer[i] = 0;

    uint16_t buffer_size = (FREQ / Config_fps) * CHANNELS;
    for (int i = 0; i < buffer_size; i++)
        Audio_mix_buffer[i] = 0;

    callbacktick = 0;
}

void Audio_stop_audio()
{
    if (Audio_sound_enabled)
    {
        Audio_sound_enabled = FALSE;

        SDL_PauseAudio(1);
        SDL_CloseAudio();

        free(dsp_buffer);
        free(Audio_mix_buffer);
    }
}

void Audio_pause_audio()
{
    if (Audio_sound_enabled)
    {
        SDL_PauseAudio(1);
    }
}

void Audio_resume_audio()
{
    if (Audio_sound_enabled)
    {
        Audio_clear_buffers();
        SDL_PauseAudio(0);
    }
}

// Called every frame to update the audio
void Audio_tick()
{
    int bytes_written = 0;
    int newpos;
    double bytes_per_ms;

    if (!Audio_sound_enabled) return;

    // Update audio streams from PCM & YM Devices
    SegaPCM_stream_update();
    YM_stream_update();

    // Get the audio buffers we've just output
    int16_t *pcm_buffer = SegaPCM_get_buffer();
    int16_t *ym_buffer  = YM_get_buffer();
    int16_t *wav_buffer = Audio_wavfile.data;

    int samples_written = SegaPCM_buffer_size;

    // And mix them into the mix_buffer
    for (int i = 0; i < samples_written; i++)
    {
        int32_t mix_data = wav_buffer[Audio_wavfile.pos] + pcm_buffer[i] + ym_buffer[i];

        // Clip mix data
        if (mix_data >= (1 << 15))
            mix_data = (1 << 15);
        else if (mix_data < -(1 << 15))
            mix_data = -(1 << 15);

        Audio_mix_buffer[i] = mix_data;

        // Loop wav files
        if (++Audio_wavfile.pos >= Audio_wavfile.length)
            Audio_wavfile.pos = 0;
    }

    // Cast mix_buffer to a byte array, to align it with internal SDL format 
    uint8_t* mbuf8 = (uint8_t*) Audio_mix_buffer;

    // produce samples from the sound emulation
    bytes_per_ms = (bytes_per_sample) * (FREQ/1000.0);
    bytes_written = (BITS == 8 ? samples_written : samples_written*2);
    
    SDL_LockAudio();

    // this is the gap as of the most recent callback
    int gap = dsp_write_pos - dsp_read_pos;
    // an estimation of the current gap, adding time since then
    if (callbacktick != 0)
        Audio_gap_est = (int) (gap - (bytes_per_ms)*(SDL_GetTicks() - callbacktick));

    // if there isn't enough room...
    while (gap + bytes_written > dsp_buffer_bytes) 
    {
        // then we allow the callback to run..
        SDL_UnlockAudio();
        // and delay until it runs and allows space.
        SDL_Delay(1);
        SDL_LockAudio();
        //printf("sound buffer overflow:%d %d\n",gap, dsp_buffer_bytes);
        gap = dsp_write_pos - dsp_read_pos;
    }
    // now we copy the data into the buffer and adjust the positions
    newpos = dsp_write_pos + bytes_written;
    if (newpos/dsp_buffer_bytes == dsp_write_pos/dsp_buffer_bytes) 
    {
        // no wrap
        memcpy(dsp_buffer+(dsp_write_pos%dsp_buffer_bytes), mbuf8, bytes_written);
    }
    else 
    {
        // wraps
        int first_part_size = dsp_buffer_bytes - (dsp_write_pos%dsp_buffer_bytes);
        memcpy(dsp_buffer+(dsp_write_pos%dsp_buffer_bytes), mbuf8, first_part_size);
        memcpy(dsp_buffer, mbuf8+first_part_size, bytes_written-first_part_size);
    }
    dsp_write_pos = newpos;

    // Sound callback has not yet been called
    if (callbacktick == 0)
        dsp_read_pos += bytes_written;

    while (dsp_read_pos > dsp_buffer_bytes) 
    {
        dsp_write_pos -= dsp_buffer_bytes;
        dsp_read_pos -= dsp_buffer_bytes;
    }
    SDL_UnlockAudio();
}

// Adjust the speed of the emulator, based on audio streaming performance.
// This ensures that we avoid pops and crackles (in theory). 
double Audio_adjust_speed()
{
    if (!Audio_sound_enabled)
        return 1.0;

    double alpha = 2.0 / (1.0+40.0);
    int gap_too_small;
    int gap_too_large;
    Boolean inited = FALSE;

    if (!inited) 
    {
        inited = TRUE;
        Audio_avg_gap = Audio_gap_est;
    }
    else 
    {
        Audio_avg_gap = Audio_avg_gap + alpha * (Audio_gap_est - Audio_avg_gap);
    }

    gap_too_small = (SND_DELAY * FREQ * bytes_per_sample)/1000;
    gap_too_large = ((SND_DELAY + SND_SPREAD) * FREQ * bytes_per_sample)/1000;
    
    if (Audio_avg_gap < gap_too_small) 
    {
        double speed = 0.9;
        return speed;
    }
    if (Audio_avg_gap > gap_too_large)
    {
        double speed = 1.1;
        return speed;
    }
    return 1.0;
}

// Empty Wav Buffer
static int16_t EMPTY_BUFFER[] = {0, 0, 0, 0};

void Audio_load_wav(const char* filename)
{
    if (Audio_sound_enabled)
    {
        Audio_clear_wav();

        // Load Wav File
        SDL_AudioSpec wave;
    
        uint8_t *data;
        uint32_t length;

        Audio_pause_audio();

        if( SDL_LoadWAV(filename, &wave, &data, &length) == NULL)
        {
            Audio_wavfile.loaded = 0;
            Audio_resume_audio();
            fprintf(stderr, "Error: Could not load wav:  %s.\n", filename);
            return;
        }
        
        SDL_LockAudio();

        // Halve Volume Of Wav File
        uint8_t* data_vol = (uint8_t*)malloc(length);
        SDL_MixAudio(data_vol, data, length, SDL_MIX_MAXVOLUME / 2);

        // WAV File Needs Conversion To Target Format
        if (wave.format != AUDIO_S16 || wave.channels != 2 || wave.freq != FREQ)
        {
            SDL_AudioCVT cvt;
            SDL_BuildAudioCVT(&cvt, wave.format, wave.channels, wave.freq,
                                    AUDIO_S16,   CHANNELS,      FREQ);

            cvt.buf = (uint8_t*) malloc(length*cvt.len_mult);
            memcpy(cvt.buf, data_vol, length);
            cvt.len = length;
            SDL_ConvertAudio(&cvt);
            SDL_FreeWAV(data);
            free(data_vol);

            Audio_wavfile.data = (int16_t*) cvt.buf;
            Audio_wavfile.length = cvt.len_cvt / 2;
            Audio_wavfile.pos = 0;
            Audio_wavfile.loaded = 1;
        }
        // No Conversion Needed
        else
        {
            SDL_FreeWAV(data);
            Audio_wavfile.data = (int16_t*) data_vol;
            Audio_wavfile.length = length / 2;
            Audio_wavfile.pos = 0;
            Audio_wavfile.loaded = 2;
        }

        Audio_resume_audio();
        SDL_UnlockAudio();
    }
}

void Audio_clear_wav()
{
    if (Audio_wavfile.loaded)
    {
        if (Audio_wavfile.data)
            free(Audio_wavfile.data);
    }

    Audio_wavfile.length = 1;
    Audio_wavfile.data   = EMPTY_BUFFER;
    Audio_wavfile.pos    = 0;
    Audio_wavfile.loaded = FALSE;
}

// SDL Audio Callback Function
//
// Called when the audio device is ready for more data.
//
// stream:  A pointer to the audio buffer to be filled
// len:     The length (in bytes) of the audio buffer

void fill_audio(void *udata, Uint8 *stream, int len)
{
    int gap;
    int newpos;
    int underflow_amount = 0;
#define MAX_SAMPLE_SIZE 4
    static char last_bytes[MAX_SAMPLE_SIZE];

    gap = dsp_write_pos - dsp_read_pos;
    if (gap < len) 
    {
        underflow_amount = len - gap;
        len = gap;
    }
    newpos = dsp_read_pos + len;

    // No Wrap
    if (newpos/dsp_buffer_bytes == dsp_read_pos/dsp_buffer_bytes) 
    {
        memcpy(stream, dsp_buffer + (dsp_read_pos%dsp_buffer_bytes), len);
    }
    // Wrap
    else 
    {
        int first_part_size = dsp_buffer_bytes - (dsp_read_pos%dsp_buffer_bytes);
        memcpy(stream,  dsp_buffer + (dsp_read_pos%dsp_buffer_bytes), first_part_size);
        memcpy(stream + first_part_size, dsp_buffer, len - first_part_size);
    }
    // Save the last sample as we may need it to fill underflow
    if (gap >= bytes_per_sample) 
    {
        memcpy(last_bytes, stream + len - bytes_per_sample, bytes_per_sample);
    }
    // Just repeat the last good sample if underflow
    if (underflow_amount > 0 ) 
    {
        int i;
        for (i = 0; i < underflow_amount/bytes_per_sample; i++) 
        {
            memcpy(stream + len +i*bytes_per_sample, last_bytes, bytes_per_sample);
        }
    }
    dsp_read_pos = newpos;

    // Record the tick at which the callback occured.
    callbacktick = SDL_GetTicks();
}

#endif
