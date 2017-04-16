/***************************************************************************
    Load OutRun ROM Set.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "stdint.h"
#include "roms.h"

RomLoader Roms_rom0;
RomLoader Roms_rom1;
RomLoader Roms_tiles;
RomLoader Roms_sprites;
RomLoader Roms_road;
RomLoader Roms_z80;
RomLoader Roms_pcm;
RomLoader Roms_j_rom0;
RomLoader Roms_j_rom1;
RomLoader* Roms_rom0p;
RomLoader* Roms_rom1p;

int jap_rom_status = -1;



Boolean Roms_load_revb_roms()
{
    // If incremented, a rom has failed to load.
    int status = 0;

    // Load Master CPU ROMs
    RomLoader_init(&Roms_rom0, 0x40000);
    status += RomLoader_load(&Roms_rom0, "roms/epr-10381a.132", 0x20000, 0x10000, 0xbe8c412b, ROMLOADER_INTERLEAVE2);
    
    // Try alternate filename for this rom
    if (status)
    {
        RomLoader_load(&Roms_rom0, "roms/epr-10381b.132", 0x20000, 0x10000, 0xbe8c412b, ROMLOADER_INTERLEAVE2);
        status--;
    }
    
    status += RomLoader_load(&Roms_rom0, "roms/epr-10383b.117", 0x20001, 0x10000, 0x10a2014a, ROMLOADER_INTERLEAVE2);
    status += RomLoader_load(&Roms_rom0, "roms/epr-10380b.133", 0x00000, 0x10000, 0x1f6cadad, ROMLOADER_INTERLEAVE2);
    status += RomLoader_load(&Roms_rom0, "roms/epr-10382b.118", 0x00001, 0x10000, 0xc4c3fa1a, ROMLOADER_INTERLEAVE2);

    // Load Slave CPU ROMs
    RomLoader_init(&Roms_rom1, 0x40000);
    status += RomLoader_load(&Roms_rom1, "roms/epr-10327a.76", 0x00000, 0x10000, 0xe28a5baf, ROMLOADER_INTERLEAVE2);
    status += RomLoader_load(&Roms_rom1, "roms/epr-10329a.58", 0x00001, 0x10000, 0xda131c81, ROMLOADER_INTERLEAVE2);
    status += RomLoader_load(&Roms_rom1, "roms/epr-10328a.75", 0x20000, 0x10000, 0xd5ec5e5d, ROMLOADER_INTERLEAVE2);
    status += RomLoader_load(&Roms_rom1, "roms/epr-10330a.57", 0x20001, 0x10000, 0xba9ec82a, ROMLOADER_INTERLEAVE2);

    // Load Non-Interleaved Tile ROMs
    RomLoader_init(&Roms_tiles, 0x30000);
    status += RomLoader_load(&Roms_tiles, "roms/opr-10268.99",  0x00000, 0x08000, 0x95344b04, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_tiles, "roms/opr-10232.102", 0x08000, 0x08000, 0x776ba1eb, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_tiles, "roms/opr-10267.100", 0x10000, 0x08000, 0xa85bb823, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_tiles, "roms/opr-10231.103", 0x18000, 0x08000, 0x8908bcbf, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_tiles, "roms/opr-10266.101", 0x20000, 0x08000, 0x9f6f1a74, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_tiles, "roms/opr-10230.104", 0x28000, 0x08000, 0x686f5e50, ROMLOADER_NORMAL);

    // Load Non-Interleaved Road ROMs (2 identical roms, 1 for each road)
    RomLoader_init(&Roms_road, 0x10000);
    status += RomLoader_load(&Roms_road, "roms/opr-10185.11", 0x000000, 0x08000, 0x22794426, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_road, "roms/opr-10186.47", 0x008000, 0x08000, 0x22794426, ROMLOADER_NORMAL);

    // Load Interleaved Sprite ROMs
    RomLoader_init(&Roms_sprites, 0x100000);
    status += RomLoader_load(&Roms_sprites, "roms/mpr-10371.9",  0x000000, 0x20000, 0x7cc86208, ROMLOADER_INTERLEAVE4);
    status += RomLoader_load(&Roms_sprites, "roms/mpr-10373.10", 0x000001, 0x20000, 0xb0d26ac9, ROMLOADER_INTERLEAVE4);
    status += RomLoader_load(&Roms_sprites, "roms/mpr-10375.11", 0x000002, 0x20000, 0x59b60bd7, ROMLOADER_INTERLEAVE4);
    status += RomLoader_load(&Roms_sprites, "roms/mpr-10377.12", 0x000003, 0x20000, 0x17a1b04a, ROMLOADER_INTERLEAVE4);
    status += RomLoader_load(&Roms_sprites, "roms/mpr-10372.13", 0x080000, 0x20000, 0xb557078c, ROMLOADER_INTERLEAVE4);
    status += RomLoader_load(&Roms_sprites, "roms/mpr-10374.14", 0x080001, 0x20000, 0x8051e517, ROMLOADER_INTERLEAVE4);
    status += RomLoader_load(&Roms_sprites, "roms/mpr-10376.15", 0x080002, 0x20000, 0xf3b8f318, ROMLOADER_INTERLEAVE4);
    status += RomLoader_load(&Roms_sprites, "roms/mpr-10378.16", 0x080003, 0x20000, 0xa1062984, ROMLOADER_INTERLEAVE4);

    // Load Z80 Sound ROM
    RomLoader_init(&Roms_z80, 0x10000);
    status += RomLoader_load(&Roms_z80, "roms/epr-10187.88", 0x0000, 0x08000, 0xa10abaa9, ROMLOADER_NORMAL);

    // Load Sega PCM Chip Samples
    RomLoader_init(&Roms_pcm, 0x60000);
    status += RomLoader_load(&Roms_pcm, "roms/opr-10193.66", 0x00000, 0x08000, 0xbcd10dde, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_pcm, "roms/opr-10192.67", 0x10000, 0x08000, 0x770f1270, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_pcm, "roms/opr-10191.68", 0x20000, 0x08000, 0x20a284ab, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_pcm, "roms/opr-10190.69", 0x30000, 0x08000, 0x7cab70e2, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_pcm, "roms/opr-10189.70", 0x40000, 0x08000, 0x01366b54, ROMLOADER_NORMAL);
    status += RomLoader_load(&Roms_pcm, "roms/opr-10188.71", 0x50000, 0x08000, 0xbad30ad9, ROMLOADER_NORMAL);

    // If status has been incremented, a rom has failed to load.
    return status == 0;
}

Boolean Roms_load_japanese_roms()
{
    // Only attempt to initalize the arrays once.
    if (jap_rom_status == -1)
    {
        RomLoader_init(&Roms_j_rom0, 0x40000);
        RomLoader_init(&Roms_j_rom1, 0x40000);
    }

    // If incremented, a rom has failed to load.
    jap_rom_status = 0;

    // Load Master CPU ROMs     
    jap_rom_status += RomLoader_load(&Roms_j_rom0, "roms/epr-10380.133", 0x00000, 0x10000, 0xe339e87a, ROMLOADER_INTERLEAVE2);
    jap_rom_status += RomLoader_load(&Roms_j_rom0, "roms/epr-10382.118", 0x00001, 0x10000, 0x65248dd5, ROMLOADER_INTERLEAVE2);
    jap_rom_status += RomLoader_load(&Roms_j_rom0, "roms/epr-10381.132", 0x20000, 0x10000, 0xbe8c412b, ROMLOADER_INTERLEAVE2);
    jap_rom_status += RomLoader_load(&Roms_j_rom0, "roms/epr-10383.117", 0x20001, 0x10000, 0xdcc586e7, ROMLOADER_INTERLEAVE2);
                                                 
    // Load Slave CPU ROMs                       
    jap_rom_status += RomLoader_load(&Roms_j_rom1, "roms/epr-10327.76", 0x00000, 0x10000, 0xda99d855, ROMLOADER_INTERLEAVE2);
    jap_rom_status += RomLoader_load(&Roms_j_rom1, "roms/epr-10329.58", 0x00001, 0x10000, 0xfe0fa5e2, ROMLOADER_INTERLEAVE2);
    jap_rom_status += RomLoader_load(&Roms_j_rom1, "roms/epr-10328.75", 0x20000, 0x10000, 0x3c0e9a7f, ROMLOADER_INTERLEAVE2);
    jap_rom_status += RomLoader_load(&Roms_j_rom1, "roms/epr-10330.57", 0x20001, 0x10000, 0x59786e99, ROMLOADER_INTERLEAVE2);
    // If status has been incremented, a rom has failed to load.
    return jap_rom_status == 0;
}

Boolean Roms_load_pcm_rom(Boolean fixed_rom)
{
    int status = 0;

    if (fixed_rom)
        status += RomLoader_load(&Roms_pcm, "roms/opr-10188.71f", 0x50000, 0x08000, 0x37598616, ROMLOADER_NORMAL);
    else
        status += RomLoader_load(&Roms_pcm, "roms/opr-10188.71", 0x50000, 0x08000, 0xbad30ad9, ROMLOADER_NORMAL);

    return status == 0;
}