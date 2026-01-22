#include "../headers/cpu.h"

struct CPU{

    uint8_t   iram[256];
    uint8_t   xram[65536];
    uint8_t   rom[65536];
    
    uint16_t  PC;
    uint8_t opcode;

    bool   halted;
    
};

#define   A     0xE0
#define   B     0xF0
#define   PSW   0xD0
#define   SP    0x81

#define   DPL   0x82
#define   DPH   0x83

#define   CY    0x80
#define   AC    0x40
#define   OV    0x04
#define   P     0x01

static inline uint8_t parity(uint8_t v){ //if even number of 1s in ACC, returns 0, else returns 1
    uint8_t p = 0;
    for(int i=0;i<8;i++)
        p ^= (v >> i) & 1;
    return p;
}

void fetch(struct CPU* cpu){
    if(cpu->halted) return;
    cpu->opcode = cpu->rom[cpu->PC++];
    
}

void execute(struct CPU* cpu){

    if(cpu->halted) return;

    uint8_t psw = cpu->iram[PSW];
    uint8_t curr_bank = ((psw>>3) & 0x03) * 8;

    switch(cpu->opcode){

        case 0x00 ... 0x07:{
            //MOV A, Rn
            uint8_t reg = cpu->opcode & 0x07;
            cpu->iram[A] = cpu->iram[curr_bank + reg];
            break;
        }

        case 0x08:{
            //MOV A, Direct
            cpu->iram[A] = cpu->iram[ cpu->rom[ cpu->PC++ ] ];
            break;
        }

        case 0x09:{
            //MOV A, #data
            cpu->iram[A] = cpu->rom[ cpu->PC++ ];
            break;
        }

        case 0x0A:{
            //MOV A, @R0
            cpu->iram[A] = cpu->iram[ cpu->iram[ curr_bank + 0 ] ];
            break;
        }
        case 0x0B:{
            //MOV A, @R1
            cpu->iram[A] = cpu->iram[ cpu->iram[ curr_bank + 1 ] ];
            break;
        }

        case 0x0C ... 0x13:{
            //MOV Rn, A
            cpu->iram[ curr_bank + cpu->opcode - 0x0C ] = cpu->iram[A];
            break;
        }

        case 0x14 ... 0x1B:{
            //MOV Rn, Direct
            cpu->iram[ curr_bank + cpu->opcode - 0x14 ] = cpu->iram[ cpu->rom[ cpu->PC++ ] ];
            
            break;
        }

        case 0x1C ... 0x23:{
            //MOV Rn, #data
            cpu->iram[ curr_bank + cpu->opcode - 0x1C ] = cpu->rom[ cpu->PC++ ];
            
            break;
        }

        case 0x24 ... 0x2B:{
            //MOV Direct, Rn
            cpu->iram[ cpu->rom[ cpu->PC++ ] ] = cpu->iram[ curr_bank + cpu->opcode - 0x24 ];
            
            break;
        }

        case 0x2C:{
            //MOV Direct, @R0
            cpu->iram[ cpu->rom[ cpu->PC++ ] ] = cpu->iram[ cpu->iram[ curr_bank + 0 ] ];
            
            break;
        }

        case 0x2D:{
            //MOV Direct, @R1
            cpu->iram[ cpu->rom[ cpu->PC++ ] ] = cpu->iram[ cpu->iram[ curr_bank + 1 ] ];
            
            break;
        }

        case 0x2E:{
            //MOV Direct, Direct
            uint8_t dest_addr = cpu->rom[cpu->PC++]; 
            uint8_t src_addr  = cpu->rom[cpu->PC++]; 
            cpu->iram[dest_addr] = cpu->iram[src_addr];
            
            break;
        }

        case 0x2F:{
            //MOV Direct, #data
            uint8_t dest_addr = cpu->rom[cpu->PC++]; 
            uint8_t data  = cpu->rom[cpu->PC++]; 
            cpu->iram[dest_addr] = data;
            
            break;
        }

        case 0x30 ... 0x31:{
            //MOV @R0/@R1, A
            cpu->iram[ cpu->iram[ curr_bank + cpu->opcode - 0x30 ] ] = cpu->iram[A];
            
            break;
        }

        case 0x32 ... 0x33:{
            //MOV @R0/@R1, Direct
            cpu->iram[ cpu->iram[ curr_bank + cpu->opcode - 0x32 ] ] = cpu->iram[ cpu->rom[cpu->PC++] ];
            
            break;
        }

        case 0x34 ... 0x35:{
            //MOV @R0/@R1, Data
            cpu->iram[ cpu->iram[ curr_bank + cpu->opcode - 0x34 ] ] = cpu->rom[ cpu->PC++ ];
            
            break;
        }

        case 0x36:{
            //MOV DPTR, #data 16 { HIGH BYTE, LOW BYTE}
            cpu->iram[DPH]=cpu->rom[cpu->PC++];
            cpu->iram[DPL]=cpu->rom[cpu->PC++];
            
            break;
        }

        case 0x37:{
            //MOV Direct, A
            cpu->iram[ cpu->rom[cpu->PC++] ] = cpu->iram[A];
            
            break;
        }

        case 0x38 ... 0x3F:{
            //ADD A, Rn
            cpu->iram[A] += cpu->iram[ curr_bank + cpu->opcode - 0x38];
            
            break;
        }

        case 0x40:{
            //ADD A, Direct
            cpu->iram[A] += cpu->iram[ cpu->rom[cpu->PC++] ];
            
            break;
        }

        case 0x41 ... 0x42:{
            //ADD A, @R0/ @R1
            cpu->iram[A] += cpu->iram[ cpu->iram[curr_bank + cpu->opcode - 0x41] ];
            
            break;
        }

        case 0x43:{
            //ADD A, #data
            cpu->iram[A] += cpu->rom[cpu->PC++];
            
            break;
        }

        // ADDC
        case 0x44 ... 0x4F: {
            uint16_t temp = 0;
        
            uint8_t a  = cpu->iram[A];

            uint8_t b;
            switch (cpu->opcode)
            {
                // ADDC A, Rn
                case 0x44 ... 0x4B:
                    b  = cpu->iram[curr_bank + cpu->opcode - 0x44];
                    break;

                // ADDC A, Direct
                case 0x4C:
                    b = cpu->iram[cpu->rom[cpu->PC++]];
                    break;
                
                // ADDC A, @R0/@R1
                case 0x4D ... 0x4E:
                    b = cpu->iram[cpu->iram[curr_bank + cpu->opcode - 0x4D]];
                    break;
                
                // ADDC A, #data
                case 0x4F:
                    b = cpu->rom[cpu->PC++];
                    break;
            }
            
            uint8_t cy = (cpu->iram[PSW] & CY) ? 1 : 0;
        
            temp = a + b + cy;
        
            // clear old flags
            cpu->iram[PSW] &= ~(CY | AC | OV);
        
            // overflow flag
            if ( (~(a ^ b) & (a ^ (uint8_t)temp) & 0x80) )
                cpu->iram[PSW] |= OV;
        
            // carry flag
            if (temp > 0xFF)
                cpu->iram[PSW] |= CY;
        
            // auxiliary carry for BCD
            if ( ((a & 0x0F) + (b & 0x0F) + cy) > 0x0F )
                cpu->iram[PSW] |= AC;
        
            cpu->iram[A] = (uint8_t)temp;
        
            break;
        }

        // SUBB 
        case 0x50 ... 0x5B: {
            int16_t temp = 0;
            
            uint8_t a = cpu->iram[A];
            
            uint8_t b;
            switch (cpu->opcode)
            {
                // SUBB A, Rn
                case 0x50 ... 0x57:
                    b = cpu->iram[curr_bank + cpu->opcode - 0x50];
                    break;

                // SUBB A, Direct
                case 0x58:
                    b = cpu->iram[cpu->rom[cpu->PC++]];
                    break;
                
                // SUBB A, @R0/@R1
                case 0x59 ... 0x5A:
                    b = cpu->iram[cpu->iram[curr_bank + cpu->opcode - 0x59]];
                    break;
                
                // SUBB A, #data
                case 0x5B:
                    b = cpu->rom[cpu->PC++];
                    break;
            }

            uint8_t cy = (cpu->iram[PSW] & CY) ? 1 : 0;
            
            // SUBB performs: A - B - CY
            temp = (int16_t)a - (int16_t)b - (int16_t)cy;
            
            // Clear old flags
            cpu->iram[PSW] &= ~(CY | AC | OV);
            
            // Overflow flag (for signed subtraction)
            // Overflow occurs if:
            // 1. Positive - Negative = Negative (underflow)
            // 2. Negative - Positive = Positive (overflow)
            // Using: (a ^ b) & (a ^ temp) & 0x80
            if (((a ^ b) & (a ^ (uint8_t)temp)) & 0x80) {
                cpu->iram[PSW] |= OV;
            }
            
            // Carry flag (borrow)
            if (temp < 0) {
                cpu->iram[PSW] |= CY;
            }
            
            // Auxiliary carry
            // set if there was a borrow from bit 3 to bit 4
            if (((a & 0x0F) - (b & 0x0F) - cy) < 0) {
                cpu->iram[PSW] |= AC;
            }
            
            cpu->iram[A] = (uint8_t)temp;
            
            break;
        }

        // INC
        case 0x5C ... 0x68: {
            uint8_t mem_addr;
            switch (cpu->opcode)
            {
                // INC A
                case 0x5C:
                    mem_addr = A;
                    break;

                // INC Rn
                case 0x5D ... 0x64:
                    mem_addr = curr_bank + cpu->opcode - 0x5D;
                    break;
                
                // INC Direct
                case 0x65:
                    mem_addr = cpu->rom[cpu->PC++];
                    break;

                // INC @R0/@R1
                case 0x66 ... 0x67:
                    mem_addr = cpu->iram[curr_bank + cpu->opcode - 0x66];
                    break;

                // INC DPTR
                case 0x68:
                    mem_addr = DPL;
                    
                    break;
            }

            // increment the value at that address
            // original value at that address before incrementing
            uint8_t org_value = (cpu->iram[mem_addr])++;

            // one special case - INC DPTR
            // if DPL overflows, we have to increment DPH
            if (org_value == 255) (cpu->iram[DPH])++;
            break;
        }

        
        default:
            fprintf(stderr, "opcode not implemented\n");
            cpu->halted=true;
            break;          
    }

    //UPDATES PARITY
    if(!cpu->halted){
        cpu->iram[PSW] = (cpu->iram[PSW] & ~P) | parity(cpu->iram[A]);
    }

}


int main(){}