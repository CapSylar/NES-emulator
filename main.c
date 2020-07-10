#include "6502.h"
#include "ppu.h"
#include "interface.h"

cpu_6502 cpu;
ppu_state nes_ppu ;

extern uint8_t internal_ram[0x800];

bool halt ;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Please enter binary file name\n");
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(*++argv, "rb");

    if ( !fp )
    {
        perror("could not open file") ;
        return EXIT_FAILURE ;
    }

    load_cartridge(fp);
    fclose(fp);

    reset_cpu() ;
    reset_ppu() ;

    if ( init_interface() )
        return EXIT_FAILURE ;

    unsigned wait_inter = 0; // just for now
    unsigned wait_poll = 0 ;

    while ( !halt )
    {
        if (wait_inter++ == 89343 ) // 89343
        {
            update_interface();
            wait_inter = 0 ;
        }

        if ( wait_poll++ == 200 )
        {
            poll_inputs();
            wait_poll = 0 ;
        }

        clock_system() ;
    }
    return EXIT_SUCCESS;
}
