#include "6502.h"
#include "ppu.h"

uint8_t internal_ram[0x800] ; // NES only has 2KB of internal RAM from 0000 to 07FFF
uint8_t cartridge_mem [0xBFE0] ;
extern uint8_t pattern_tables [2][0x1000] ;
extern ppu_state nes_ppu ;

// we can't write a single function for the read and write because some reads and writes trigger special behavior
// on the ships that can't be predicted if we simply return a pointer to the location for reading and writing
uint8_t cpu_read ( uint16_t address )
{
    if ( address >= 0 && address <= 0x1FFF ) // RAM with mirrors
        return internal_ram[address & 0x07FF] ;

    else if ( address >= 0x2000 && address <= 0x3FFF ) // PPU registers
    {
        address &= 0x2000 ;
        switch ( address ) {
            case 0: // control register
                break ;
            case 1: // mask register
                break;
            case 2: // status register
                break ;
            case 3: // oam address register
                break ;
            case 4: // oam data register
                break ;
            case 5: // ppu scroll register
                break ;
            case 6: // ppu address register
                break ;
            case 7: // ppu data register
                break ;
            default:
                break ;
        }
        return 0;
    }
    else if ( address >= 4000 && address <= 0x401F ) // NES APU and audio
    {
        //TODO : implement the nes apu and io
        return 0;
    }
    else // from 0x4020 to 0xFFFF which is Cartridge space
        return cartridge_mem[address-0x4020] ;
}

void cpu_write ( uint16_t address , uint8_t data )
{
    if ( address >= 0 && address <= 0x1FFF ) // RAM with mirrors
        internal_ram[address&0x07FF] = data ;

    else if ( address >= 0x2000 && address <= 0x3FFF ) // PPU registers
    {
        address &= 0x2000 ;
        switch ( address ) {
            case 0: // control register
                nes_ppu.ctrl.p_ctrl = data ;
                break ;
            case 1: // mask register
                nes_ppu.mask.p_mask = data ;
                break;
            case 2: // status register
                break ;
            case 3: // oam address register
                break ;
            case 4: // oam data register
                break ;
            case 5: // ppu scroll register
                break ;
            case 6: // ppu address register
                break ;
            case 7: // ppu data register
                break ;
            default:
                break ;
        }
        return ;
    }
    else if ( address >= 4000 && address <= 0x401F ) // NES APU and audio
    {
        //TODO : implement the nes apu and io
        return;
    }
    else // from 0x4020 to 0xFFFF which is Cartridge space
        cartridge_mem[address-0x4020] = data ;
}

void load_cartridge ( FILE *fp )
{
    // first we have to read INES header which is 16 bytes
    uint8_t prg_rom , chr_rom ;
    int mapper ;
    fseek ( fp , 4 , SEEK_SET ) ; // skip first 4 bytes "NES" + EOF
    prg_rom = getc(fp) ; // size of PRG ROM in 16KB units (byte 4)
    chr_rom = getc(fp) ; // size of CHR ROM in 8KB units (byte 5)
    mapper = getc(fp) & 0xF0 ; // (byte 6) we get the lower nibble of the mapper number
    mapper |= mapper >> 4 | getc(fp) & 0xF0 ; // (byte 7) we get the higher nibble of the mapper number
    fseek ( fp , 16 , SEEK_SET ) ; // skip the rest for now
    switch ( mapper )
    {
        case 0: // NROM
            for ( int i = 0x3FE0 , mapper = 0 ; mapper < 0x4000 ; mapper++ , i++ ) // copy 16KiB
                cartridge_mem[i] = getc(fp) ;
            --prg_rom ;
            if ( prg_rom ) // NROM-256 it has 2 banks of 16KiB
                for ( int i = 0x7FE0 , mapper = 0 ; mapper < 0x4000 ; mapper++ , i++ )
                    cartridge_mem[i] = getc(fp) ;
            else // NROM-128
                memcpy (cartridge_mem + 0x7FE0 , cartridge_mem + 0x3FE0 , 0x4000 ) ;

            // map character rom or ram to ppu
            for ( int b = 0 ; b < 2 ; ++b )
                for ( int i = 0 ; i < 0x1000 ; i++ ) // copy the 8KB to the pattern tables
                    pattern_tables[b][i] = getc(fp) ;
            break ;
    }
}