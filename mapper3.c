#include "mapper3.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static nes_header header;
static uint8_t *prg_rom;
static uint8_t *chr_rom;

static uint8_t nametables[2][0x400] ; // 2KiB of internal ram
static uint8_t mapper3_reg ; // holds the configurations

static uint8_t *bank0 , *cpu_bank0 , *cpu_bank1 ; // cpu banks are fixed

//implements mapper2
void init_mapper3 ( nes_header header0 , reader *mapper_cpu_read , reader *mapper_ppu_read , writer *mapper_cpu_write,
                    writer *mapper_ppu_write )
{
    header = header0 ;
    prg_rom = header0.prg_mem ;
    chr_rom = header0.chr_mem ;

    // assign the mapper0 function to the used mapping function ( set mapper0 )
    *mapper_cpu_read = mapper3_cpu_read;
    *mapper_cpu_write = mapper3_cpu_write;
    *mapper_ppu_read = mapper3_ppu_read;
    *mapper_ppu_write = mapper3_ppu_write;

    bank0 = chr_rom ; // put it at start of chr rom

    cpu_bank0 = prg_rom ;
    cpu_bank1 = (header.prg_rom_count == 2) ? (prg_rom + 0x4000): prg_rom ; // in the latter case it is identical to bank0
}

uint8_t mapper3_ppu_read ( uint16_t address )
{
    if ( address >= 0 && address <= 0x1FFF ) // pattern tables
        return bank0[address] ;
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
void mapper3_ppu_write( uint16_t address , uint8_t data )
{
    if ( address >= 0 && address <= 0x1FFF ) // pattern tables
        //chr_rom[address] = data;
        return ; // this is all rom
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
void mapper3_cpu_write( uint16_t address , uint8_t data )
{
    // cpu is writing to the mapper registers
    mapper3_reg = data & 0x03 ;
    bank0 = chr_rom + mapper3_reg * 0x2000 ; // position the bank
}
uint8_t mapper3_cpu_read(  uint16_t address )
{
    if ( address >= 0x8000 && address <= 0xBFFF )
        return cpu_bank0[address - 0x8000] ;
    else if ( address >= 0xC000 && address <= 0xFFFF )
    {
        return cpu_bank1[address - 0xC000];
    }
    return 0 ;
}