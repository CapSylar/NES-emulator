// this cpu is developed for an NES emulator and thus the actual cpu name is 2A03
// which is a 6502 cpu lacking a decimal mode

#include "6502.h"

#define ADDRESS ( (uint16_t) cpu_read(cpu.pc+1) << 8 | cpu_read(cpu.pc) )

#define PUSH_S(qt) ( cpu_write( cpu.sp-- + 0x100 , (qt) ))

#define PUSH_D(qt) { cpu_write ( cpu.sp-- + 0x100 , (qt >> 8) );\
                     cpu_write( cpu.sp-- + 0x100 , (qt) ) ; }

#define POP_S ( cpu_read( ++cpu.sp + 0x100 ) )
#define POP_D  ( (uint16_t) cpu_read( cpu.sp + 0x101 ) | cpu_read( cpu.sp + 0x102 ) << 8 )
#define SIGN_EXT(qt) ( (uint16_t)(qt) | 0xFF00 * ( ((qt) & 0x80) != 0 ) )
#define CHECK_PAGE_CROSS(x,y) (((x) & 0xFF00) != ( ((x) + (y)) & 0xFF00))

#define BRANCH(cond)  { if ( cond )\
                        {\
                            operand = SIGN_EXT(b_offset) ;\
                            if ( CHECK_PAGE_CROSS(cpu.pc,operand))\
                            ++cycles ;\
                                ++cycles ;\
                            cpu.pc += operand ;\
                        } } ;

extern cpu_6502 cpu ;

int cycles ;

static uint8_t pack ( struct sr *current ) ;
static void unpack ( struct sr *current , uint8_t packed ) ;

void execute_opcode ()
{
    int pc = cpu.pc ;
    uint8_t opcode = cpu_read(cpu.pc++) ; // read an instruction
    uint16_t operand = 0 , temp = 0 ;
    uint8_t b_offset = 0 ; // has to be signed since branching can be done back and forth
    // first we have to determine the addressing mode

    if ( opcode == 0 ) // for debugging only
        exit(0) ;

    print_status ( pc , opcode) ;

    cycles += inst_cycle[opcode] ;
    switch ( inst_mode[opcode] )
    {
        case absolute :
            operand = ADDRESS ;
            cpu.pc += 2 ;
            break ;
        case absolute_x :
            operand = ADDRESS ;
            if ( inst_page_cross[opcode] && CHECK_PAGE_CROSS(operand , cpu.x_reg ) )
                ++cycles ; // a page cross occured
            operand += cpu.x_reg ;
            cpu.pc += 2 ;
            break ;
        case absolute_y :
            operand = ADDRESS ;
            if ( inst_page_cross[opcode] && CHECK_PAGE_CROSS(operand , cpu.y_reg ) )
                ++cycles ; // a page cross occured
            operand += cpu.y_reg ;
            cpu.pc += 2 ;
            break ;
        case a_only :
            operand = cpu.a_reg ;
            break ;
        case immediate :
            operand = cpu_read(cpu.pc++) ;
            break ;
        case indexed_indirect :
            operand = cpu_read(cpu.pc++) ;
            operand = cpu_read(operand+cpu.x_reg + 1 & 0xFF ) << 8 | cpu_read(operand + cpu.x_reg & 0xFF) ;
            break ;
        case indirect : // JMP (xxFF) has a bug where LSB is fetched from xxFF and MSB from xx00 not xx+1 , we have to emulate it
            operand = ADDRESS ;
            operand = cpu_read(operand) | cpu_read( operand & 0xFF == 0xFF ? operand & 0xFF00 : operand + 1  )  << 8  ;
            cpu.pc += 2 ;
            break ;
        case indirect_indexed :
            operand = cpu_read(cpu.pc++) ;
            operand = (cpu_read(operand+1 & 0xFF ) << 8 | cpu_read(operand & 0xFF)) ;
            if ( inst_page_cross[opcode] && CHECK_PAGE_CROSS(operand , cpu.y_reg ) )
                ++cycles ; // a page cross occured
            operand += cpu.y_reg ;
            break ;
        case relative :
            b_offset = cpu_read(cpu.pc++);
            break ;
        case zero_p :
            operand = cpu_read(cpu.pc++);
            break ;
        case zero_p_x :
            operand = cpu_read(cpu.pc++ ) + cpu.x_reg & 0xFF ; // it has to wrap around zero page
            break ;
        case zero_p_y :
            operand = cpu_read(cpu.pc++ ) + cpu.y_reg & 0xFF  ;
            break ;
    }
    switch ( opcode )
    {
        case 0x65: case 0x75: case 0x6D: case 0x7D: // ADC add memory to accumulator with carry
        case 0x79: case 0x61: case 0x71: case 0x69:
            temp = cpu.a_reg ;
            if ( opcode != 0x69 ) // if immediate case operand is already the value
                operand = cpu_read(operand) ;
            temp += operand + cpu.sr.c ;
            cpu.sr.v = (0x80 & ( temp ^ cpu.a_reg ) & ( operand ^ temp )) >> 7 ;
            set_flags ( temp , 7 ) ;
            cpu.a_reg = temp ;
            break ;
        case 0x29: case 0x25: case 0x35: case 0x2D: // AND and memory with accumulator
        case 0x3D: case 0x39: case 0x21: case 0x31:
            if ( opcode != 0x29 ) // immediate case
                operand = cpu_read(operand) ;
            cpu.a_reg &= operand ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0x0A: // ASL shift accumulator left one bit
            cpu.sr.c = (cpu.a_reg & 0x80) >> 7 ;
            cpu.a_reg <<= 1 ;
            set_flags ( cpu.a_reg , 6 );
            break ;
        case 0x06: case 0x16: case 0x0E: case 0x1E: // ASL shift memory left one bit
            b_offset = cpu_read(operand) ;
            cpu.sr.c = ( b_offset & 0x80) >> 7 ;
            //*cpu_bus_rw(operand) <<= 1 ;
            cpu_write( operand , b_offset <<= 1 ) ;
            set_flags ( b_offset , 6 ) ;
            break ;
        case 0x90 : // BCC branch on carry clear
        BRANCH(!cpu.sr.c) ;
            break ;
        case 0xB0 : // BCS branch on carry set
        BRANCH(cpu.sr.c) ;
            break ;
        case 0xF0 : // BEQ branch on result zero
        BRANCH(cpu.sr.z) ;
            break ;
        case 0x24: case 0x2C: // BIT test bits in memory with accumulator
            operand = cpu_read(operand) ;
            cpu.sr.n = (operand & 0x80) >> 7 ;
            cpu.sr.v = (operand & 0x40) >> 6 ;
            cpu.sr.z = ( (operand & cpu.a_reg) == 0 ) ;
            break ;
        case 0x30: // BMI branch on minus
        BRANCH(cpu.sr.n) ;
            break ;
        case 0xD0: // BNE branch on result not zero
        BRANCH(!cpu.sr.z) ;
            break ;
        case 0x10: // BPL branch on result positive
        BRANCH(!cpu.sr.n) ;
            break ;
        case 0x00: // BRK force break
            // write pc then sr to stack
            PUSH_D(cpu.pc);
            cpu.sr.b = 1; // for BRK and PHP
            uint8_t brk_temp = pack(&cpu.sr);
            PUSH_S(brk_temp);
            // interrupt request vector is at 0xFFFE and 0xFFFF
            cpu.pc = cpu_read(0xFFFE) | (cpu_read(0xFFFF) << 8);
            break ;
        case 0x50: // BVC branch on overflow clear
        BRANCH(!cpu.sr.v) ;
            break ;
        case 0x70: // BVS branch on overflow set
        BRANCH(cpu.sr.v) ;
            break ;
        case 0x18: // CLC clear carry flag
            cpu.sr.c = 0 ;
            break ;
        case 0xD8: // CLD clear decimal mode
            cpu.sr.d = 0 ;
            break ;
        case 0x58: // CLI clear interrupt disable bit
            cpu.sr.i = 0 ;
            break ;
        case 0xB8: // CLV clear overflow flag
            cpu.sr.v = 0 ;
            break ;
        case 0xC9: case 0xC5: case 0xD5: case 0xCD: // CMP compare memory with accumulator
        case 0xDD: case 0xD9: case 0xC1: case 0xD1:
            if ( opcode != 0xC9 )
                operand = cpu_read(operand) ;

            temp = cpu.a_reg ;
            temp -= operand ;
            cpu.sr.c = !( temp >> 8 ) ;
            set_flags ( temp , 6 ) ;
            break ;
        case 0xE0: case 0xE4: case 0xEC: // CPX compare memory with index x
            if ( opcode != 0xE0 )
                operand = cpu_read(operand) ;
            temp = cpu.x_reg ;
            temp -= operand ;
            cpu.sr.c = !( temp >> 8 ) ;
            set_flags ( temp , 6 ) ;
            break ;
        case 0xC0: case 0xC4: case 0xCC: // CPY compare memory with index y
            if ( opcode != 0xC0 )
                operand = cpu_read(operand) ;
            temp = cpu.y_reg ;
            temp -= operand ;
            cpu.sr.c = !( temp >> 8 ) ;
            set_flags ( temp , 6  ) ;
            break ;
        case 0xC6: case 0xD6: case 0xCE: case 0xDE: // DEC decrement memory by one
            b_offset = cpu_read(operand) ;
            cpu_write( operand , --b_offset) ;
            set_flags ( b_offset , 6  ) ;
            break ;
        case 0xCA: // DEX decrement x by one
            --cpu.x_reg ;
            set_flags ( cpu.x_reg , 6  ) ;
            break ;
        case 0x88: // DEY decrement y by one
            --cpu.y_reg ;
            set_flags ( cpu.y_reg , 6  ) ;
            break ;
        case 0x49: case 0x45: case 0x55: case 0x4D: // EOR XOR memory with accumulator
        case 0x5D: case 0x59: case 0x41: case 0x51:

            if ( opcode != 0x49 )
                operand = cpu_read(operand) ;
            cpu.a_reg ^= operand ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0xE6: case 0xF6: case 0xEE: case 0xFE: // INC increment memory by one
            b_offset = cpu_read(operand) ;
            cpu_write( operand , ++b_offset ) ;
            set_flags ( b_offset , 6  ) ;
            break ;
        case 0xE8: // INX increment index x by one
            ++cpu.x_reg ;
            set_flags ( cpu.x_reg , 6 ) ;
            break ;
        case 0xC8: // INY increment index y by one
            ++cpu.y_reg ;
            set_flags ( cpu.y_reg , 6 ) ;
            break ;
        case 0x4C: case 0x6C: // JMP jump to new location
            cpu.pc = operand ;
            break ;
        case 0x20: // JSR jump to new location saving return address
        PUSH_D(cpu.pc-1) ; // pc is already incremented by two due to the way the code way written
            cpu.pc = operand ; // the 6502 has a bug in this location where it pushes return address - 1
            break ;
        case 0xA9: case 0xA5: case 0xB5: case 0xAD: // LDA load accumulator with memory
        case 0xBD: case 0xB9: case 0xA1: case 0xB1:
            if ( opcode != 0xA9 )
                operand = cpu_read(operand) ;
            cpu.a_reg = operand ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0xA2: case 0xA6: case 0xB6: case 0xAE: // LDX load index x with memory
        case 0xBE:
            if ( opcode != 0xA2 )
                operand = cpu_read(operand) ;
            cpu.x_reg = operand ;
            set_flags ( cpu.x_reg , 6 ) ;
            break ;
        case 0xA0: case 0xA4: case 0xB4: case 0xAC: // LDY load index y with memory
        case 0xBC:
            if ( opcode != 0xA0 )
                operand = cpu_read(operand) ;
            cpu.y_reg = operand ;
            set_flags ( cpu.y_reg , 6 );
            break ;
        case 0x4A: // LSR A shift right one bit
            cpu.sr.c = cpu.a_reg ;
            cpu.a_reg >>= 1 ;
            set_flags ( cpu.a_reg , 2 );
            cpu.sr.n = 0 ; // reset the negative flag
            break ;
        case 0x46: case 0x56: case 0x4E: case 0x5E: // LSR shift right memory
            b_offset = cpu_read(operand) ;
            cpu.sr.c = b_offset ;
            //*cpu_bus_rw(operand) >>= 1 ;
            cpu_write( operand , b_offset >>= 1 ) ;
            set_flags ( b_offset , 2 ) ;
            cpu.sr.n = 0 ; // reset the negative flag
            break ;
        case 0xEA: // NOP no operation
            break ;
        case 0x09: case 0x05: case 0x15: case 0x0D: // OR memory with accumulator
        case 0x1D: case 0x19: case 0x01: case 0x11:
            if ( opcode != 0x09 )
                operand = cpu_read(operand) ;
            cpu.a_reg |= operand ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0x48: // PHA push accumulator on stack
            PUSH_S ( cpu.a_reg ) ;
            break ;
        case 0x08: // PHP push processor status on stack
            b_offset = pack( &cpu.sr ) ;
            PUSH_S ( b_offset ) ;
            break ;
        case 0x68: // PLA pull accumulator from stack
            cpu.a_reg = POP_S ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0x28: // PLP pull process status from stack
            b_offset = POP_S ;
            // deconstruct byte into flags
            unpack( &cpu.sr , b_offset ) ;
            break ;
        case 0x2A: // ROL rotate left accumulator one bit
            temp = (cpu.a_reg & 0x80) >> 7 ;
            cpu.a_reg = cpu.a_reg << 1 | cpu.sr.c ;
            cpu.sr.c = temp ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0x26: case 0x36: case 0x2E: case 0x3E: // ROL rotate left memory
            b_offset = cpu_read(operand) ;
            temp = ( b_offset & 0x80) >> 7 ;
            //*cpu_bus_rw(operand) = *cpu_bus_rw(operand) << 1 | cpu.sr.c ;
            b_offset = b_offset << 1 | cpu.sr.c ;
            cpu_write( operand , b_offset ) ; // update the mem
            cpu.sr.c = temp ;
            set_flags ( b_offset , 6 ) ;
            break ;
        case 0x6A: // ROR rotate right accumulator
            temp = cpu.a_reg & 1 ;
            cpu.a_reg = cpu.a_reg >> 1 | 0x80 * cpu.sr.c ;
            cpu.sr.c = temp ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0x66: case 0x76: case 0x6E: case 0x7E: // ROR rotate right memory
            b_offset = cpu_read(operand) ;
            temp = b_offset & 1 ;
            b_offset = b_offset >> 1 | 0x80 * cpu.sr.c ;
            cpu.sr.c = temp ;
            cpu_write( operand , b_offset ) ; // update the mem
            set_flags ( b_offset , 6 ) ;
            break ;
        case 0x40: // RTI return from interrupt
            b_offset = POP_S ;
            // deconstruct byte into flags
            unpack( &cpu.sr , b_offset ) ;
            cpu.pc = POP_D ;
            cpu.sp += 2 ;
            break ;
        case 0x60: // RTS return from subroutine
            cpu.pc = POP_D + 1 ; // bug where the 6502 adds + 1 to the address popped
            cpu.sp += 2 ;
            break ;
        case 0xE9: case 0xE5: case 0xF5: case 0xED: // SBC internal_ram from accumulator with borrow
        case 0xFD: case 0xF9: case 0xE1: case 0xF1:
        case 0xEB: // $EB is an illegal instruction
            temp = cpu.a_reg + cpu.sr.c ;
            b_offset = ( opcode == 0xE9 || opcode == 0xEB ) ? ~operand : ~cpu_read(operand) ;
            temp += b_offset ;
            cpu.a_reg += cpu.sr.c ;
            cpu.sr.v = (0x80 & ( temp ^ cpu.a_reg ) & ( b_offset ^ temp )) >> 7;
            set_flags ( temp , 7 ) ;
            cpu.a_reg = temp ;
            break ;
        case 0x38: // SEC set carry flags
            cpu.sr.c = 1 ;
            break ;
        case 0xF8: // SED set decimal flags
            cpu.sr.d = 1 ;
            break ;
        case 0x78: // SEI set interrupt disable status
            cpu.sr.i = 1 ;
            break ;
        case 0x85: case 0x95: case 0x8D: case 0x9D: // STA store accumulator in memory
        case 0x99: case 0x81: case 0x91:
            cpu_write( operand , cpu.a_reg ) ;
            break ;
        case 0x86: case 0x96: case 0x8E: // STX store x index in memory
            cpu_write( operand , cpu.x_reg ) ;
            break ;
        case 0x84: case 0x94: case 0x8C: // STY store y register in memory
            cpu_write( operand , cpu.y_reg ) ;
            break ;
        case 0xAA: // TAX transfer accumulator to index x
            cpu.x_reg = cpu.a_reg ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0xA8: // TAY tranfer accumulator to index y
            cpu.y_reg = cpu.a_reg ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0xBA: // TSX tranfer stack pointer to index  x
            cpu.x_reg = cpu.sp ;
            set_flags ( cpu.sp , 6 ) ;
            break ;
        case 0x8A: // TXA tranfer index x to accumulator
            cpu.a_reg = cpu.x_reg ;
            set_flags ( cpu.x_reg , 6 ) ;
            break ;
        case 0x9A: // TXS transfer index x to stack register
            cpu.sp = cpu.x_reg ;
            break ;
        case 0x98: // TYA tranfer index y to accumulator
            cpu.a_reg = cpu.y_reg ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;

            // ILLEGAL OPCODES SECTION

        case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: // NOP
            break ;
        case 0x80: case 0x82: case 0xC2: case 0xE2: case 0x04: case 0x14: // SKB skip next byte
        case 0x34: case 0x44: case 0x54: case 0x64: case 0x74: case 0xD4:
        case 0xF4:
            break ;
        case 0xAF: case 0xBF: case 0xA7: case 0xB7: case 0xA3: case 0xB3: // LAX load x and a with value from address
            cpu.x_reg = cpu_read(operand) ;
            cpu.a_reg = cpu.x_reg ;
            set_flags ( cpu.x_reg , 6 ) ;
            break ;
        case 0x0F: case 0x1F: case 0x1B: case 0x07: case 0x17: case 0x03: case 0x13: // ASO or SLO shift loc then or with acc
            b_offset = cpu_read( operand ) ;
            cpu.sr.c = ( b_offset & 0x80) >> 7 ;
            b_offset <<= 1 ; // shift left
            cpu_write( operand , b_offset ); // update the mem
            cpu.a_reg |= b_offset ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0x2F: case 0x3F: case 0x3B: case 0x27: case 0x37: case 0x23: case 0x33: // RLA ROL the and location with acc
            b_offset = cpu_read(operand) ;
            temp = ( b_offset & 0x80) >> 7 ;
            b_offset = b_offset << 1 | cpu.sr.c ;
            cpu.sr.c = temp ;
            cpu_write( operand , b_offset ) ; // update the mem
            cpu.a_reg &= b_offset ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0x4F: case 0x5F: case 0x5B: case 0x47: case 0x57: case 0x43: case 0x53: // LSE or SRE logical shift right then xor
            b_offset = cpu_read(operand) ;
            cpu.sr.c = b_offset ;
            b_offset >>= 1 ;
            cpu_write( operand , b_offset ) ; // update the mem
            cpu.a_reg ^= b_offset ;
            set_flags ( cpu.a_reg , 6 ) ;
            break ;
        case 0x6F: case 0x7F: case 0x7B: case 0x67: case 0x77: case 0x63: case 0x73: // RRA rotate right then ADC
            b_offset = cpu_read(operand) ;
            temp = b_offset & 1 ;
            b_offset = b_offset >> 1 | 0x80 * cpu.sr.c ;
            cpu_write( operand , b_offset ) ; // update the mem
            cpu.sr.c = temp ;
            temp = cpu.a_reg ;
            operand = b_offset ;
            temp += operand + cpu.sr.c ;
            cpu.sr.v = (0x80 & ( temp ^ cpu.a_reg ) & ( operand ^ temp )) >> 7 ;
            set_flags ( temp , 7 ) ;
            cpu.a_reg = temp ;
            break ;
        case 0xCF: case 0xDF: case 0xDB: case 0xC7: case 0xD7: case 0xC3: case 0xD3: // DCP decrement memory then compare
            b_offset = cpu_read( operand ) ;
            temp = --b_offset;
            cpu_write( operand , b_offset );
            operand = temp ;
            temp = cpu.a_reg ;
            temp -= operand ;
            cpu.sr.c = !( temp >> 8 ) ;
            set_flags ( temp , 6 ) ;
            break;
        case 0x83: case 0x87: case 0x8F: case 0x97: // SAX store bitwise AND of acc and x
            cpu_write( operand , cpu.a_reg & cpu.x_reg ) ;
            break ;
        case 0xE3: case 0xE7: case 0xEF: case 0xF3: case 0xF7: case 0xFB: case 0xFF: // ISC or ISB = INC then SBC
            b_offset = cpu_read(operand);
            cpu_write( operand , ++b_offset );
            temp = cpu.a_reg + cpu.sr.c ;
            b_offset = ( opcode == 0xE9 || opcode == 0xEB ) ? ~operand : ~cpu_read(operand) ;
            temp += b_offset ;
            cpu.a_reg += cpu.sr.c ;
            cpu.sr.v = (0x80 & ( temp ^ cpu.a_reg ) & ( b_offset ^ temp )) >> 7;
            set_flags ( temp , 7 ) ;
            cpu.a_reg = temp ;
            break ;
    }
}

void reset_cpu()
{
    cpu.x_reg = cpu.y_reg = cpu.a_reg = 0;
    cpu.sr.i = 1; // disable interrupts
    cpu.sp = 0xFD ;
    // reset vector is located at 0xFFFC and 0xFFFD
    cpu.pc = cpu_read(0xFFFC) | (cpu_read(0xFFFD) << 8) ;
}

void IRQ()
{
    if ( !cpu.sr.i )
    {
        // write pc then sr to stack
        PUSH_D( cpu.pc );
        cpu.sr.b = 0; // for NMI and IRQ
        cpu.sr.i = 1; // disable interrupts
        uint8_t temp = pack( &cpu.sr );
        PUSH_S( temp ) ;
        // interrupt request vector is at 0xFFFE and 0xFFFF
        cpu.pc = cpu_read(0xFFFE) | (cpu_read(0xFFFF) << 8) ;

        cycles += 7;
    }
}

void NMI()
{
    // same as IRQ but we do not check for an interrupt flag
    //write pc then sr to stack
    PUSH_D( cpu.pc );
    cpu.sr.b = 0; // for NMI and IRQ
    cpu.sr.i = 1; // disable interrupts
    uint8_t temp = pack( &cpu.sr );
    PUSH_S(temp) ;
    // interrupt request vector is at 0xFFFA and 0xFFFB
    cpu.pc = cpu_read(0xFFFA) | (cpu_read(0xFFFB) << 8) ;

    cycles += 7;
}
void set_flags ( uint16_t operand , uint8_t code )
{ // simple code : 0 bit for C ; 1 bit for Z ; and bit 2 for N
    if ( code & 1 )
        cpu.sr.c = operand >> 8 ;
    if ( code & 2 )
        cpu.sr.z = ( ( operand & 0xFF ) == 0 ) ;
    if ( code & 4 )
        cpu.sr.n = ( operand & 0x80 ) >> 7 ;
}
void print_status ( int pc , uint8_t opcode )
{
    static size_t instr_count ;
    uint8_t sr = 0 ;
    sr = pack( &cpu.sr );
    printf("%zu:Instruction:%s pc:%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X CYCLES:%d\n",
           instr_count++ , inst_name[opcode] , pc  ,
           cpu.a_reg ,cpu.x_reg , cpu.y_reg , sr , cpu.sp , cycles + 7 );
}

static void unpack ( struct sr *current , uint8_t packed )
{
    current ->n = (packed & 0x80) >> 7 ;
    current ->v = (packed & 0x40) >> 6 ;
    //current ->b = (packed & 0x10) >> 4 ; // we dont read the break since it doesn't really exist
    current ->d = (packed & 0x08) >> 3 ;
    current ->i = (packed & 0x04) >> 2 ;
    current ->z = (packed & 0x02) >> 1 ;
    current ->c = packed ;
}

static uint8_t pack ( struct sr *current )
{
    uint8_t packed = 0 ;
    packed |= current -> n << 7 ;
    packed |= current -> v << 6 ;
    packed |= 0x20 ; // unused bit set to 1 for debugging purposes
    packed |= current -> b << 4 ;
    packed |= current -> d << 3 ;
    packed |= current -> i << 2 ;
    packed |= current -> z << 1 ;
    packed |= current -> c ;

    return packed;
}
