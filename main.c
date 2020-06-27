#include "6502.h"
#include "ppu.h"
#include "interface.h"

cpu_6502 cpu;

bool halt ;

int main(int argc, char **argv) {
    if (argc < 2) {
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

    cpu.pc = 0xC000; // start of cartridge memory space
    cpu.sr.i = 1; // for nestest rom
    cpu.sp = 0xFD;

    reset_cpu() ;
    if ( init_interface() )
        return EXIT_FAILURE ;

    while ( !halt )
    {
        update_interface();
        execute_opcode() ;
    }

    return EXIT_SUCCESS;
}
