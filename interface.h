#ifndef NESX_INTERFACE_H
#define NESX_INTERFACE_H

#define W 128 // pattern tables pixels
#define H 128

#define NES_H 240
#define NES_W 256
#define PATTERN_UPDATE_PERIOD 15

#define SCALE_F 2

int init_interface() ;
void update_interface () ;
void copy_pattern_table() ;
void poll_inputs();

#endif //NESX_INTERFACE_H
