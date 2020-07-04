#ifndef __PPU_H__
#define __PPU_H__
#include <inttypes.h>

uint8_t ppu_read ( uint16_t address )  ;
void ppu_write ( uint16_t address , uint8_t data ) ;
void ppu_clock () ;
void reset_ppu() ;

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


    } up_mask ;

    uint8_t p_mask ;
};

typedef union ppu_ctrl ppu_ctrl ;
typedef union ppu_status ppu_status ;
typedef union ppu_mask ppu_mask ;

typedef struct {
    bool odd_cycle;
    int dot_counter , scanline ;

    uint8_t shift_reg[8];
    uint8_t latches[8];
    uint8_t x_pos[8];

    // registers
    ppu_ctrl ctrl ;
    ppu_status status ;
    ppu_mask mask ;

    // internals

    uint8_t address_latch ;
    uint8_t ppu_data_buffer ;
    uint16_t ppu_address ;

} ppu_state ;

// color palette rom


#endif


