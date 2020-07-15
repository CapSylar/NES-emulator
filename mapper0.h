#ifndef NESX_MAPPER0_H
#define NESX_MAPPER0_H

#include <stdint.h>
#include "mapper.h"

void init_mapper0 ( nes_header header0 , reader *mapper_cpu_read , reader *mapper_ppu_read , writer *mapper_cpu_write,
                    writer *mapper_ppu_write ) ;
uint8_t mapper0_ppu_read ( uint16_t address );
void mapper0_ppu_write( uint16_t address , uint8_t data );
void mapper0_cpu_write( uint16_t address , uint8_t data );
uint8_t mapper0_cpu_read(  uint16_t address );

#endif //NESX_MAPPER0_H
