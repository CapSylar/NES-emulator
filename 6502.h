#ifndef NES_H_
#define NES_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ROM1 0x8000
#define ROM2 0xC000

typedef struct {

    uint8_t a_reg , x_reg , y_reg , sp  ;
    uint16_t pc ;

    struct sr
    {
        uint8_t n : 1 ;
        uint8_t v : 1 ;
        uint8_t b : 1 ;
        uint8_t d : 1 ; // not used here
        uint8_t i : 1 ;
        uint8_t z : 1 ;
        uint8_t c : 1 ;

    } sr ; // status register

} cpu_6502 ;

static const uint8_t inst_mode[256] = {
        6, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 1, 1, 1, 1,
        10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
        1, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 1, 1, 1, 1,
        10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
        6, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 1, 1, 1, 1,
        10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
        6, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 8, 1, 1, 1,
        10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
        5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
        10, 9, 6, 9, 12, 12, 13, 13, 6, 3, 6, 3, 2, 2, 3, 3,
        5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
        10, 9, 6, 9, 12, 12, 13, 13, 6, 3, 6, 3, 2, 2, 3, 3,
        5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
        10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
        5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
        10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2
};


static const char inst_name[256][4] = { // for debugging purposes
        //FUT represents unimplemented op codes
        "BRK", "ORA", "FUT", "FUT", "FUT", "ORA", "ASL", "FUT",
        "PHP", "ORA", "ASL", "FUT", "FUT", "ORA", "ASL", "FUT",
        "BPL", "ORA", "FUT", "FUT", "FUT", "ORA", "ASL", "FUT",
        "CLC", "ORA", "FUT", "FUT", "FUT", "ORA", "ASL", "FUT",
        "JSR", "AND", "FUT", "FUT", "BIT", "AND", "ROL", "FUT",
        "PLP", "AND", "ROL", "FUT", "BIT", "AND", "ROL", "FUT",
        "BMI", "AND", "FUT", "FUT", "FUT", "AND", "ROL", "FUT",
        "SEC", "AND", "FUT", "FUT", "FUT", "AND", "ROL", "FUT",
        "RTI", "EOR", "FUT", "FUT", "FUT", "EOR", "LSR", "FUT",
        "PHA", "EOR", "LSR", "FUT", "JMP", "EOR", "LSR", "FUT",
        "BVC", "EOR", "FUT", "FUT", "FUT", "EOR", "LSR", "FUT",
        "CLI", "EOR", "FUT", "FUT", "FUT", "EOR", "LSR", "FUT",
        "RTS", "ADC", "FUT", "FUT", "FUT", "ADC", "ROR", "FUT",
        "PLA", "ADC", "ROR", "FUT", "JMP", "ADC", "ROR", "FUT",
        "BVS", "ADC", "FUT", "FUT", "FUT", "ADC", "ROR", "FUT",
        "SEI", "ADC", "FUT", "FUT", "FUT", "ADC", "ROR", "FUT",
        "FUT", "STA", "FUT", "FUT", "STY", "STA", "STX", "FUT",
        "DEY", "FUT", "TXA", "FUT", "STY", "STA", "STX", "FUT",
        "BCC", "STA", "FUT", "FUT", "STY", "STA", "STX", "FUT",
        "TYA", "STA", "TXS", "FUT", "FUT", "STA", "FUT", "FUT",
        "LDY", "LDA", "LDX", "FUT", "LDY", "LDA", "LDX", "FUT",
        "TAY", "LDA", "TAX", "FUT", "LDY", "LDA", "LDX", "FUT",
        "BCS", "LDA", "FUT", "FUT", "LDY", "LDA", "LDX", "FUT",
        "CLV", "LDA", "TSX", "FUT", "LDY", "LDA", "LDX", "FUT",
        "CPY", "CMP", "FUT", "FUT", "CPY", "CMP", "DEC", "FUT",
        "INY", "CMP", "DEX", "FUT", "CPY", "CMP", "DEC", "FUT",
        "BNE", "CMP", "FUT", "FUT", "FUT", "CMP", "DEC", "FUT",
        "CLD", "CMP", "FUT", "FUT", "FUT", "CMP", "DEC", "FUT",
        "CPX", "SBC", "FUT", "FUT", "CPX", "SBC", "INC", "FUT",
        "INX", "SBC", "NOP", "FUT", "CPX", "SBC", "INC", "FUT",
        "BEQ", "SBC", "FUT", "FUT", "FUT", "SBC", "INC", "FUT",
        "SED", "SBC", "FUT", "FUT", "FUT", "SBC", "INC", "FUT"
};

static const int inst_cycle[256] =
        {
                7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, //00 -> 0F
                2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //10 -> 1F
                6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6, //20 -> 2F
                2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //30 -> 3F
                6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, //40 -> 4F
                2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //50 -> 5F
                6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, //60 -> 6F
                2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //70 -> 7F
                2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 0, 4, 4, 4, 4, //80 -> 8F
                2, 6, 0, 0, 4, 4, 4, 4, 2, 5, 2, 0, 0, 5, 0, 0, //90 -> 9F
                2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 0, 4, 4, 4, 4, //A0 -> AF
                2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 0, 4, 4, 4, 4, //B0 -> BF
                2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, //C0 -> CF
                2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //D0 -> DF
                2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, //E0 -> EF
                2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7 //F0 -> FF
        };

static const bool inst_page_cross[256] =
        {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //00 -> 0F
                0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, //10 -> 1F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20 -> 2F
                0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, //30 -> 3F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, //40 -> 4F
                0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, //50 -> 5F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //60 -> 6F
                0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, //70 -> 7F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //80 -> 8F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //90 -> 9F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //A0 -> AF
                0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, //B0 -> BF
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //C0 -> CF
                0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, //D0 -> DF
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //E0 -> EF
                0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0 //F0 -> FF
        };


enum modes { illegal = 0 , absolute = 1 , absolute_x , absolute_y , a_only , immediate ,
    implied , indexed_indirect , indirect , indirect_indexed ,
    relative , zero_p , zero_p_x , zero_p_y } ;

void set_flags ( uint16_t operand , uint8_t code ) ;
void load_program ( FILE *fp , int prog_start ) ;
//uint8_t *cpu_bus_rw ( uint16_t address ) ;
void print_status ( int pc , uint8_t opcode ) ;
void load_cartridge ( FILE *fp ) ;
void execute_opcode () ;
uint8_t cpu_read ( uint16_t address ) ;
void cpu_write ( uint16_t address , uint8_t data ) ;
void reset_cpu() ;

#endif
