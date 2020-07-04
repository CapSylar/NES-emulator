#include "6502.h"
#include "ppu.h"

uint8_t pattern_tables[2][0x1000] ;
uint8_t p_oam [256] ;
uint8_t s_oam [64] ;
uint8_t palette[0x20];

extern ppu_state nes_ppu ;

extern bool nmi ;

typedef uint8_t (*reader)(uint16_t) ;
typedef void (*writer)( uint16_t , uint8_t ) ;

extern reader mapper_ppu_read ;
extern writer mapper_ppu_write ;

uint32_t color_table[] = { 0xFF545454 , // ARGB format for SDL
                           0xFF001e74 ,
                           0xFF081090 ,
                           0xFF300088 ,
                           0xFF440064 ,
                           0xFF5c0030 ,
                           0xFF540400 ,
                           0xFF3c1800 ,
                           0xFF202a00 ,
                           0xFF083a00 ,
                           0xFF004000 ,
                           0xFF003c00 ,
                           0xFF00323c ,
                           0xFF00000 , // 0d
                           0xFF000000 , // 0e
                           0xFF000000 , // 0f
                           0xFF989698 ,
                           0xff084cc4 ,
                           0xff3032ec ,
                           0xff5c1ee4 ,
                           0xff8814b0 ,
                           0xffa01464 ,
                           0xff982220 ,
                           0xff783c00 ,
                           0xff545a00 ,
                           0xff287200 ,
                           0xff087c00 ,
                           0xff007628 ,
                           0xff006678 ,
                           0xff000000 , // 1d
                           0xff000000 , // 1e
                           0xff000000 , // 1f
                           0xffeceeec ,
                           0xff4c9aec ,
                           0xff787cec ,
                           0xffb062ec ,
                           0xffe454ec ,
                           0xffec58b4 ,
                           0xffec6a64 ,
                           0xffd48820 ,
                           0xffa0aa00 ,
                           0xff74c400 ,
                           0xff4cd020 ,
                           0xff38cc6c ,
                           0xff38b4cc ,
                           0xff3c3c3c ,
                           0xff000000 , // 2e
                           0xff000000 , // 2f
                           0xffeceeec ,
                           0xffa8ccec ,
                           0xffbcbcec ,
                           0xffd4b2ec ,
                           0xffecaeec ,
                           0xffecaed4 ,
                           0xffecb4b0 ,
                           0xffe4c490 ,
                           0xffccd278 ,
                           0xffb4de78 ,
                           0xffa8e290 ,
                           0xff98e2b4 ,
                           0xffa0d6e4 ,
                           0xffa0a2a0 ,
                           0xff000000 , // 3e
                           0xff000000 //3f
}  ;

uint8_t ppu_read ( uint16_t address ) // 14 lines are actually used
{
    if ( address >= 0x0000 && address <= 0x3EFF ) // ( cartridge dependant )
    {
        return mapper_ppu_read(address);
    }
    else if ( address >= 0x3F00 && address <= 0x3FFF ) // palette RAM indexes and mirrors ( fixed wiring )
    {
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
    { // TODO: implement the mirroring of the background in the palette
        palette[0x1f & address] = data;
    }
}

void ppu_clock ()
{
    if ( nes_ppu.dot_counter >= 340 )
    {
        nes_ppu.dot_counter = 0 ;
        nes_ppu.scanline += 1;

        if ( nes_ppu.scanline > 261 )
            nes_ppu.scanline = 0 ;
    }
    if ( nes_ppu.scanline == 241 )
    {
        if ( nes_ppu.dot_counter == 0 && nes_ppu.ctrl.up_ctrl.nmi )
            nmi = true ;
        else if ( nes_ppu.dot_counter == 1 )
            nes_ppu.status.up_status.vblank = 1 ; // set vertical blank

    }
    else if ( nes_ppu.scanline == 261 && nes_ppu.dot_counter == 1 )
    {
        // clear ppu status
        nes_ppu.status.p_status = 0 ;// clear status register
    }

    ++nes_ppu.dot_counter ;
}

void reset_ppu()
{
    nes_ppu.scanline = 0 ;
    nes_ppu.dot_counter = 0 ;
}
