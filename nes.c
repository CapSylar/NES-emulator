#include "6502.h"
#include "ppu.h"
#include "mapper.h"

uint8_t internal_ram[0x800] ; // NES only has 2KB of internal RAM from 0000 to 07FFF
extern ppu_state nes_ppu ;

bool nmi ;

// function pointers for the current used mapper
reader mapper_cpu_read , mapper_ppu_read ;
writer mapper_cpu_write , mapper_ppu_write ;

// we can't write a single function for the read and write because some reads and writes trigger special behavior
// on the ships that can't be predicted if we simply return a pointer to the location for reading and writing
uint8_t cpu_read ( uint16_t address )
{
    uint8_t ret = 0 ;
    if ( address >= 0 && address <= 0x1FFF ) // RAM with mirrors
        return internal_ram[address & 0x07FF] ;

    else if ( address >= 0x2000 && address <= 0x3FFF ) // PPU registers
    {
        address &= 0x0007 ;
        switch ( address )
        {
            case 0: // control register
                break ;
            case 1: // mask register
                break;
            case 2: // status register
                ret = (nes_ppu.status.p_status & 0xE0) | (nes_ppu.ppu_data_buffer & 0x1F);
                // include last 5 bits of the data buffer
                nes_ppu.address_latch = 0 ;
                nes_ppu.status.up_status.vblank = 0 ;// reset vblank
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
                ret = nes_ppu.ppu_data_buffer ;
                nes_ppu.ppu_data_buffer = ppu_read( nes_ppu.ppu_address ) ;

                // when reading from palette memory no dummy read is required so we return immediately
                if ( nes_ppu.ppu_address >= 0x3F00 ) // palette range
                    ret = nes_ppu.ppu_data_buffer ;

                nes_ppu.ppu_address += ( nes_ppu.ctrl.up_ctrl.incr ) ? 32 : 1 ;

                break ;
            default:
                fprintf( stderr , "error occurred in ppu read") ;
                break ;
        }
        return ret;
    }
    else if ( address >= 4000 && address <= 0x401F ) // NES APU and audio
    {
        //TODO : implement the nes apu and io
        return 0;
    }
    else // from 0x4020 to 0xFFFF which is Cartridge space
        return mapper_cpu_read(address);
}

void cpu_write ( uint16_t address , uint8_t data )
{
    if ( address >= 0 && address <= 0x1FFF ) // RAM with mirrors
        internal_ram[ address & 0x07FF ] = data ;

    else if ( address >= 0x2000 && address <= 0x3FFF ) // PPU registers
    {
        address &= 0x0007 ;
        switch ( address )
        {
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
                if ( !nes_ppu.address_latch ) // write the low byte
                {
                    nes_ppu.address_latch = 1;
                    nes_ppu.ppu_address = ( nes_ppu.ppu_address & 0x00FF ) | (data << 8 ); // fill the high byte
                } else // write the high byte
                {
                    nes_ppu.address_latch = 0 ;
                    //nes_ppu.address_latch = ( nes_ppu.ppu_address & 0xFF00 ) | data ; // fill the low byte
                    nes_ppu.ppu_address = ( nes_ppu.ppu_address & 0xFF00 ) | data ; // fill the low byte ;
                }
                break ;
            case 7: // ppu data register
                ppu_write( nes_ppu.ppu_address , data ) ;
                nes_ppu.ppu_address += ( nes_ppu.ctrl.up_ctrl.incr ) ? 32 : 1 ;
                break ;
            default:
                fprintf( stderr , "error occurred in ppu write") ;
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
        mapper_cpu_write ( address , data ) ;
}

void clock_system()
{
    static int cycle_count;
    if ( cycle_count % 3 == 0 )
    {
        cpu_clock();
        cycle_count = 0;
    }

    ppu_clock() ;
    ++cycle_count ;
}