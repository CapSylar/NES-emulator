#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool poll ;
static uint8_t w_4016 ;
static uint8_t cntrl1_latch ;
static uint8_t cntrl2_latch ;

uint8_t read_controller1 ()
{
    if (poll)
        return ( cntrl1_latch & 0x01 ) ;
    else
    {
        uint8_t temp = ( cntrl1_latch & 0x01 );
        cntrl1_latch >>= 1;
        return temp ;
    }
}

uint8_t read_controller2()
{
    return 0; // for now
}

void write_cntrl ( uint8_t data )
{
    w_4016 = data ;
    poll = data & 0x01 ;
}

void update_cntrl1 ( uint8_t data )
{
    cntrl1_latch = data ;
}