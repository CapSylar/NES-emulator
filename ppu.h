#ifndef __PPU_H__
#define __PPU_H__
#include <inttypes.h>

uint8_t ppu_read ( uint16_t address )  ;
void ppu_write ( uint16_t address , uint8_t data ) ;
void ppu_clock () ;
void reset_ppu () ;
void ppu_oam_write ( uint8_t address , uint8_t data ) ;
uint8_t ppu_oam_read( uint8_t address ) ;
void dma_start ( uint8_t page_start ) ;
void dma_clock() ;


typedef union loopy_reg
{
    struct
    {
        uint16_t coarse_x : 5 ;
        uint16_t coarse_y : 5 ;
        uint16_t nn_sel : 2 ;
        uint16_t fine_y : 3 ;
        uint16_t unused : 1 ;
    } up_reg ;

    uint16_t p_reg ;

} loopy_reg ;
union ppu_ctrl
{
    struct
    {
        uint8_t name_sel : 2 ; // nametable select (NN)
        uint8_t incr : 1 ; //increment mode (I)
        uint8_t spr_ts : 1; // sprite tile select (S)
        uint8_t bg_ts : 1; // background tile select (B)
        uint8_t spr_h : 1; // sprite height (H)
        uint8_t mas_sla : 1; // PPU master/slave (P)
        uint8_t nmi : 1; // NMI enable  (V)

    } up_ctrl ;

    uint8_t p_ctrl ;
};

union ppu_status
{
    struct
    {
        uint8_t unsused : 5 ;
        uint8_t sp_v : 1 ; //sprite overflow (O); read resets write pair for $2005/$2006
        uint8_t sp_0 : 1 ; // sprite 0 hit (S)
        uint8_t vblank : 1 ; // vblank (V)

    } up_status ;

    uint8_t p_status ;
};

union ppu_mask
{
    struct
    {
        uint8_t grey : 1;
        uint8_t show_bg_l8 : 1;
        uint8_t show_spr_l8 : 1;
        uint8_t show_bg : 1;
        uint8_t show_spr : 1;
        uint8_t emph_red : 1;
        uint8_t emph_green : 1;
        uint8_t emph_blue : 1;

    } up_mask ;

    uint8_t p_mask ;
};

typedef union ppu_ctrl ppu_ctrl ;
typedef union ppu_status ppu_status ;
typedef union ppu_mask ppu_mask ;

typedef struct {
    bool odd_cycle;
    int dot_counter , scanline ;

    //for the sprites

    uint8_t pattern_hi_sr[8]; // hods the high byte of the pattern data
    uint8_t pattern_lo_sr[8]; // hods the low byte of the pattern data
    uint8_t at_latch[8]; // holds the attribute bytes
    uint8_t x_pos[8]; // hold the x positions

    // registers
    ppu_ctrl ctrl ;
    ppu_status status ;
    ppu_mask mask ;

    //OAM

    uint8_t oam_address ;

    // internals

    bool w_toggle ;
    uint8_t ppu_data_buffer ;
    uint16_t ppu_address ;
    loopy_reg loopy_t , loopy_v ;
    uint8_t fine_x ;

} ppu_state ;

// color palette rom

#endif


