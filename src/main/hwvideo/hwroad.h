#pragma once

#include "stdint.h"


#define ROAD_RAM_SIZE 0x1000
#define HWRoad_rom_size 0x8000

void HWRoad_init(const uint8_t*, const Boolean hires);
void HWRoad_write16(uint32_t adr, const uint16_t data);
void HWRoad_write16IncP(uint32_t* adr, const uint16_t data);
void HWRoad_write32(uint32_t* adr, const uint32_t data);
uint16_t HWRoad_read_road_control();
void HWRoad_write_road_control(const uint8_t);

extern void (*HWRoad_render_background)(uint16_t*);
extern void (*HWRoad_render_foreground)(uint16_t*);
  
