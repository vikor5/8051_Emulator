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

        case 0x44 ... 0x4B: {
            // ADDC A, Rn
            uint16_t temp = 0;
        
            uint8_t a  = cpu->iram[A];
            uint8_t b  = cpu->iram[curr_bank + cpu->opcode - 0x44];
            uint8_t cy = (cpu->iram[PSW] & CY) ? 1 : 0;
        
            temp = a + b + cy;
        
            //clear old carrys
            cpu->iram[PSW] &= ~(CY | AC | OV);
        
            //overflow
            if ( (~(a ^ b) & (a ^ (uint8_t)temp) & 0x80) )
                cpu->iram[PSW] |= OV;
        
            //carry
            if (temp > 0xFF)
                cpu->iram[PSW] |= CY;
        
            //auxiliary carry
            if ( ((a & 0x0F) + (b & 0x0F) + cy) > 0x0F )
                cpu->iram[PSW] |= AC;
        
            cpu->iram[A] = (uint8_t)temp;
        
            break;
        }

        case 0x4C: {
            // ADDC A, Direct
            uint16_t temp = 0;
        
            uint8_t a  = cpu->iram[A];
            uint8_t b  = cpu->iram[cpu->rom[cpu->PC++]];
            uint8_t cy = (cpu->iram[PSW] & CY) ? 1 : 0;
        
            temp = a + b + cy;
        
            //clear old carrys
            cpu->iram[PSW] &= ~(CY | AC | OV);
        
            //overflow
            if ( (~(a ^ b) & (a ^ (uint8_t)temp) & 0x80) )
                cpu->iram[PSW] |= OV;
        
            //carry
            if (temp > 0xFF)
                cpu->iram[PSW] |= CY;
        
            //auxiliary carry
            if ( ((a & 0x0F) + (b & 0x0F) + cy) > 0x0F )
                cpu->iram[PSW] |= AC;
        
            cpu->iram[A] = (uint8_t)temp;
        
            break;
        }

        case 0x4D ... 0x4E:{
            //ADDC A, @R0/@R1

            uint16_t temp = 0;
        
            uint8_t a  = cpu->iram[A];
            uint8_t b  = cpu->iram[cpu->iram[curr_bank + cpu->opcode - 0x4D]];
            uint8_t cy = (cpu->iram[PSW] & CY) ? 1 : 0;
        
            temp = a + b + cy;
        
            //clear old carrys
            cpu->iram[PSW] &= ~(CY | AC | OV);
        
            //overflow
            if ( (~(a ^ b) & (a ^ (uint8_t)temp) & 0x80) )
                cpu->iram[PSW] |= OV;
        
            //carry
            if (temp > 0xFF)
                cpu->iram[PSW] |= CY;
        
            //auxiliary carry
            if ( ((a & 0x0F) + (b & 0x0F) + cy) > 0x0F )
                cpu->iram[PSW] |= AC;
        
            cpu->iram[A] = (uint8_t)temp;
        
            break;
        }

        case 0x4F:{
            //ADDC A, #data

            uint16_t temp = 0;
        
            uint8_t a  = cpu->iram[A];
            uint8_t b  = cpu->rom[cpu->PC++];
            uint8_t cy = (cpu->iram[PSW] & CY) ? 1 : 0;
        
            temp = a + b + cy;
        
            //clear old carrys
            cpu->iram[PSW] &= ~(CY | AC | OV);
        
            //overflow
            if ( (~(a ^ b) & (a ^ (uint8_t)temp) & 0x80) )
                cpu->iram[PSW] |= OV;
        
            //carry
            if (temp > 0xFF)
                cpu->iram[PSW] |= CY;
        
            //auxiliary carry
            if ( ((a & 0x0F) + (b & 0x0F) + cy) > 0x0F )
                cpu->iram[PSW] |= AC;
        
            cpu->iram[A] = (uint8_t)temp;
        
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