/***************************************************************************
    SDL Software Video Rendering.  
    
    Known Bugs:
    - Scanlines don't work when Endian changed?

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "rendersw.h"
#include "frontend/config.h"

#include "../globals.h"
#include "../setup.h"
#include <SDL.h>
 

uint32_t my_min(uint32_t a, uint32_t b) { return a < b ? a : b; }

SDL_Surface *Render_surface;

// Palette Lookup
uint32_t Render_rgb[S16_PALETTE_ENTRIES * 3];    // Extended to hold shadow/hilight colours

uint32_t *Render_screen_pixels;

// Original Screen Width & Height
uint16_t Render_orig_width, Render_orig_height;

// --------------------------------------------------------------------------------------------
// Screen setup properties. Example below: 
// ________________________
// |  |                |  | <- screen size      (e.g. 1280 x 720)
// |  |                |  |
// |  |                |<-|--- destination size (e.g. 1027 x 720) to maintain aspect ratio
// |  |                |  | 
// |  |                |  |    source size      (e.g. 320  x 224) System 16 proportions
// |__|________________|__|
//
// --------------------------------------------------------------------------------------------

// Source texture / pixel array that we are going to manipulate
int Render_src_width, Render_src_height;

// Destination window width and height
int Render_dst_width, Render_dst_height;

// Screen width and height 
int Render_scn_width, Render_scn_height;

// Full-Screen, Stretch, Window
int Render_video_mode;

// Scanline density. 0 = Off, 1 = Full
int Render_scanlines;

// Offsets (for full-screen mode, where x/y resolution isn't a multiple of the original height)
uint32_t Render_screen_xoff, Render_screen_yoff;

// SDL Pixel Format Codes. These differ between platforms.
uint8_t  Render_Rshift, Render_Gshift, Render_Bshift;
uint32_t Render_Rmask, Render_Gmask, Render_Bmask;

Boolean Render_sdl_screen_size();

// Scanline pixels
uint32_t* Render_scan_pixels = NULL;

// Pixel Conversion
uint32_t* Render_pix = NULL;

// Scale the screen
int Render_scale_factor;

void Render_scale( uint32_t* src, int srcwid, int srchgt, 
            uint32_t* dest, int dstwid, int dsthgt);
    
void Render_scanlines_32bpp(uint32_t* src, const int width, const int height, 
                            uint32_t* dst, int percent, Boolean interpolate);

void Render_scalex( uint32_t* src, const int srcwid, const int srchgt, uint32_t* dest, const int scale);

Boolean Render_init(int src_width, int src_height, 
                    int scale,
                    int video_mode,
                    int scanlines)
{
    Render_src_width  = src_width;
    Render_src_height = src_height;
    Render_video_mode = video_mode;
    Render_scanlines  = scanlines;

    // Setup SDL Screen size
    if (!Render_sdl_screen_size())
        return FALSE;

    int flags = SDL_FLAGS;

    // --------------------------------------------------------------------------------------------
    // Full Screen Mode
    // When using full-screen mode, we attempt to keep the current resolution.
    // This is because for LCD monitors, I suspect it's what we want to remain in
    // and we don't want to risk upsetting the aspect ratio.
    // --------------------------------------------------------------------------------------------
    if (video_mode == VIDEO_MODE_FULL || video_mode == VIDEO_MODE_STRETCH)
    {
        Render_scn_width  = Render_orig_width;
        Render_scn_height = Render_orig_height;

        Render_scale_factor = 0; // Use scaling code
        
        if (video_mode == VIDEO_MODE_STRETCH)
        {
            Render_dst_width  = Render_scn_width;
            Render_dst_height = Render_scn_height;
            scanlines = 0; // Disable scanlines in stretch mode
        }
        else
        {
            // With scanlines, only allow a proportional scale
            if (scanlines)
            {
                Render_scale_factor = my_min(Render_scn_width / Render_src_width, Render_scn_height / Render_src_height);
                Render_dst_width    = src_width  * Render_scale_factor;
                Render_dst_height   = src_height * Render_scale_factor;
            }
            else
            {
                // Calculate how much to scale screen from its original resolution
                uint32_t w = (Render_scn_width  << 16)  / src_width;
                uint32_t h = (Render_scn_height << 16)  / src_height;
                Render_dst_width  = (src_width  * my_min(w, h)) >> 16;
                Render_dst_height = (src_height * my_min(w, h)) >> 16;
            }
        }
        flags |= SDL_FULLSCREEN; // Set SDL flag
        SDL_ShowCursor(FALSE);   // Don't show mouse cursor in full-screen mode
    }
    // --------------------------------------------------------------------------------------------
    // Windowed Mode
    // --------------------------------------------------------------------------------------------
    else
    {
        Render_video_mode = VIDEO_MODE_WINDOW;
       
        Render_scale_factor  = scale;

        Render_scn_width  = Render_src_width  * Render_scale_factor;
        Render_scn_height = Render_src_height * Render_scale_factor;

        // As we're windowed this is just the same
        Render_dst_width  = Render_scn_width;
        Render_dst_height = Render_scn_height;
        
        SDL_ShowCursor(TRUE);
    }

    // If we're not stretching the screen, centre the image
    if (video_mode != VIDEO_MODE_STRETCH)
    {
        Render_screen_xoff = Render_scn_width - Render_dst_width;
        if (Render_screen_xoff)
            Render_screen_xoff = (Render_screen_xoff / 2);

        Render_screen_yoff = Render_scn_height - Render_dst_height;
        if (Render_screen_yoff) 
            Render_screen_yoff = (Render_screen_yoff / 2) * Render_scn_width;
    }
    // Otherwise set to the top-left corner
    else
    {
        Render_screen_xoff = 0;
        Render_screen_yoff = 0;
    }

    //int bpp = info->vfmt->BitsPerPixel;
    const int bpp = 16;
    const int available = SDL_VideoModeOK(Render_scn_width, Render_scn_height, bpp, flags);

    // Frees (Deletes) existing surface
    if (Render_surface)
        SDL_FreeSurface(Render_surface);

    // Set the video mode
    Render_surface = SDL_SetVideoMode(Render_scn_width, Render_scn_height, bpp, flags);

    if (!Render_surface || !available)
    {
        fprintf(stderr, "Video mode set failed: %d.\n", SDL_GetError());
        return FALSE;
    }

    // Convert the SDL pixel surface to 32 bit.
    // This is potentially a larger surface area than the internal pixel array.
    Render_screen_pixels = (uint16_t*)Render_surface->pixels;
    
    // SDL Pixel Format Information
    Render_Rshift = Render_surface->format->Rshift;
    Render_Gshift = Render_surface->format->Gshift;
    Render_Bshift = Render_surface->format->Bshift;
    Render_Rmask  = Render_surface->format->Rmask;
    Render_Gmask  = Render_surface->format->Gmask;
    Render_Bmask  = Render_surface->format->Bmask;

    // Doubled intermediate pixel array for scanlines
    if (scanlines)
    {
        if (Render_scan_pixels) 
            free(Render_scan_pixels);
        Render_scan_pixels = (uint16_t*)malloc((Render_src_width * 2) * (Render_src_height * 2)* sizeof(uint16_t));
    }

    if (Render_pix)
        free(Render_pix);
    Render_pix = (uint16_t*)malloc(Render_src_width * Render_src_height * sizeof(uint16_t));

    return TRUE;
}

void Render_disable()
{

}

Boolean Render_start_frame()
{
    return TRUE;
}

Boolean Render_finalize_frame()
{
   
    SDL_Flip(Render_surface);

    return TRUE;
}

void Render_draw_frame(uint16_t* pixels)
{
    int i = 0;
    uint16_t* spix = Render_screen_pixels;

  
// Lookup real RGB value from rgb array for backbuffer
    for (i = 0; i < (0x11800); i+=16)
    {
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];
        *(spix++) = Render_rgb[*(pixels++) & ((S16_PALETTE_ENTRIES * 3) - 1)];

    }        
}

 
 


// Setup screen size
Boolean Render_sdl_screen_size()
{
    if (Render_orig_width == 0 || Render_orig_height == 0)
    {
        const SDL_VideoInfo* info = SDL_GetVideoInfo();

        if (!info)
        {
            //std::cerr << "Video query failed: " << SDL_GetError() << std::endl;
            return FALSE;
        }
        
        Render_orig_width  = 320; 
        Render_orig_height = 224;
    }

    Render_scn_width  = Render_orig_width;
    Render_scn_height = Render_orig_height;

    return TRUE;
}

// See: SDL_PixelFormat
#define CURRENT_RGB() (r << Render_Rshift) | (g << Render_Gshift) | (b << Render_Bshift);

void Render_convert_palette(uint32_t adr, uint32_t r, uint32_t g, uint32_t b)
{
    adr >>= 1;
 
    Render_rgb[adr] = CURRENT_RGB();
      
    // Create shadow / highlight colours at end of RGB array
    // The resultant values are the same as MAME
    r = r * 202 / 256;
    g = g * 202 / 256;
    b = b * 202 / 256;
        
    Render_rgb[adr + S16_PALETTE_ENTRIES] =
    Render_rgb[adr + (S16_PALETTE_ENTRIES * 2)] = CURRENT_RGB();
}
