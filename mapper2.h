#ifndef NESX_MAPPER2_H
#define NESX_MAPPER2_H

#include <stdint.h>
#include "mapper.h"

void init_mapper2 ( nes_header header0 , reader *mapper_cpu_read , reader *mapper_ppu_read , writer *mapper_cpu_write,
                    writer *mapper_ppu_write ) ;
uint8_t mapper2_ppu_read ( uint16_t address );
void mapper2_ppu_write( uint16_t address , uint8_t data );
void mapper2_cpu_write( uint16_t address , uint8_t data );
uint8_t mapper2_cpu_read(  uint16_t address );

#endif //NESX_MAPPER2_H
