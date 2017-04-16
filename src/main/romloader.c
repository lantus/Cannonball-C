/***************************************************************************
    Binary File Loader. 
    
    Handles loading an individual binary file to memory.
    Supports reading bytes, words and longs from this area of memory.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "stdint.h"
#include "romloader.h"
#include "thirdparty/crc/crc.h"

#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif

int RomLoader_filesize(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    fseek(file, 0L, SEEK_END);
    uint32_t size = ftell(file);
    fclose(file);
    return size; 
}

void RomLoader_create(RomLoader* romLoader)
{
    romLoader->loaded = FALSE;
}

void RomLoader_init(RomLoader* romLoader, uint32_t length)
{
    romLoader->length = length;
    romLoader->rom = (uint8_t*)malloc(length);
    crcInit();
}

void RomLoader_unload(RomLoader* romLoader)
{
    free(romLoader->rom);
}

int maxsize = 0;

int RomLoader_load(RomLoader* romLoader, const char* filename, const int offset, const int length, const int expected_crc, const uint8_t interleave)
{
    int i = 0;
    if (maxsize < length)
    {
        maxsize = length;
    }

#ifdef __APPLE__    
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char bundlepath[PATH_MAX];

    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)bundlepath, PATH_MAX))
    {
        // error!
    }

    CFRelease(resourcesURL);
    chdir(bundlepath);
#endif

    // Open rom file
    FILE* file = fopen(filename, "rb");

    if (!file)
    {
        fprintf(stderr, "Cannot open rom: %s\n", filename);
        romLoader->loaded = FALSE;
        return 1; // fail
    }

    // Read file
    char* buffer = (char*) malloc(length);
    int read = fread(buffer, length, 1, file);

    crc computed_crc = crcSlow(buffer, length);

    if (expected_crc != computed_crc)
    {
        fprintf(stderr, "Error: %s has incorrect checksum.\nExpected: %x Found: %x.\n", filename, expected_crc, computed_crc);
    }

    // Interleave file as necessary
    for (i = 0; i < length; i++)
    {
        romLoader->rom[(i * interleave) + offset] = buffer[i];
    }

    // Clean Up
    free(buffer);
    fclose(file);
    romLoader->loaded = TRUE;
    return 0; // success
}

// Load Binary File (LayOut Levels, Tilemap Data etc.)
int RomLoader_load_binary(RomLoader* romLoader, const char* filename)
{
#ifdef __APPLE__    
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char bundlepath[PATH_MAX];

    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)bundlepath, PATH_MAX))
    {
        // error!
    }

    CFRelease(resourcesURL);
    chdir(bundlepath);
#endif

    // --------------------------------------------------------------------------------------------
    // Read LayOut Data File
    // --------------------------------------------------------------------------------------------

    FILE* file = fopen(filename, "rb");

    if (!file)
    {
        fprintf(stderr, "Error: cannot open file:  %s.\n", filename);
        romLoader->loaded = FALSE;
        return 1; // fail
    }

    romLoader->length = RomLoader_filesize(filename);

    // Read file
    char* buffer = (char*)malloc(romLoader->length);
    fread(buffer, romLoader->length, 1, file);
    romLoader->rom = (uint8_t*) buffer;

    // Clean Up
    fclose(file);

    romLoader->loaded = TRUE;
    return 0; // success
}

// ----------------------------------------------------------------------------
// Used by translated 68000 Code
// ----------------------------------------------------------------------------

uint32_t RomLoader_read32IncP(RomLoader* romLoader, uint32_t* addr)
{    
    uint32_t data = (romLoader->rom[*addr] << 24) | (romLoader->rom[*addr+1] << 16) | (romLoader->rom[*addr+2] << 8) | (romLoader->rom[*addr+3]);
    *addr += 4;
    return data;
}

uint16_t RomLoader_read16IncP(RomLoader* romLoader, uint32_t* addr)
{
    uint16_t data = (romLoader->rom[*addr] << 8) | (romLoader->rom[*addr+1]);
    *addr += 2;
    return data;
}

uint8_t RomLoader_read8IncP(RomLoader* romLoader, uint32_t* addr)
{
    return romLoader->rom[(*addr)++]; 
}

uint32_t RomLoader_read32(RomLoader* romLoader, uint32_t addr)
{    
    return (romLoader->rom[addr] << 24) | (romLoader->rom[addr+1] << 16) | (romLoader->rom[addr+2] << 8) | romLoader->rom[addr+3];
}

uint16_t RomLoader_read16(RomLoader* romLoader, uint32_t addr)
{
    return (romLoader->rom[addr] << 8) | romLoader->rom[addr+1];
}

uint8_t RomLoader_read8(RomLoader* romLoader, uint32_t addr)
{
    return romLoader->rom[addr];
}

// ----------------------------------------------------------------------------
// Used by translated Z80 Code
// Note that the endian is reversed compared with the 68000 code.
// ----------------------------------------------------------------------------

uint16_t RomLoader_read16IncP_addr16(RomLoader* romLoader, uint16_t* addr)
{
    uint16_t data = (romLoader->rom[*addr+1] << 8) | (romLoader->rom[*addr]);
    *addr += 2;
    return data;
}

uint8_t RomLoader_read8IncP_addr16(RomLoader* romLoader, uint16_t* addr)
{
    return romLoader->rom[(*addr)++]; 
}

uint16_t RomLoader_read16_addr16(RomLoader* romLoader, uint16_t addr)
{
    return (romLoader->rom[addr+1] << 8) | romLoader->rom[addr];
}

uint8_t RomLoader_read8_addr16(RomLoader* romLoader, uint16_t addr)
{
    return romLoader->rom[addr];
}
