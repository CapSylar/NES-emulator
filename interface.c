// this code handles the creation of the gui and the glue code to display the internals of the system
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <stdlib.h>

#include <stdbool.h>

#include "interface.h"
#include "mapper.h"
#include "controller.h"

extern reader mapper_cpu_read , mapper_ppu_read ;
extern writer mapper_cpu_write , mapper_ppu_write ;

extern uint32_t color_table[] ;

extern bool halt;
extern bool poll ; // for the controllers
extern uint8_t palette[0x20];

uint32_t pattern0_dp [W * H] ;
uint32_t pattern1_dp [W * H] ;
uint32_t screen [NES_W * NES_H] ; // memory for main display

SDL_Texture *texture;
SDL_Texture *texture1;
SDL_Texture *texture2;

SDL_Renderer *renderer ;

SDL_Rect pat0 , pat1 , screen0 ;

void display_nametable();

void poll_inputs()
{
    halt = false ;
    static uint8_t mapped_key ;
    SDL_Event event;

    while (SDL_PollEvent(&event))
        switch (event.type)
        {
            case SDL_QUIT: // handles x button on the upper right of the window
                halt = true;
                break;

            case SDL_KEYUP:
            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                    case SDL_SCANCODE_ESCAPE:
                        halt = true ;
                        break ;
                    default :
                        break;
                }
                break ;
            default:
                break ;
        }
}

int init_interface()
{
    if ( SDL_Init ( SDL_INIT_VIDEO ) != 0 )
    {
        fprintf(stderr, "%s\n", SDL_GetError() );
        return EXIT_FAILURE ;
    }

    pat0 = ( SDL_Rect ) { 0 , 0 , W * SCALE_F, H * SCALE_F } ;
    pat1 = ( SDL_Rect ) { W * SCALE_F , 0 , W * SCALE_F , H * SCALE_F } ;
    screen0 = ( SDL_Rect ) { 0 , H * SCALE_F , NES_W * SCALE_F , NES_H * SCALE_F } ;


    SDL_Window *window = SDL_CreateWindow( "NESX", SDL_WINDOWPOS_UNDEFINED,
                                           SDL_WINDOWPOS_UNDEFINED, 2 * W * SCALE_F  , (H + NES_H) * SCALE_F ,
                                           0);

    renderer = SDL_CreateRenderer(window, -1, 0);

    texture = SDL_CreateTexture( renderer , SDL_PIXELFORMAT_ARGB8888 , SDL_TEXTUREACCESS_STREAMING , W , H ) ;
    texture1 = SDL_CreateTexture( renderer , SDL_PIXELFORMAT_ARGB8888 , SDL_TEXTUREACCESS_STREAMING , W , H ) ;
    texture2 = SDL_CreateTexture( renderer , SDL_PIXELFORMAT_ARGB8888 , SDL_TEXTUREACCESS_STREAMING , NES_W , NES_H ) ;


    return EXIT_SUCCESS ;
}

void update_interface ()
{
    static int update_period ;

    if ( halt )
    {
        SDL_Quit();
    }
    else
    {
        if ( update_period++ == PATTERN_UPDATE_PERIOD )
        {
            copy_pattern_table() ;
            update_period = 0 ;
        }


        SDL_UpdateTexture( texture , 0 , pattern0_dp , W * 4 ) ;
        SDL_UpdateTexture( texture1 , 0 , pattern1_dp , W * 4 ) ;
        SDL_UpdateTexture( texture2 , 0 , screen , NES_W * 4 ) ;

        SDL_RenderCopy( renderer , texture , NULL , &pat0 ) ;
        SDL_RenderCopy( renderer , texture1 , NULL , &pat1 ) ;
        SDL_RenderCopy( renderer , texture2 , NULL , &screen0 ) ;
        SDL_RenderPresent( renderer ) ;
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