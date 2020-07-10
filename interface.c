// this code handles the creation of the gui and the glue code to display the internals of the system

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

#include <stdbool.h>

#include "interface.h"
#include "mapper.h"
#include "controller.h"
#include "ppu.h"

extern reader mapper_cpu_read , mapper_ppu_read ;
extern writer mapper_cpu_write , mapper_ppu_write ;

extern uint32_t color_table[] ;

extern bool halt;
extern bool poll ; // for the controllers
extern uint8_t palette[0x20];

uint32_t pattern0_dp [W * H] ;
uint32_t pattern1_dp [W * H] ;
uint32_t screen [256 * 240] ; // memory for main display

SDL_Texture *texture;
SDL_Texture *texture1;
SDL_Texture *texture2;

SDL_Surface *surface;
SDL_Surface *surface1;
SDL_Surface *surface2;

SDL_Renderer *renderer ;

SDL_Rect pat0 , pat1 , screen0 ;

void display_nametable();

int init_interface()
{
    if ( SDL_Init ( SDL_INIT_VIDEO ) != 0 )
    {
        fprintf(stderr, "%s\n", SDL_GetError() );
        return 1 ;
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

void poll_inputs()
{
    halt = false ;
    static uint8_t mapped_key ;
    bool state = false ;

    SDL_Event event;

    while (SDL_PollEvent(&event))
        switch (event.type)
        {
            case SDL_QUIT: // handles x button on the upper right of the window
                halt = true;
                break;

            case SDL_KEYUP:
            case SDL_KEYDOWN:
                state = ( event.type == SDL_KEYDOWN ) ;
                switch (event.key.keysym.scancode)
                {
                    // for controller 1 for now
                    /*case SDL_SCANCODE_A : // left
                        mapped_key = (mapped_key & ~0x40) | (0x40 * state );
                        break;
                    case SDL_SCANCODE_S : // down
                        mapped_key = (mapped_key & ~0x20) | (0x20 * state );
                        break;
                    case SDL_SCANCODE_D : // right
                        mapped_key = (mapped_key & ~0x80) | (0x80 * state );
                        break;
                    case SDL_SCANCODE_W : // up
                        mapped_key = (mapped_key & ~0x10) | (0x10 * state );
                        break;
                    case SDL_SCANCODE_J : // A button
                        mapped_key = (mapped_key & ~0x01) | (0x01 * state );
                        break;
                    case SDL_SCANCODE_K : // B button
                        mapped_key = (mapped_key & ~0x02) | (0x02 * state );
                        break;
                    case SDL_SCANCODE_N : // Select
                        mapped_key = (mapped_key & ~0x04) | (0x04 * state );
                        break;
                    case SDL_SCANCODE_M : // Start
                        mapped_key = (mapped_key & ~0x08) | (0x08 * state );
                        break;*/
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

/*    if ( poll )
    {
//        printf("handed %02x as input\n" , mapped_key);
        update_cntrl1( mapped_key ) ;
    }*/
}
void update_interface ()
{
    if ( halt )
    {
        SDL_Quit();
    }
    else
    {
        copy_pattern_table() ;
        //display_nametable();

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

void display_nametable() // debug
{
    // this is where we render the display for now
    for ( int y = 0 ; y < 30 ; ++y )
    {
        for (int x = 0; x < 32 ; ++x)
        {
            uint8_t offset = mapper_ppu_read( 0x2000 + y * 32 + x );
            printf("%02x ",offset);
        }
        printf("\n");
    }
    printf("\n");
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