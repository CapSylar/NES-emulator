#include "6502.h"
#include "ppu.h"
#include "color_def.h"

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

extern uint32_t screen [] ;

// for sprite 0 hit
bool nextsc_sprite0 ; // if sprite 0 is possible on the next scanline
bool nowsc_sprite0 ;  // if sprite 0 hit is possible for the current scanline

// background rendering

uint16_t hi_pattern_sr; // pattern shift register
uint16_t low_pattern_sr;
uint16_t palette_lo_sr; // palette color shift register
uint16_t  palette_hi_sr; // here we combined the register and the latch

uint8_t nt_byte ;
uint8_t at_byte ;
uint8_t lo_bg_byte ;
uint8_t hi_bg_byte ;

int spr_num ; // hold the number of sprites for the current scanline

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
void update_su () ;
void update_shifters();
void render () ;

int vb_cycles = 0 ;

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
            if ( nes_ppu.mask.up_mask.show_bg || nes_ppu.mask.up_mask.show_spr )
                sprite_evaluation() ;
        }
        else if ( nes_ppu.dot_counter >= 257 && nes_ppu.dot_counter <= 320 ) // load the sprites for the next scanline
        {
            if ( nes_ppu.mask.up_mask.show_spr )
                sprite_loading() ;
        }

        if ( (nes_ppu.dot_counter > 0 && nes_ppu.dot_counter <= 256) ||
                    ( nes_ppu.dot_counter >= 321 && nes_ppu.dot_counter <= 336 ) ) // TODO: check boundaries here
        {
            if ( nes_ppu.scanline >= 0 && nes_ppu.scanline <= 239 && nes_ppu.dot_counter > 0 && nes_ppu.dot_counter <= 256 ) // in a scanline that renders
            {
                render();
            }

            //update the shift registers
            update_shifters();

            if ( nes_ppu.dot_counter != 256 ) // to skip the increment horizontal since it will be wasted
                fetch_cycle() ;

            if ( nes_ppu.dot_counter == 256 ) // end of scanline
            {
                increment_vertical();
            }
            // solely for the pre-render line
            else if ( nes_ppu.scanline == 261 && nes_ppu.dot_counter == 1 ) // pre_render line
            {
                nes_ppu.status.p_status = 0;// clear status register
                vb_cycles = 0 ;
            }
        }
        else if ( nes_ppu.mask.up_mask.show_bg )
        {
            if ( nes_ppu.dot_counter == 257 ) // horizontal(v) = horizontal(t)
            {
                // ppu copies all bits related to horizontal position from t to v
                // which is the lower bit of the nametable select and the 5 bits from coarseX
                nes_ppu.loopy_v.p_reg = nes_ppu.loopy_v.p_reg & (~0x041F) | (nes_ppu.loopy_t.p_reg & 0x041F ) ;
            }
            else if ( nes_ppu.dot_counter >= 280 && nes_ppu.dot_counter <= 304 && nes_ppu.scanline == 261 )
            {
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
        {
            nes_ppu.status.up_status.vblank = 1; // set vertical blank
        }
    }

    ++nes_ppu.dot_counter ;

    if ( nes_ppu.dot_counter > 340 )
    {
        nes_ppu.dot_counter = 0 ;
        ++nes_ppu.scanline;
        nowsc_sprite0 = nextsc_sprite0 ;

        if ( nes_ppu.scanline > 261 )
            nes_ppu.scanline = 0 ;
    }
}

void render () // handles the rendering routine for sprites and background
{
    // render and copy pixel to main display memory

    uint8_t bg_pixel = 0 ;
    uint8_t fg_pixel = 0 ;

    if ( nes_ppu.mask.up_mask.show_bg )
    {
        bg_pixel = ((low_pattern_sr & (0x8000 >> nes_ppu.fine_x)) > 0) +
                     ((hi_pattern_sr & (0x8000 >> nes_ppu.fine_x)) > 0) * 2 ;
    }
    // extract any pixels from active units
    uint8_t current_index = 9; // set to an invalid index

    if ( nes_ppu.mask.up_mask.show_spr )
    {
        for (int i = 0; i < 8; ++i) // search for the first available pixel
        {
            if (!nes_ppu.x_pos[i]) // found an active sprite
            {
                fg_pixel = ((nes_ppu.pattern_lo_sr[i] & 0x80) > 0) |
                           (((nes_ppu.pattern_hi_sr[i] & 0x80) > 0) << 1);

                if (fg_pixel) // if not transparent
                {
                    current_index = i;
                    break;
                }
            }
        }

        // sprite hit check
        if (!(nes_ppu.dot_counter > 0 && nes_ppu.dot_counter <= 8 && !nes_ppu.mask.up_mask.show_spr_l8 &&
              !nes_ppu.mask.up_mask.show_bg_l8))
        {
            if (nowsc_sprite0 && current_index == 0 && bg_pixel &&
                nes_ppu.mask.up_mask.show_bg && nes_ppu.mask.up_mask.show_spr && nes_ppu.dot_counter != 256)
            {
                nes_ppu.status.up_status.sp_0 = true; // sprite 0 hit!
            }
        }
    }

    // now we have a fg pixel that we can use
    bool priority = nes_ppu.at_latch[current_index] & 0x20 ;
    uint16_t used_pixel = 0 ; // this is actually the address of the used pixel

    if ( (!bg_pixel && fg_pixel || bg_pixel && fg_pixel && !priority)) // render sprite
    {
        used_pixel = 0x3F10 | ((nes_ppu.at_latch[current_index] & 0x03) << 2 | fg_pixel) ;
    }
    else // render background
    {
        used_pixel = 0x3F00 | (bg_pixel + ((palette_lo_sr & (0x8000 >> nes_ppu.fine_x)) > 0) * 4 +
                                            ((palette_hi_sr & (0x8000 >> nes_ppu.fine_x)) > 0) * 8 )  ;
    }

    screen[ nes_ppu.scanline * 256 + nes_ppu.dot_counter - 1 ] = color_table[ppu_read ( used_pixel )] ;

    // update the shift registers
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

void update_shifters() // update the shifters used for background rendering
{
    low_pattern_sr <<= 1;
    hi_pattern_sr <<= 1;
    palette_lo_sr <<= 1;
    palette_hi_sr <<= 1;
}

void reset_ppu()
{
    nes_ppu.scanline = 0 ;
    nes_ppu.dot_counter = 0 ;
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
    static int n , m , s_index , num ;
    static uint8_t data;
    static bool copy_spr; // indicated that we are copying a full sprite
    static bool w_dis ; // indicated that writer to secondary OAM are disabled
    static bool halt ; // used to wait till HBLANK is reached
    static bool incr_n ; // increment n

    if ( nes_ppu.dot_counter == 65 ) // called on this dot counter
    {
        n = m = s_index = spr_num = num = 0;
        copy_spr = w_dis = halt = nextsc_sprite0 = false;
    }

    if ( halt )
        return ;

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
                    incr_n = true ;
                    m = 0 ;
                }
                else
                    ++m ;
            }
            else if ( nes_ppu.scanline >= data && nes_ppu.scanline <= (data + (nes_ppu.ctrl.up_ctrl.spr_h ? 15 : 7) ) ) // is in range
            {
                copy_spr = true;
                nes_ppu.status.up_status.sp_v = 1; // sprite overflow

                // increment the indices
                if( m == 3 )
                {
                    incr_n = true ;
                    m = 0 ;
                }
                else
                    ++m ;
                ++num ;
            }
            else // not in range
            {
                incr_n = true ;
                if ( ++m == 4 )
                    m = 0 ;
            }
        }
        else
        {
                data = p_oam[n][m] ;
        }

    }
    else if ( !w_dis )// even cycle: write to secondary OAM
    {
        s_oam[s_index] = data ;
        // evaluate if the y index that we just copied is in range
        if ( copy_spr )
        {
            ++s_index;
            if ( m == 3 ) // we are done
            {
                incr_n = true ;
                m = 0 ;
                copy_spr = false;
                ++spr_num;
                if ( spr_num == 8 ) // max number of sprites has been found
                    w_dis = true ; // disable writes to secondary OAM
            }
            else
                ++m ;
        }
        else if ( nes_ppu.scanline >= data && nes_ppu.scanline <= (data + (nes_ppu.ctrl.up_ctrl.spr_h ? 15 : 7) ) ) // is in range
        {
            if ( n == 0 ) // sprite 0
                nextsc_sprite0 = true;

            copy_spr = true;
            ++s_index ;
            ++m ;
        }
        else
        {
            m = 0 ;
            incr_n = true ;
        }
    }

    // increment n
    if ( incr_n )
    {
        ++n ; // got next p_oam y entry
        if ( (n & 0x3F) == 0 ) // overflow occured
            w_dis = halt = true ;

        incr_n = false ;
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

            if ( count >= spr_num ) // load with empty data since these are unsused units
                nes_ppu.pattern_lo_sr[count] = 0 ; // transparent
            else
            {
                difference = s_oam[count * 4 + 1];

                //check the mode that we are in
                if ( nes_ppu.ctrl.up_ctrl.spr_h ) // 16 * 8 mode
                {
                    if (s_oam[count * 4 + 2] & 0x80) // handles vertical flip
                    {
                        difference = 15 - (nes_ppu.scanline - s_oam[count*4]) ;
                        if ( difference > 7 ) // wrap around to next tile
                        {
                            difference = 16 + difference % 8 ;
                        }
                        address = (( s_oam[count * 4 + 1] & 0x1 ) << 12) + ( (s_oam[count * 4 + 1] & 0xFE) << 4 ) + difference ;
                    }
                    else
                    {
                        difference = (nes_ppu.scanline - s_oam[count*4]) ;
                        if ( difference > 7 ) // wrap around to next tile
                        {
                            difference = 16 + difference % 8 ;
                        }
                        address = (( s_oam[count * 4 + 1] & 0x1 ) << 12) + ( (s_oam[count * 4 + 1] &  0xFE) << 4 ) + difference ;
                    }
                }
                else
                {
                    if (s_oam[count * 4 + 2] & 0x80) // handles vertical flip
                        difference = (difference << 4) | (7 - (nes_ppu.scanline - s_oam[count * 4]));
                    else
                        difference = (difference << 4) | (nes_ppu.scanline - s_oam[count * 4]);

                    address = ((nes_ppu.ctrl.up_ctrl.spr_ts << 12) | difference) ;
                }

                nes_ppu.pattern_lo_sr[count] = ppu_read(address);
                if (nes_ppu.at_latch[count] & 0x40) // if needs horizontal flipping
                    flip_byte(&nes_ppu.pattern_lo_sr[count]);
            }
            break ;
        case 6 : // pattern table tile high
            if ( count >= spr_num )
                nes_ppu.pattern_hi_sr[count] = 0 ;
            else
            {
                nes_ppu.pattern_hi_sr[count] = ppu_read(address + 8);

                if (nes_ppu.at_latch[count] & 0x40) // if needs horizontal flipping
                    flip_byte(&nes_ppu.pattern_hi_sr[count]);

            }

            if (++count == 8)
                count = 0;

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

