// this code handles the creation of the gui and the glue code to display the internals of the system

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

#include "interface.h"

extern bool halt;
extern uint8_t pattern_tables [2][0x1000] ;

uint32_t pixels [W*H] ;
SDL_Texture *texture;
SDL_Renderer *renderer ;

int init_interface()
{
    if ( SDL_Init ( SDL_INIT_AUDIO ) != 0 )
    {
        fprintf(stderr, "%s\n", SDL_GetError() );
        return 1 ;
    }

    SDL_Window *window = SDL_CreateWindow( "testing", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W * 4, H * 4,
                                          SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, W, H);
    SDL_UpdateTexture( texture , NULL , pixels , W*4 ) ;
    SDL_RenderCopy( renderer , texture , NULL , NULL ) ;
    SDL_RenderPresent( renderer ) ;
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
        SDL_UpdateTexture( texture , NULL , pixels , W*4 ) ;
        SDL_RenderCopy( renderer , texture , NULL , NULL ) ;
        SDL_RenderPresent( renderer ) ;
    }
}

void copy_pattern_table()
{
    uint32_t palette[] = { 0x00000000 , 0x00FF0000 , 0x0000FF00 , 0x000000FF };
//    uint32_t bit_0 = 0x00000000 ; // black
//    uint32_t bit_1 = 0x00FF0000 ; //red
//    uint32_t bit_2 = 0x0000FF00 ; //green
//    uint32_t bit_3 = 0x000000FF ; // blue

    // for now we will copy only the first pattern table
    int offset = 0 ;

    for ( uint16_t current = 0 ; current < 0x1000 ; ++current )
    {
        for ( int i = 0 ; i < 8 ; ++i )
        {
            int bit =
                    ((pattern_tables[0][current] >> (7 - i % 8)) & 0x1) | (((pattern_tables[0][current + 8] >> (7 - i % 8)) & 0x1) << 1) ;
            pixels[offset++] = palette[bit];
        }

        if ( (current & 0x000F )== 7 ) // jump over second color pane
        {
            current += 8 ;
        }
    }
}