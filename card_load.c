#include <stdio.h>
#include <stdint.h>
#include "mapper.h"
#include <stdlib.h>
#include "mapper0.h"

void load_cartridge ( FILE *fp ) // creates the nes header struct for the mapper and reads in the entire file
{
    // first we have to read INES header which is 16 bytes
    nes_header header;
    uint8_t mapper ;
    fseek ( fp , 4 , SEEK_SET ) ; // skip first 4 bytes "NES" + EOF
    header.prg_rom_count = getc(fp) ; // size of PRG ROM in 16KB units (byte 4)
    header.chr_rom_count = getc(fp) ; // size of CHR ROM in 8KB units (byte 5)

    uint8_t byte6 = getc(fp) ; // (byte 6)
    mapper = byte6 & 0xF0 ; // (byte 6) we get the lower nibble of the mapper number
    header.mirroring = byte6 & 0x01 ; // 0 horizontal || 1 vertical
    header.has_pers_mem = byte6 & 0x02 ;
    header.four_nt = byte6 & 0x08 ;

    mapper = (mapper >> 4) | (getc(fp) & 0xF0) ;  // (byte 7) we get the higher nibble of the mapper number

    header.prg_ram_size = getc(fp);
    getc(fp);
    header.has_prg_ram = getc(fp) & 0x10 ;

    fseek ( fp , 16 , SEEK_SET ) ; // skip the rest of the header

    // allocate memory for the prg and chr roms

    uint8_t* prg_mem = malloc ( 0x4000 * header.prg_rom_count ) ; // 16KiB units
    uint8_t* chr_mem = malloc( 0x2000 * header.chr_rom_count ); // 8 KiB units

    header.prg_mem = prg_mem ;
    header.chr_mem = chr_mem ;

    for ( int i = 0 ; i < header.prg_rom_count * 0x4000 ; ++i ) // copy the prg memory
        prg_mem[i] = getc(fp) ;
    for ( int i = 0 ; i < header.chr_rom_count * 0x2000 ; ++i ) // copy the chr memory
        chr_mem[i] = getc(fp) ;

    switch ( mapper ) // call the appropriate init function for the mapper that will be used
    {
        case 0: // NROM
            init_mapper0( header ) ;
            break;
        default:
            fprintf( stderr , "unimplemented mapper %d" , mapper ) ;
            exit(EXIT_FAILURE) ;
            break ;
    }
}