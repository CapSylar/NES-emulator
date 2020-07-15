#include "mapper2.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static nes_header header;
static uint8_t *prg_rom;
static uint8_t *chr_rom;

static uint8_t nametables[2][0x400] ; // 2KiB of internal ram
static uint8_t mapper2_reg ; // holds the configurations

// pointers for the 2 "windows"
static uint8_t *bank0 , *bank1 ;


//implements mapper2
void init_mapper2 ( nes_header header0 , reader *mapper_cpu_read , reader *mapper_ppu_read , writer *mapper_cpu_write,
                    writer *mapper_ppu_write )
{
    header = header0 ;
    prg_rom = header0.prg_mem ;
    chr_rom = header0.chr_mem ;

    // assign the mapper0 function to the used mapping function ( set mapper0 )
    *mapper_cpu_read = mapper2_cpu_read;
    *mapper_cpu_write = mapper2_cpu_write ;
    *mapper_ppu_read = mapper2_ppu_read ;
    *mapper_ppu_write = mapper2_ppu_write ;

    // set the correct banks to start
    if ( header.prg_rom_count == 8 ) // UNROM
        bank1 = prg_rom + 7 * 0x4000 ; // move to last bank
    else // UOROM
    {
        fprintf( stderr , "rom uses unimplemented UOROM version of mapper 2\n" ) ;;
        exit(EXIT_FAILURE) ;
    }

    bank0 = prg_rom ; // set to start as a default
}

uint8_t mapper2_ppu_read ( uint16_t address )
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
void mapper2_ppu_write( uint16_t address , uint8_t data )
{
    if ( address >= 0 && address <= 0x1FFF ) // pattern tables
        chr_rom[address] = data;
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
void mapper2_cpu_write( uint16_t address , uint8_t data )
{
    // cpu is writing to the mapper registers
    mapper2_reg = data & 0x0F ;
    bank0 = prg_rom + mapper2_reg * 0x4000 ; // position the bank
}
uint8_t mapper2_cpu_read(  uint16_t address )
{
    // we are in 0x4020 to 0xFFFF which is Cartridge space
    // 1 prg rom means NROM-128 , 2 for NROM-256 || 1: $8000-$BFFF 2:$C000-$FFFF or mirror of 1
    if ( address >= 0x8000 && address <= 0xBFFF )
        return bank0[address - 0x8000] ;
    else if ( address >= 0xC000 && address <= 0xFFFF ) // fixed to last 16Kb bank
    {
        return bank1[address - 0xC000];
    }
    return 0 ;
}