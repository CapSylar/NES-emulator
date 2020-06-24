enum ppu_ctrl
{
    nametable_addr = 3 , // base nametable address
    addr_inc = 4 , // VRAM address increment per CPU read/write of PPUDATA
    sprite_patt = 8 , // Sprite pattern table address for 8x8 sprites
    backg_tab = 16, // Background pattern table address (0: $0000; 1: $1000)
    sprite_size = 32 , // Sprite size (0: 8x8 pixels; 1: 8x16 pixels)
    ppu_ms = 64 , // PPU master/slave select
    gen_nmi = 128 , // Generate an NMI at the start of the
} ;


