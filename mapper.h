#ifndef NESX_MAPPER_H
#define NESX_MAPPER_H

#include <stdbool.h>

typedef struct // contains all the flags extracted from the header
{
    int prg_rom_count;
    int chr_rom_count;
    bool mirroring ; // 0 : horizontal 1 : vertical
    bool has_pers_mem ; // has persistent memory
    bool four_nt ; // 4 nametables option
    bool has_prg_ram ; // PRG RAM at $6000-$7FFF 0:present 1:not present
    int prg_ram_size ; // size in 8KB units

    uint8_t *prg_mem ;
    uint8_t *chr_mem ;

} nes_header ;

// the mappers will use these types for IO

typedef uint8_t (*reader)(uint16_t) ;
typedef void (*writer)( uint16_t , uint8_t ) ;

#endif //NESX_MAPPER_H
