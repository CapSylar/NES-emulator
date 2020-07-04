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

    int wait = 0; // just for now

    while ( !halt )
    {
        if ( wait++ == 89000 )
        {
            update_interface();
            wait = 0 ;
        }
        clock_system() ;
    }

    return EXIT_SUCCESS;
}
