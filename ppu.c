#include "6502.h"
#include "ppu.h"

uint8_t pattern_tables[2][0x1000] ;
uint8_t p_oam [64][4] ; // primary
uint8_t s_oam [32] ; // secondary
uint8_t palette[0x20];

extern ppu_state nes_ppu ;

extern bool nmi ;
extern bool dma_on ; // indicates if a DMA transfter is in progession

extern int cycle_count; // system cycle count

bool dma_started ;
uint16_t dma_initial;
uint16_t dma_current;
uint8_t oam_start ; // marks the start of the OAM DMA address

typedef uint8_t (*reader)(uint16_t) ;
typedef void (*writer)( uint16_t , uint8_t ) ;

extern reader mapper_ppu_read ;
extern writer mapper_ppu_write ;

extern uint32_t screen [256 * 240] ;

// for sprite 0 hit
bool sprite0 ; // if sprite 0 is in secondary OAM thus it will be loaded into the first unit

// background rendering

uint16_t hi_pattern_sr; // pattern shift register
uint16_t low_pattern_sr;
uint16_t palette_lo_sr; // palette color shift register
uint16_t  palette_hi_sr; // here we combined the register and the latch

uint8_t nt_byte ;
uint8_t at_byte ;
uint8_t lo_bg_byte ;
uint8_t hi_bg_byte ;

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
                           0xFF000000 , // 0d
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
        palette[0x001f & address] = data;
    }
}

void ppu_oam_write ( uint8_t address , uint8_t data )
{
    // TODO: implement corruption as per the documentation
    p_oam[(address & 0xFC) >> 2][address & 0x03] = data ;
    ++nes_ppu.oam_address ;
}
uint8_t ppu_oam_read( uint8_t address )
{
    // TODO: implement check to see if we are in blank
    return p_oam[(address & 0xFC) >> 2][address & 0x03] ;
}

void dma_start ( uint8_t page_start )
{
    // start the dma process
    dma_on = true ; // suspends the cpu
    dma_started = false ;
    dma_current = dma_initial = page_start << 8 ;
    oam_start = nes_ppu.oam_address ;
}

void dma_clock()
{
    static uint8_t read_data;
    static bool last_read ;

    if ( (dma_initial & 0x00FF) == (dma_current & 0x00FF) ) // at beginning or end
    {
        if ( !dma_started )
        {
            if (cycle_count % 2 == 1) // skip first odd clock
                return;
            dma_started = true;
        }
        else // we are at the end and we did an extra read as needed
        {
            dma_on = dma_started = false;
        }
    }

    if ( !last_read ) // last operation was not a read
    {
        read_data = cpu_read(dma_current++);
       last_read = true;
    }
    else
    {
        ppu_oam_write(oam_start++, read_data);
        last_read = false;
    }
}

void fetch_cycle() ;
void increment_vertical () ;
void sprite_evaluation();
void sprite_loading() ;


void update_shifters();

// TODO: rewrite ppu_clock function
void ppu_clock ()
{
    if ( nes_ppu.scanline >= 0 && nes_ppu.scanline <= 239 || nes_ppu.scanline == 261 ) // the visible frame or pre-render
    {
        if ( nes_ppu.dot_counter > 0 && nes_ppu.dot_counter <= 64 && nes_ppu.scanline != 261 ) // Secondary OAM clear
        {
            s_oam[(nes_ppu.dot_counter-1)/2] = 0xFF ;
        }
        else if ( nes_ppu.dot_counter >= 65 && nes_ppu.dot_counter <= 256 && nes_ppu.scanline != 261 ) // sprite evaluation for next scanline
        {
            sprite_evaluation() ;
        }
        else if ( nes_ppu.dot_counter >= 257 && nes_ppu.dot_counter <= 320 ) // load the sprites for the next scanline
        {
            sprite_loading() ;
        }

        if ( (nes_ppu.dot_counter > 0 && nes_ppu.dot_counter <= 256) ||
                    ( nes_ppu.dot_counter >= 321 && nes_ppu.dot_counter <= 336 ) ) // TODO: check boundaries here
        {
            if ( nes_ppu.scanline >= 0 && nes_ppu.scanline <= 239 && nes_ppu.dot_counter > 0 && nes_ppu.dot_counter <= 256 ) // in a scanline that renders
            {

                // render and copy pixel to main display memory

                uint8_t bg_pixel = ((low_pattern_sr & (0x8000 >> nes_ppu.fine_x) ) >> 15) + ((hi_pattern_sr & 0x8000) >> 15 )* 2
                        + ((palette_lo_sr & 0x8000) >> 15 ) * 4 + ((palette_hi_sr & 0x8000 )>> 15 ) * 8 ;

                // extract any pixels from active units
                uint8_t current_index = 0;
                uint8_t fg_pattern = 0 ;

                for ( int i = 0 ; i < 8 ; ++i )
                {
                    if ( !nes_ppu.x_pos[i] ) // found an active sprite
                    {
                        current_index = i;
                        fg_pattern = ((nes_ppu.pattern_lo_sr[i] & 0x80) > 0) | (((nes_ppu.pattern_hi_sr[i] & 0x80) > 0) << 1 );

                        if ( fg_pattern ) // if not transparent
                            break ;
                    }
                }

                if ( sprite0 && !current_index && bg_pixel )
                    nes_ppu.status.up_status.sp_0 = true ; // sprite 0 hit!

                // now we have a fg pixel that we can use
                bool priority = nes_ppu.at_latch[current_index] & 0x20 ;
                uint16_t used_pixel = 0 ;

                if (!bg_pixel && fg_pattern || bg_pixel && fg_pattern && !priority ) // render sprite
                {
                    fg_pattern = (nes_ppu.at_latch[current_index] & 0x03) << 2 | fg_pattern ;
                    used_pixel = 0x3F10 | fg_pattern ;
                }
                else // render background
                {
                    used_pixel = 0x3F00 | bg_pixel ;
                }

                screen[ nes_ppu.scanline * 256 + nes_ppu.dot_counter - 1 ] =
                        color_table[ppu_read ( used_pixel )] ;

                for ( int i = 0 ; i < 8 ; ++i ) // update the sprite latches and registers
                {
                    if ( nes_ppu.x_pos[i] )
                        --nes_ppu.x_pos[i] ;
                    else // has been active on the current scanline, shift
                    {
                        nes_ppu.pattern_hi_sr[i] <<= 1 ;
                        nes_ppu.pattern_lo_sr[i] <<= 1 ;
                    }
                }
            }
            //update the shift registers

            update_shifters();

            if ( nes_ppu.dot_counter != 256 )
                fetch_cycle() ;

            if ( nes_ppu.dot_counter == 256 ) // end of scanline
            {
                increment_vertical();
            }

            // solely for the pre-render line
            else if ( nes_ppu.scanline == 261 && nes_ppu.dot_counter == 1 ) // pre_render line
            {
                nes_ppu.status.p_status = 0;// clear status register
            }
        }
        else if ( nes_ppu.mask.up_mask.show_bg )
        {
            if ( nes_ppu.dot_counter == 257 ) // horizontal(v) = horizontal(t)
            {
                // ppu copies all bits related to horizontal position from t to v
                // which is the lower bit of the nametable select and the 5 bits from coarseX
                //nes_ppu.loopy_v.p_reg |= (nes_ppu.loopy_t.p_reg & 0x41F) ;
                nes_ppu.loopy_v.p_reg = nes_ppu.loopy_v.p_reg & (~0x041F) | (nes_ppu.loopy_t.p_reg & 0x041F ) ;
            }
            else if ( nes_ppu.dot_counter >= 280 && nes_ppu.dot_counter <= 304 && nes_ppu.scanline == 261 )
            {
                //nes_ppu.loopy_v.p_reg |= (0xBE0 & nes_ppu.loopy_t.p_reg) ;
                nes_ppu.loopy_v.p_reg =
                        nes_ppu.loopy_v.p_reg & ~0x7BE0 | nes_ppu.loopy_t.p_reg & 0x7BE0; // vertical(b) = vertical(t)
            }
        }
    }
    else if ( nes_ppu.scanline == 241 )
    {
        if ( nes_ppu.dot_counter == 0 && nes_ppu.ctrl.up_ctrl.nmi )
            nmi = true ;
        else if ( nes_ppu.dot_counter == 1 )
            nes_ppu.status.up_status.vblank = 1 ; // set vertical blank
    }

    ++nes_ppu.dot_counter ;

    if ( nes_ppu.dot_counter > 340 )
    {
        nes_ppu.dot_counter = 0 ;
        nes_ppu.scanline += 1;

        if ( nes_ppu.scanline > 261 )
            nes_ppu.scanline = 0 ;
    }
}

void update_shifters()
{
    low_pattern_sr <<= 1;
    hi_pattern_sr <<= 1;
    palette_lo_sr <<= 1;
    palette_hi_sr <<= 1;

}

void reset_ppu()
{
    nes_ppu.scanline = 261 ;
    nes_ppu.dot_counter = 0 ;

    for ( int i = 0 ; i < 8 ; ++i )
    {
        nes_ppu.pattern_hi_sr[i] = nes_ppu.pattern_lo_sr[i] = nes_ppu.at_latch[i] = 0 ;
    }
}

void fetch_cycle()
{ // does a nt,at or bg byte fetch
    switch( (nes_ppu.dot_counter - 1) % 8) // handles nt , at , low bg and high bg fetches
    {
        case 0: // nt byte fetch, 2 cycles
            nt_byte = ppu_read( 0x2000 | nes_ppu.loopy_v.p_reg & 0x0FFF ) ;
            break;
        case 2: // at byte fetch , 2 cycles
            ;
            uint16_t v = nes_ppu.loopy_v.p_reg ;
            at_byte = ppu_read( 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07) ) ;
            // extract 2 bits form this byte
            if ( v & 0x0040 )
                at_byte >>= 4 ;
            if ( v & 0x0002 )
                at_byte >>= 2 ;
            at_byte &= 0x03 ;
            break ;
        case 4: // low bg tile fetch , 2 cycles
            lo_bg_byte = ppu_read( (nes_ppu.ctrl.up_ctrl.bg_ts << 12) + ( ( uint16_t) nt_byte << 4) + nes_ppu.loopy_v.up_reg.fine_y ) ; // * 16
            break ;
        case 6: // high bg tile fetch , 2 cycles
            hi_bg_byte = ppu_read( (nes_ppu.ctrl.up_ctrl.bg_ts << 12) + ( (uint16_t ) nt_byte << 4) + nes_ppu.loopy_v.up_reg.fine_y + 8 ) ; // * 16
            break;
        case 7: // increment horizontal (v)
            if ( nes_ppu.mask.up_mask.show_bg )
            {
                if ( nes_ppu.loopy_v.up_reg.coarse_x == 31 )
                {
                    nes_ppu.loopy_v.up_reg.coarse_x = 0;
                    nes_ppu.loopy_v.up_reg.nn_sel ^= 0x01 ; // switch horizontal nametable
                }
                else
                    nes_ppu.loopy_v.up_reg.coarse_x += 1 ;
            }

            // copy the bytes into the shift registers
            hi_pattern_sr = hi_pattern_sr & 0xFF00 | hi_bg_byte ;
            low_pattern_sr = low_pattern_sr & 0xFF00 | lo_bg_byte ;
            palette_lo_sr = palette_lo_sr & 0xFF00 | ((at_byte & 0x01) ? 0xFF : 0x00) ;
            palette_hi_sr = palette_hi_sr & 0xFF00 | ((at_byte & 0x02) ? 0xFF : 0x00) ;

            //TODO: shift into palette shift registers
            break ;
    }
}
void increment_vertical ()
{
    if ( !nes_ppu.mask.up_mask.show_bg )
        return;

    // increment vertical
    if ( nes_ppu.loopy_v.up_reg.fine_y != 7 ) // fine Y < 7
        ++nes_ppu.loopy_v.up_reg.fine_y ;
    else
    {
        nes_ppu.loopy_v.up_reg.fine_y = 0 ;
        // now we have to increment coarse Y
        if ( nes_ppu.loopy_v.up_reg.coarse_y == 29 ) // last line before attribute table,jump over
        {
            nes_ppu.loopy_v.up_reg.coarse_y = 0;
            // move down to the neighboring vertical table
            nes_ppu.loopy_v.up_reg.nn_sel += 2 ;
        }
        else if ( nes_ppu.loopy_v.up_reg.coarse_y == 31 ) // handles the case where coarseY is set out of bounds
            nes_ppu.loopy_v.up_reg.coarse_y = 0;
        else
            ++nes_ppu.loopy_v.up_reg.coarse_y;
    }
}

void sprite_evaluation() // TODO: rewrite this function for the love of Jesus
{
    static int n , m , s_index , spr_num , num ;
    static uint8_t data;
    static bool copy_spr; // indicated that we are copying a full sprite
    static bool w_dis ; // indicated that writer to secondary OAM are disabled
    static bool halt ;

    if ( nes_ppu.dot_counter == 65 ) // called on this dot counter, dirty tricks
    {
        n = m = s_index = spr_num = num = 0;
        copy_spr = w_dis = halt = sprite0 = false;
    }

    if ( halt )
        return ; // first corner we cut

    if ( nes_ppu.dot_counter % 2 ) // odd cycles: read from primary OAM
    {
        if ( w_dis ) // in this "mode" we seach primary OAM for sprites that also appear so we can see the overflow flag
        {
            data = p_oam[n][m] ; // read as Y coordinate

            if ( copy_spr )
            {
                ++num ;
                if ( num == 4 )
                {
                    num = 0 ;
                    copy_spr = false;
                }

                if( m == 3 ) // handle carry in case
                {
                    ++n ;
                    if ( (n & 0x3F) == 0 ) // overflow occured
                        w_dis = halt = true ;
                    m = 0 ;
                }
                else
                    ++m ;
            }
            else if ( nes_ppu.scanline >= data && nes_ppu.scanline <= (data + 8) ) // is in range
            {
                copy_spr = true;
                nes_ppu.status.up_status.sp_v = 1; // sprite overflow

                // increment the indices
                if( m == 3 )
                {
                    ++n ;
                    if ( (n & 0x3F) == 0 ) // overflow occured
                        w_dis = halt = true ;
                    m = 0 ;
                }
                else
                    ++m ;
                ++num ;
            }
            else // not in range
            {
                ++n ;
                if ( (n & 0x3F) == 0 ) // overflow occured
                    w_dis = halt = true ;
                if ( ++m == 4 )
                    m = 0 ;
            }
        }
        else
        {
            if ( copy_spr ) // we are copying the 4 bytes of a sprite
                data = p_oam[n][++m];
            else
                data = p_oam[n][0] ;
        }

    }
    else // even cycle: write to secondary OAM
    {
        if ( !w_dis )
        {
            s_oam[s_index] = data ;
            // evaluate if the y index that we just copied is in range
            if ( copy_spr )
            {
                ++s_index;
                if ( m == 3 ) // we are done
                {
                    ++n ;
                    if ( (n & 0x3F) == 0 ) // overflow occured
                        w_dis = halt = true ;
                    m = 0 ;
                    copy_spr = false;
                    ++spr_num;
                    if ( spr_num == 8 ) // max number of sprites has been found
                        w_dis = true ; // disable writes to secondary OAM
                }
            }
            else if ( nes_ppu.scanline >= data && nes_ppu.scanline <= (data + 7) )
            {
                if ( n == 0 ) // sprite 0!!
                    sprite0 = true ;

                copy_spr = true;
                ++s_index ;
            }
            else
            {
                ++n ; // got next p_oam y entry
                if ( (n & 0x3F) == 0 ) // overflow occured
                    w_dis = halt = true ;
            }
        }
    }
}

int count; // counts the number of sprites we handled
void flip_byte ( uint8_t *x ) ;

void sprite_loading() // no support for 8X16 sprites for now
{
    static uint16_t difference ;
    static uint16_t address ;

    switch ( (nes_ppu.dot_counter-1) % 8)
    {
        case 0: // garbage nametable byte
            break ;
        case 2: // garbage nametable byte, first tick
            nes_ppu.at_latch[count] = s_oam[count*4 + 2] ;
            break ;
        case 3: // second tick
            nes_ppu.x_pos[count] = s_oam[count*4 + 3] ; // load x position
            break ;
        case 4 : // pattern table tile low
            difference = s_oam[count*4 + 1] ;
            difference = (difference << 4 )| ( nes_ppu.scanline - s_oam[count * 4] );

            nes_ppu.pattern_lo_sr[count] = ppu_read( address = ((nes_ppu.ctrl.up_ctrl.spr_ts << 12) | difference) ) ;
            if ( nes_ppu.at_latch[count] & 0x40 ) // if needs horizontal flipping
                flip_byte( &nes_ppu.pattern_lo_sr[count]) ;
            break ;
        case 6 : // pattern table tile high

            nes_ppu.pattern_hi_sr[count] = ppu_read( address + 8 ) ;

            if ( nes_ppu.at_latch[count] & 0x40 ) // if needs horizontal flipping
                flip_byte( &nes_ppu.pattern_hi_sr[count] ) ;

            if ( ++count == 8 )
                count = 0 ;
            break ;
    }
}

void flip_byte ( uint8_t *x )
{
    *x = (*x & 0xF0) >> 4 | (*x & 0x0F) << 4; //ty, stackoverflow
    *x = (*x & 0xCC) >> 2 | (*x & 0x33) << 2;
    *x = (*x & 0xAA) >> 1 | (*x & 0x55) << 1;
}

/* source wiki.nesdev.om

  NN 1111 YYY XXX          address of the attribute
 || |||| ||| +++-- high 3 bits of coarse X (x/4)
 || |||| +++------ high 3 bits of coarse Y (y/4)
 || ++++---------- attribute offset (960 bytes)
 ++--------------- nametable select
 */

/*
yyy NN YYYYY XXXXX       layout of t and v registers
||| || ||||| +++++-- coarse X scroll
||| || +++++-------- coarse Y scroll
||| ++-------------- nametable select
+++----------------- fine Y scroll


43210  the mechanism by which we choose the color for a pixel
|||||
|||++- Pixel value from tile data
|++--- Palette number from attribute table or OAM
+----- Background/Sprite select
*/

