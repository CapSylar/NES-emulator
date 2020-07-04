// this code handles the creation of the gui and the glue code to display the internals of the system

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

#include "SDL_ttf.h"
#include <stdbool.h>

#include "interface.h"
#include "mapper.h"
#include "ppu.h"

extern reader mapper_cpu_read , mapper_ppu_read ;
extern writer mapper_cpu_write , mapper_ppu_write ;

extern uint32_t color_table[] ;

extern bool halt;
extern uint8_t palette[0x20];

uint32_t pattern0_dp [W * H] ;
uint32_t pattern1_dp [W * H] ;
uint32_t screen [256 * 240] ;

SDL_Texture *texture;
SDL_Texture *texture1;
SDL_Texture *texture2;

SDL_Surface *surface;
SDL_Surface *surface1;
SDL_Surface *surface2;

SDL_Renderer *renderer ;

SDL_Rect pat0 , pat1 , screen0 ;

void copy_main_display();

int init_interface()
{
    if ( SDL_Init ( SDL_INIT_AUDIO ) != 0 )
    {
        fprintf(stderr, "%s\n", SDL_GetError() );
        return 1 ;
    }

    if ( TTF_Init() == -1 )
    {
        fprintf(stderr , "error: %s", SDL_GetError());
        exit(1);
    }

    // fuck this mess TODO: fix the mess in the interface
    pat0 = ( SDL_Rect ) { 0 , 0 , W * SCALE_F, H * SCALE_F } ;
    pat1 = ( SDL_Rect ) { W * SCALE_F , 0 , W * SCALE_F , H * SCALE_F } ;
    screen0 = ( SDL_Rect ) { 0 , H * SCALE_F , 256 * SCALE_F , 240 * SCALE_F } ;


    SDL_Window *window = SDL_CreateWindow( "testing", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, 2 * W * SCALE_F  , (H + 240) * SCALE_F ,
                                          0);
    renderer = SDL_CreateRenderer(window, -1, 0);

    return 0 ;
}

void update_interface ()
{
    halt = false ;

    SDL_Event event;
    while (SDL_PollEvent(&event))
        switch (event.type)
        {
            case SDL_QUIT: // handles x button on the upper right of the window
                halt = true;
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:; // put there because the C standard does not allow for declarations after labels only statements
                bool state = (event.type == SDL_KEYDOWN);
                uint8_t mapped_key;
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_ESCAPE :
                        halt = true;
                        break;
                }
        }

    if ( halt )
    {
        SDL_Quit();
    }
    else
    {
        copy_pattern_table() ;
        copy_main_display();

        surface = SDL_CreateRGBSurfaceFrom( pattern0_dp , W , H , 32 , W * 4 ,
                                            0x00FF0000 ,0x0000FF00 , 0x000000FF , 0xFF000000) ;
        surface1 = SDL_CreateRGBSurfaceFrom( pattern1_dp , W , H , 32 , W * 4 ,
                                            0x00FF0000 ,0x0000FF00 , 0x000000FF , 0xFF000000) ;
        surface2 = SDL_CreateRGBSurfaceFrom( screen , 256 , 240 , 32 , 256 * 4 ,
                                             0x00FF0000 ,0x0000FF00 , 0x000000FF , 0xFF000000) ;
        texture = SDL_CreateTextureFromSurface( renderer , surface ) ;
        texture1 = SDL_CreateTextureFromSurface( renderer , surface1 ) ;
        texture2 = SDL_CreateTextureFromSurface( renderer , surface2 ) ;
        SDL_FreeSurface(surface) ;
        SDL_FreeSurface(surface1) ;
        SDL_FreeSurface(surface2) ;

        SDL_RenderCopy( renderer , texture , NULL , &pat0 ) ;
        SDL_RenderCopy( renderer , texture1 , NULL , &pat1 ) ;
        SDL_RenderCopy( renderer , texture2 , NULL , &screen0 ) ;
        SDL_RenderPresent( renderer ) ;

    }
}

void copy_main_display()
{
    // this is where we render the display for now
    for ( int y = 0 ; y < 30 ; ++y )
    {
        for (int x = 0; x < 32 ; ++x)
        {
            uint8_t offset = mapper_ppu_read( 0x2000 + y * 32 + x );
            for (uint16_t row = 0; row < 8; ++row)
            {
                uint8_t tile_lsb0 = mapper_ppu_read(0x1000 + offset * 16 + row); // 16 to skip pane1
                uint8_t tile_msb0 = mapper_ppu_read(0x1000 + offset * 16 + row + 8);

                for (uint16_t col = 0; col < 8; ++col)
                {
                    uint8_t pixel0 = (tile_lsb0 & 0x01) + (tile_msb0 & 0x01) * 2;
                    tile_lsb0 >>= 1;
                    tile_msb0 >>= 1;

                    screen[x * 8 + (7 - col) + (y * 8 + row) * 256] = color_table[ppu_read(0x3F00 + pixel0)];
                }
            }
        }
    }
}
void copy_pattern_table()
{
    for ( uint16_t tileY = 0 ; tileY < 16 ; ++tileY )
        for ( uint16_t tileX = 0 ; tileX < 16 ; ++tileX )
        {
            uint16_t offset = tileY * 256 + tileX * 16 ;

            for ( uint16_t row = 0 ; row < 8 ; ++row )
            {
                uint8_t tile_lsb0 = mapper_ppu_read(offset + row) ;
                uint8_t tile_msb0 = mapper_ppu_read(offset + row + 8);
                uint8_t tile_lsb1 = mapper_ppu_read( 0x1000 + offset + row);
                uint8_t tile_msb1 = mapper_ppu_read( 0x1000 + offset + row +8) ;

                for ( uint16_t col = 0 ; col < 8 ; ++col )
                {
                    uint8_t pixel0 = (tile_lsb0 & 0x01 ) + (tile_msb0 & 0x01) * 2 ;
                    tile_lsb0 >>= 1 ; tile_msb0 >>= 1 ;

                    uint8_t pixel1 = (tile_lsb1 & 0x01 ) + (tile_msb1 & 0x01) * 2 ;
                    tile_lsb1 >>= 1 ; tile_msb1 >>= 1 ;

                    pattern0_dp[tileX * 8 + (7 - col) + (tileY * 8 + row ) * 128 ] = color_table[palette[pixel0]] ;
                    pattern1_dp[tileX * 8 + (7 - col) + (tileY * 8 + row ) * 128 ] = color_table[palette[pixel1]] ;
                }
            }
        }
}