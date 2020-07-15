#ifndef NESX_MAPPER3_H
#define NESX_MAPPER3_H

#include <stdint.h>
#include "mapper.h"

void init_mapper3 ( nes_header header0 , reader *mapper_cpu_read , reader *mapper_ppu_read , writer *mapper_cpu_write,
                    writer *mapper_ppu_write ) ;
uint8_t mapper3_ppu_read ( uint16_t address );
void mapper3_ppu_write( uint16_t address , uint8_t data );
void mapper3_cpu_write( uint16_t address , uint8_t data );
uint8_t mapper3_cpu_read(  uint16_t address );

#endif //NESX_MAPPER3_H
