#include "mapper0.h"
#include "mapper.h"
#include <stdint.h>

typedef uint8_t (*reader)(uint16_t) ;
typedef void (*writer)( uint16_t , uint8_t ) ;

extern reader mapper_cpu_read , mapper_ppu_read;
extern writer mapper_cpu_write , mapper_ppu_write;

static nes_header header;
static uint8_t *prg_rom;
static uint8_t *chr_rom;

static uint8_t nametables[2][0x400] ; // 2KiB of internal ram

// uint8_t some_ram[0x1000] ; // for blargg tests

//implements mapper0 ( NROM )
void init_mapper0 ( nes_header header0 )
{
    header = header0 ;
    prg_rom = header0.prg_mem ;
    chr_rom = header0.chr_mem ;

    // assign the mapper0 function to the used mapping function ( set mapper0 )
    mapper_cpu_read = mapper0_cpu_read;
    mapper_cpu_write = mapper0_cpu_write ;
    mapper_ppu_read = mapper0_ppu_read ;
    mapper_ppu_write = mapper0_ppu_write ;
}

uint8_t mapper0_ppu_read ( uint16_t address )
{
    if ( address >= 0 && address <= 0x1FFF ) // pattern tables
        return chr_rom[address] ;
    else if ( address >= 0x2000 && address <= 0x3EFF ) // nametables and mirrors
    {
        uint16_t temp = address & 0x0FFF ;
        if ( header.mirroring ) // if vertical
        {
            if ( temp >= 0x800 && temp <= 0xFFF ) // access from 2 or 3
                temp -= 0x800 ; // mirror to 0 and 1

            return nametables[(temp & 0x0400) >> 10][temp & 0x03FF ] ; // TODO : check here
        }
        else // horizontal
        {
            if ( temp >= 0x000 && temp <= 0x7FF ) // access from 0 or 1
                return nametables[0][temp & 0x3FF] ; // mirror 0 and 1
            else
                return nametables[1][ temp & 0xBFF -0x800 ] ; // mirror 2 and 3
        }
    }
}
void mapper0_ppu_write( uint16_t address , uint8_t data )
{
        if ( address >= 0 && address <= 0x1FFF ) // pattern tables
            //chr_rom[address] = data;
            return; // all ROM
        else if ( address >= 0x2000 && address <= 0x3EFF ) // nametables and mirrors
        {
            uint16_t temp = address & 0x0FFF ;
            if ( header.mirroring ) // if vertical
            {
                if ( temp >= 0x800 && temp <= 0xFFF ) // access from 2 or 3
                    temp -= 0x800 ; // mirror to 0 and 1

                nametables[(temp & 0x0400) >> 10][temp & 0x03FF ] = data ; // TODO : check here
            }
            else // horizontal
            {
                if ( temp >= 0x000 && temp <= 0x7FF ) // access from 0 or 1
                    nametables[0][temp & 0x3FF] = data; // mirror 0 and 1
                else
                    nametables[1][ (temp & 0xBFF) -0x800] = data; // mirror 2 and 3
            }

        }
}
void mapper0_cpu_write( uint16_t address , uint8_t data )
{
//    if (address >= 0x6000 && address <= 0x7000 )
//        some_ram[address - 0x6000 ] = data ;
//    // print the diagnostics to terminal
//
//    for ( int i = 0 ; i < 0x1000 ; ++i )
//        printf("\n %s \n ", some_ram + 4 ); // starts at 0x6004


    return ; // this is all ROM
}
uint8_t mapper0_cpu_read(  uint16_t address )
{
    // we are in 0x4020 to 0xFFFF which is Cartridge space
    // 1 prg rom means NROM-128 , 2 for NROM-256 || 1: $8000-$BFFF 2:$C000-$FFFF or mirror of 1
    if ( address >= 0x8000 && address <= 0xBFFF )
        return prg_rom[address - 0x8000 ] ;
    else if ( address >= 0xC000 && address <= 0xFFFF )
    { // see if we need to mirror
        if ( header.prg_rom_count == 2 ) // NROM-256
        {
            return prg_rom[address - 0x8000] ;
        }
        return prg_rom[address -0xC000 ] ; //mirror
    }
    return 0 ;
}