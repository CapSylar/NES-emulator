// this file represents the 'bus' of a NES as well as the read/write interface between the different components

#include <SDL_scancode.h>
#include <SDL2/SDL.h>

#include "6502.h"
#include "ppu.h"
#include "mapper.h"
#include "controller.h"

uint8_t internal_ram[0x800] ; // NES only has 2KB of internal RAM from 0000 to 07FFF
extern ppu_state nes_ppu ;
extern uint8_t palette[] ;

bool nmi ;
bool dma_on ;
extern bool poll ;

// global system cycle count 0-3
int cycle_count = 1;

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
                nes_ppu.w_toggle = 0 ;
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
                nes_ppu.ppu_data_buffer = ppu_read( nes_ppu.loopy_v.p_reg ) ;

                // when reading from palette memory no dummy read is required so we return immediately
                if ( nes_ppu.loopy_v.p_reg >= 0x3F00 ) // palette range
                    ret = nes_ppu.ppu_data_buffer ;

                nes_ppu.loopy_v.p_reg += ( nes_ppu.ctrl.up_ctrl.incr ) ? 32 : 1 ;
                break ;
            default:
                fprintf( stderr , "error occurred in ppu read") ;
                break ;
        }
        return ret;
    }
    else if ( address >= 4000 && address <= 0x401F ) // NES APU and audio
    {
        if (address == 0x4016)
            return read_controller1() ;

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
                nes_ppu.loopy_t.up_reg.nn_sel = data ; // 2 rightmost bits as NN ( nametable select )
                break ;
            case 1: // mask register
                nes_ppu.mask.p_mask = data ;
                break;
            case 2: // status register
                break ;
            case 3: // oam address register
                nes_ppu.oam_address = data ;
                break ;
            case 4: // oam data register
                ppu_oam_write( address , data );
                break ;
            case 5: // ppu scroll register
                //printf( "first write : %d , at ppu scanline %d , at ppu dot count: %d\n" , !nes_ppu.w_toggle , nes_ppu.scanline , nes_ppu.dot_counter);
                if ( !nes_ppu.w_toggle ) // first write
                {
                    nes_ppu.loopy_t.up_reg.coarse_x = (data >> 3) & 0x1F; // set the fine x scroll
                    nes_ppu.fine_x = data & 0x07 ; // last 3 bits are fine x scroll s
                    nes_ppu.w_toggle = 1 ;
                }
                else // second write
                {
                    nes_ppu.loopy_t.up_reg.fine_y = data ; // rightmost 3 bits are fine Y
                    nes_ppu.loopy_t.up_reg.coarse_y = data >> 3 ; // 5 bits for coarse Y
                    nes_ppu.w_toggle = 0;
                }
                break ;
            case 6: // ppu address register
                if ( !nes_ppu.w_toggle ) // first write
                {
                    nes_ppu.w_toggle = 1;
                    nes_ppu.loopy_t.p_reg = ( nes_ppu.loopy_t.p_reg & 0x00FF ) | ( (data & 0x3F)  << 8 ); // fill the high byte
                    // clear the highest byte
                }
                else // second write
                {
                    nes_ppu.w_toggle = 0 ;
                    nes_ppu.loopy_v.p_reg = nes_ppu.loopy_t.p_reg = ( nes_ppu.loopy_t.p_reg & 0xFF00 ) | data ; // fill the low byte
                }
                break ;
            case 7: // ppu data register
                ppu_write( nes_ppu.loopy_v.p_reg , data ) ;
                nes_ppu.loopy_v.p_reg += ( nes_ppu.ctrl.up_ctrl.incr ) ? 32 : 1 ;
                break ;
            default:
                fprintf( stderr , "error occurred in ppu write") ;
                break ;
        }
        return ;
    }
    else if ( address >= 4000 && address <= 0x401F ) // NES APU and audio
    {
        if ( address == 0x4014 ) // OAM DMA
        {
            dma_on = true ;
            dma_start( data ) ;
        }

        else if (address == 0x4016)
            write_cntrl( data ) ;

        //TODO : implement the nes apu and io
        return;
    }
    else // from 0x4020 to 0xFFFF which is Cartridge space
        mapper_cpu_write ( address , data ) ;
}

uint8_t ppu_read ( uint16_t address ) // 14 lines are actually used
{
    if ( address >= 0x0000 && address <= 0x3EFF ) // ( cartridge dependant )
    {
        return mapper_ppu_read(address);
    }
    else if ( address >= 0x3F00 && address <= 0x3FFF ) // palette RAM indexes and mirrors ( fixed wiring )
    {
        //$3F10/$3F14/$3F18/$3F1C
        if ( address == 0x3F10 || address == 0x3F14 || address == 0x3F18 || address == 0x3F1C )
            return palette[0x0F & address] ;
        else if ( address == 0x3F04 || address == 0x3F08 || address == 0x3F0C )
            return palette[0] ;
        return palette[0x1f & address] ;
    }
}

void ppu_write ( uint16_t address , uint8_t data )
{
    if ( address >= 0x0000 && address <= 0x3EFF )
    {
        mapper_ppu_write( address , data ) ;
    }
    else if ( address >= 0x3F00 && address <= 0x3FFF ) // palette RAM indexes and mirrors
    {
        if ( address == 0x3F10 || address == 0x3F14 || address == 0x3F18 || address == 0x3F1C ) // pallette mirroring
            palette[0x0F & address] = data ;
        else if ( address == 0x3F04 || address == 0x3F08 || address == 0x3F0C )
            palette[0x3F00] = data;
        else
            palette[0x001f & address] = data;
    }
}

void clock_system()
{
    if ( cycle_count % 3 == 0 )
    {
        // check for user input

        if ( poll )
        {
            uint8_t mapped_key = 0 ;
            const uint8_t *state = SDL_GetKeyboardState(NULL) ;

            mapped_key = (state[SDL_SCANCODE_D] << 7) + (state[SDL_SCANCODE_A] << 6) + (state[SDL_SCANCODE_S] << 5) +
                    (state[SDL_SCANCODE_W] << 4) + (state[SDL_SCANCODE_M] << 3) + (state[SDL_SCANCODE_N] << 2) +
                    (state[SDL_SCANCODE_K] << 1) + state[SDL_SCANCODE_J] ;

            update_cntrl1(mapped_key) ;
        }
        if ( dma_on )
        {
            dma_clock() ;
        }
        else
        {
            cpu_clock();
        }
    }

    ppu_clock() ;

    if ( cycle_count == 6 ) // we go 1 2 |3| 4 5 |6| => 1
        cycle_count = 1 ;
    else
        ++cycle_count ;
}