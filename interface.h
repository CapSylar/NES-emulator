#ifndef NESX_INTERFACE_H
#define NESX_INTERFACE_H

#define W 128 // pattern tables pixels
#define H 128

#define SCALE_F 2

int init_interface() ;
void update_interface () ;
void copy_pattern_table() ;
void copy_nametables() ;

#endif //NESX_INTERFACE_H
