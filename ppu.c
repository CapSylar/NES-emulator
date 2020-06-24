#include "6502.h"

uint8_t pattern_tables[2][0x1000] ;
uint8_t p_oam [256] ;
uint8_t s_oam [64] ;
uint8_t nametables[0x2000];
uint8_t palette[0x20];

typedef struct
{
    bool odd_cycle ;
    int scanline ;

    uint8_t shift_reg[8];
    uint8_t latches[8];
    uint8_t x_pos[8];
/*
    uint8_t *ppu_ctrl = internal_ram + 0x2000 ;
    uint8_t *ppu_mask = internal_ram + 0x2001 ;
    uint8_t *ppu_status = internal_ram + 0x2002 ;
    uint8_t *oam_addr = internal_ram + 0x2003 ;
    uint8_t *oam_data = internal_ram + 0x2004 ;
    uint8_t *ppu_scroll = internal_ram + 0x2005 ;
    uint8_t *ppu_addr = internal_ram + 0x2006 ;
    uint8_t *ppu_data = internal_ram + 0x2007 ;
    uint8_t *oam_dma = internal_ram + 0x2008 ; */

} ppu_state ;
/*

uint8_t* ppu_rw ( uint16_t address )
{
    if ( address >= 0 && address < 0x2000 ) // pattern tables
        return pattern_tables + address ;
    else if (  address >= 0x2000 && address < 0x3000 )
        return nametables + address ;
    else if ( address >= 3000 && address < 0x3F00 ) // mirrors of nametables
        return address - 0x1000 + nametables ;
    else if ( address >= 0x3F00 && address < 0x3F20 ) // palette ram
        return address + palette ;
    else if ( address >= 0x3F20 && address < 0x3FFF ) // mapped to palette ram
        return palette + address % 0x20 ;
}
*/


uint8_t ppu_read ( uint16_t address ) // 14 lines are actually used
{
    if ( address >= 0 && address <= 0x1FFF ) // pattern tables
        return pattern_tables[address & 0x1000][address & 0x0FFF];
    else if ( address >= 0x2000 && address <= 0x3EFF ) // nametables and mirrors
    {
        //TODO: map the nametables for the ppu bus
    }
    else if ( address >= 0x3F00 && address <= 0x3FFF ) // palette RAM indexes
    {

    }

}

void ppu_write ( uint16_t address , uint8_t data )
{

}
