#ifndef NESX_CONTROLLER_H
#define NESX_CONTROLLER_H

#include <stdint.h>

uint8_t read_controller1 ();
uint8_t read_controller2() ;
void write_cntrl ( uint8_t data ) ;
void update_cntrl1 ( uint8_t data ) ;


#endif //NESX_CONTROLLER_H
