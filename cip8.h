#ifndef CIP8_H_
#define CIP8_H_
#include <assert.h>
#include <stdio.h>
#include <SDL2/SDL.h>


#define ADDR_INST 0x200
#define MAX_EXCUTED_INST 20
#define CURR_INST(cip) (cip->memory[cip->ip] << 8) | cip->memory[cip->ip + 1]
#define GET_ADDR(addr)  (addr & 0x0F00)| (addr & 0x00FF) 


#define BACKGROUND 0x000000
#define FOREGROUND 0x00FFFF
#define MEMORY_SIZE 4096
#define MAX_CYCLES -1

#define PRINT_INST true

// emulator memory:  [0x000,0x200], was avoided, now used to store font data
// disaply refresh memory:       [0xF00,0xFFF], 64 * 32 display
// call stack memeory    :       [0xEA0,0xEFF]
// Timer: count down to zero 60hz


typedef double Timer; 
typedef unsigned short Addr;
typedef uint16_t OpCode; 


typedef struct  {
    uint8_t memory[MEMORY_SIZE];
    uint8_t* display_refresh;
    uint8_t* call_stack; 

    Addr ip;
    Addr sp;

    struct {
        uint8_t V[16]; // VF for flags
        Addr I; // 12 bits used
    } regs;    

    Timer delay_timer;
    Timer sound_timer;
    size_t keyboard[16];


    bool blocked;
    bool halted;
    bool display_changed;
} Cip8;


typedef enum  {
    CALL,      
    CLD, 
    RET,
    GOTO,
    CALLS,
    JVEQ, 
    JVNEQ, 
    JEQ,
    MOV, 
    ADD, 
    ASS, 
    OR, 
    XOR, 
    AND, 
    ADDC, 
    SUBC,
    SHR, 
    SUBR,
    SHL,
    JNEQ,
    SETI,
    JMV0,
    RND,
    DRW,
    KEYD,
    KEYU,
    GETDT,
    GETK,
    SETDT,
    SETST,
    ADDI,
    SETISPR,     //spirte char??
    BCD,
    DUMP,
    LOAD,
} Operation;

typedef struct {
    Operation op;
    Addr oprand;
} Inst;

typedef struct {
    uint8_t val[3];
} Char;


void cip8_clear_display(Cip8*);


void cip8_init(Cip8* cip) { 
    cip->ip = ADDR_INST; // assuming code starts at ADDR_INST  
    cip->sp = 0xEFF;
    cip->display_refresh = cip->memory + 0xF00;
    cip->call_stack      = cip->memory + 0xEA0;
    
    for (size_t i = 0; i < 16; i++) {
        cip->keyboard[i] = 0;
    }
    for (size_t i = 0; i < MEMORY_SIZE; i++) {
        cip->memory[i] = 0;
    }
    cip->blocked = false;
    cip->halted = false;
    
    // characters
    const Char chars[16] = {
        (Char){.val = {0xF9,0x99,0xF0}}, // 0
        (Char){.val = {0x46,0x44,0xE0}}, // 1
        (Char){.val = {0xF8,0xF1,0xF0}}, // 2
        (Char){.val = {0xF8,0xF8,0xF0}}, // 3
        (Char){.val = {0x99,0xF8,0x80}}, // 4
        (Char){.val = {0xF1,0xF8,0xF0}}, // 5
        (Char){.val = {0xF1,0xF9,0xF0}}, // 6
        (Char){.val = {0xF8,0xF8,0x80}}, // 7
        (Char){.val = {0xF9,0xF9,0xF0}}, // 8
        (Char){.val = {0xF9,0xF8,0xF0}}, // 9
        (Char){.val = {0xF9,0xF9,0x90}}, // A
        (Char){.val = {0x79,0xF9,0x70}}, // B
        (Char){.val = {0xE1,0x11,0xE0}}, // C
        (Char){.val = {0x79,0x99,0x70}}, // D
        (Char){.val = {0xF1,0xF1,0xF0}}, // E
        (Char){.val = {0xF1,0xF1,0x10}}, // F
    };
    for (size_t i = 0; i < 16; i++) {
        cip->memory[3 * i]   = chars[i].val[0];
        cip->memory[3 * i + 1] = chars[i].val[1];
        cip->memory[3 * i + 2] = chars[i].val[2];


        cip->regs.V[i] = 0;
    }

    // pointing I to 0
    cip->memory[3 * 17 + 0] = cip->memory[3 * 0 + 0] >> 4;
    cip->memory[3 * 17 + 1] = cip->memory[3 * 0 + 0] & 0x0F;
    cip->memory[3 * 17 + 2] = cip->memory[3 * 0 + 1] >> 4;
    cip->memory[3 * 17 + 3] = cip->memory[3 * 0 + 1] & 0x0F;
    cip->memory[3 * 17 + 4] = cip->memory[3 * 0 + 2] >> 4;
    cip->memory[3 * 17 + 5] = cip->memory[3 * 0 + 2] & 0x0F;
    cip->regs.I = 17 * 3;    


    cip->display_changed = false;
}
void cip8_load_program(Cip8* cip, size_t size , OpCode* program) {
    size_t ip = ADDR_INST;
    

    // check if little endian and load
    int n = 1;
    if(*(char*)(&n) == 1) {
        for (size_t i = 0; i < size; i++) {
            cip->memory[ip + 2 * i    ] = (program[i] & 0x00FF) >> 0;
            cip->memory[ip + 2 * i + 1] = (program[i] & 0xFF00) >> 8;

        }
    } else {
        for (size_t i = 0; i < size; i++) {
            cip->memory[ip + 2 * i    ] = (program[i] & 0xFF00) >> 8;
            cip->memory[ip + 2 * i + 1] = (program[i] & 0x00FF) >> 0;
        }
    }
}
void cip8_clear_program(Cip8* cip,size_t size ) {
    size_t ip = ADDR_INST;
    for (size_t i = 0; i < size; i++) {
        cip->memory[ip + 2 * i    ] = 0x00;
        cip->memory[ip + 2 * i + 1] = 0x00;
    }
}
void cip8_print_code(const Cip8 cip, size_t start,size_t count) {
    for (size_t i = 0; i < count * 2; i+=2) {
        printf("0x%02X%02X\n",  cip.memory[start + i], cip.memory[start + i + 1]);
    }
}


Inst cip8_get_inst(OpCode code) {
    Inst inst;
    inst.oprand = code & 0x0FFF;

    switch ((code & 0xF000) >> (4 * 3))
    {
        case 0:
            switch (code & 0x00FF) {
                case 0xEE: inst.op = RET;      break;                                          
                case 0xE0: inst.op = CLD;      break;                                          
                default:
                    printf("op-code: 0x%02X\n",code);
                    assert(0 && "Unreachable unknown op-code 0X__");
                break;
            }
        break;
        case 1: inst.op = GOTO;     break;
        case 2: inst.op = CALLS;      break;
        case 3: inst.op = JEQ;      break;
        case 4: inst.op = JNEQ;     break;          
        case 5: inst.op = JVEQ;     break;
        case 6: inst.op = MOV;      break;   
        case 7: inst.op = ADD;      break;                                          
        case 8: 
            switch (code & 0x000F) {
                case 0:     inst.op = ASS;   break;
                case 1:     inst.op = OR;    break;
                case 2:     inst.op = AND;   break;           
                case 3:     inst.op = XOR;   break;        
                case 4:     inst.op = ADDC;  break;  
                case 5:     inst.op = SUBC;  break;     
                case 6:     inst.op = SHR;   break;           
                case 7:     inst.op = SUBR;  break;            
                case 14:  inst.op = SHL;   break;                                                                                                                               
                default: 
                    printf("op-code: 0x%X\n",code);
                    assert(0 && "Unreachable unknown op-code 8XX_");
                break;
            }
        break; 
        case 9: inst.op = JVNEQ;      break;                                          
        case 10: inst.op = SETI;      break;                                          
        case 11: inst.op = JMV0;      break;                                          
        case 12: inst.op = RND;      break;                                          
        case 13: inst.op = DRW;      break;                                          
        case 14: 
            switch (code & 0x00FF) {
                case 0x9E: inst.op = KEYD;      break;                                          
                case 0xA1: inst.op = KEYU;      break;                                          
                default:
                    printf("op-code: 0x%X\n",code);
                    assert(0 && "Unreachable unknown op-code EX__");
                break;
            }
        break;        
        case 15: 
            switch (code & 0x00FF) {
                case 0x07: inst.op = GETDT;      break;                                          
                case 0x0A: inst.op = GETK;      break;                                          
                case 0x15: inst.op = SETDT;      break;                                          
                case 0x18: inst.op = SETST;      break;                                          
                case 0x1E: inst.op = ADDI;      break;                                          
                case 0x29: inst.op = SETISPR;    break;                                          
                case 0x33: inst.op = BCD;      break;                                          
                case 0x55: inst.op = DUMP;      break;                                          
                case 0x65: inst.op = LOAD;      break;                                          
                default:
                    printf("op-code: 0x%X\n",code);
                    assert(0 && "Unreachable unknown op-code EX__");
                break;
            }
        break;                                             
        default: 
            printf("op-code: 0x%X\n",code);
            assert(0 && "Unreachable unknown op-code");
        break;
    }

    return inst;
}
void cip8_print_inst(Cip8 cip,Inst inst) {
    printf("0x%X     ",GET_ADDR(cip.ip));
    printf("0x%02X%02X     ",cip.memory[cip.ip] ,cip.memory[cip.ip+1]);
    switch (inst.op)
    {
        case CLD:   printf("CLD\n");  break;
        case RET:   printf("RET\n");  break; 
        
        case CALLS: printf("CALLS 0x%X\n",inst.oprand);  break;
        case SETI:  printf("SETI  0x%X\n",inst.oprand);     break;
        case JMV0:  printf("JMV0  0x%X\n",inst.oprand);     break;
        case GOTO:  printf("GOTO  0x%X\n",inst.oprand);     break;       

                                             
        case JEQ:   printf("JEQ   V%X 0x%X \n",inst.oprand >> 8,inst.oprand & 0x0FF);  break;         
        case JNEQ:  printf("JNEQ  V%X 0x%X\n",inst.oprand >> 8,inst.oprand & 0x0FF);  break;    
        case MOV:   printf("MOV   V%X 0x%X\n", inst.oprand >> 8,inst.oprand & 0x0FF);  break;    
        case ADD:   printf("ADD   V%X 0x%X\n", inst.oprand >> 8,inst.oprand & 0x0FF);  break;    
        case RND:   printf("RND   V%X V%X\n",  inst.oprand >> 8,inst.oprand & 0x0FF);  break;

        case JVEQ:  printf("JVEQ  V%X V%X\n", inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break;   
        case ASS:   printf("ASS   V%X V%X\n", inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break;
        case OR:    printf("OR    V%X V%X\n", inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break;
        case AND:   printf("AND   V%X V%X\n", inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break;
        case XOR:   printf("XOR   V%X V%X\n", inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break;
        case ADDC:  printf("ADDC  V%X V%X\n", inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break;
        case SUBC:  printf("SUBC  V%X V%X\n", inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break;
        case SHR:   printf("SHR   V%X V%X\n", inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break;
        case SUBR:  printf("SUBR  V%X V%X\n", inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break;
        case SHL:   printf("SHL   V%X V%X\n", inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break;          
        case JVNEQ: printf("JVNEQ V%X V%X\n",inst.oprand >> 8,(inst.oprand >> 4) & 0x0F);  break; 

        case KEYD:  printf("KEYD  V%X\n",inst.oprand >> 8);   break;
        case KEYU:  printf("KEYU  V%X\n",inst.oprand >> 8);   break;
        case BCD:   printf("BCD   V%X \n",inst.oprand >> 8);   break;
        case DUMP:  printf("DUMP  V%X\n",inst.oprand >> 8);   break;
        case LOAD:  printf("LOAD  V%X\n",inst.oprand >> 8);   break;
        case GETDT: printf("GETDT V%X\n",inst.oprand >> 8);  break;
        case GETK:  printf("GETK  V%X\n",inst.oprand >> 8);  break;
        case SETDT: printf("SETDT V%X\n",inst.oprand >> 8);  break;
        case SETST: printf("SETST V%X\n",inst.oprand >> 8);  break;
        case ADDI:  printf("ADDI  V%X\n",inst.oprand >> 8);  break;
        case SETISPR: printf("SETISPR V%X\n",inst.oprand >> 8); break;  

        case DRW:   printf("DRW   V%X V%X 0x%X\n",inst.oprand >> 8,(inst.oprand >> 4) & 0x0F,(inst.oprand) & 0x0F);  break;
        default: assert(0 && "Unreachable unknown inst"); break;
    }
}
void cip8_execute(Cip8* cip,Inst inst) {
    switch (inst.op)
    {
        case CLD: 
            cip8_clear_display(cip);
        break;  
        case RET: 
            cip->sp += 2;
            cip->ip = (cip->call_stack[cip->sp - 1] << 4) | (cip->call_stack[cip->sp]);
            printf("=================> ip: 0x%0X\n",cip->ip);
        break;  
        case GOTO: 
            cip->ip = inst.oprand;
        break;   
        case CALLS: 
            assert((cip->sp > 0xEA0)&& "overflowing the stack");
            cip->call_stack[cip->sp - 1]     = (cip->ip & 0xFF0) >> 4;
            cip->call_stack[cip->sp]         = (cip->ip & 0xF);
            cip->ip = inst.oprand;
            printf("=================> ip: 0x%0X\n",(cip->call_stack[cip->sp - 1] << 4) | cip->call_stack[cip->sp]);
            cip->sp -= 2;

        break;     
        case JEQ: {
            int a =  cip->regs.V[(inst.oprand >> 8)];
            int b =  inst.oprand & 0xFF;
            if(a == b) {
                cip->ip += 2; 
            }
        }
        break;
        case JNEQ: {
            int a =  cip->regs.V[(inst.oprand >> 8)];
            int b =  (inst.oprand & 0x0FF);
            if(a != b) {
                cip->ip += 2; 
            }
        }
        break;
        case JVEQ: {
            int a =  cip->regs.V[(inst.oprand >> 8)];
            int b =  cip->regs.V[(inst.oprand >> 4) & 0x0F];
            if(a == b) {
                cip->ip += 2; 
            }
        }
        break;
        case MOV:
            cip->regs.V[(inst.oprand >> 8)] = inst.oprand & 0x0FF; 
        break;
        case ADD:
            cip->regs.V[(inst.oprand >> 8)] += inst.oprand & 0x0FF; 
        break;        


        case ASS: 
            cip->regs.V[(inst.oprand >> 8)] = cip->regs.V[(inst.oprand >> 4) & 0x0F]; 
        break;
        case OR:  
            cip->regs.V[(inst.oprand >> 8)] |= cip->regs.V[(inst.oprand >> 4) & 0x0F]; 
        break;
        case AND: 
            cip->regs.V[(inst.oprand >> 8)] &= cip->regs.V[(inst.oprand >> 4) & 0x0F]; 
        break;
        case XOR: 
            cip->regs.V[(inst.oprand >> 8)] ^= cip->regs.V[(inst.oprand >> 4) & 0x0F]; 
        break;
        case ADDC: {
            uint16_t r = cip->regs.V[(inst.oprand >> 8)] + cip->regs.V[(inst.oprand >> 4) & 0x0F];   
            cip->regs.V[(inst.oprand >> 8)] += cip->regs.V[(inst.oprand >> 4) & 0x0F]; 
            cip->regs.V[0xF] = cip->regs.V[(inst.oprand >> 8)] != r;
        }
        break;
        case SUBC: {
            uint16_t r = cip->regs.V[(inst.oprand >> 8)] - cip->regs.V[(inst.oprand >> 4) & 0x0F];   
            cip->regs.V[(inst.oprand >> 8)] -= cip->regs.V[(inst.oprand >> 4) & 0x0F]; 
            cip->regs.V[0xF] = cip->regs.V[(inst.oprand >> 8)] == r; 
        }
        break;
        case SUBR: {
            uint16_t r = cip->regs.V[(inst.oprand >> 4) & 0x0F] - cip->regs.V[(inst.oprand >> 8)];   
            cip->regs.V[(inst.oprand >> 8)] = cip->regs.V[(inst.oprand >> 4) & 0x0F] - cip->regs.V[(inst.oprand >> 8)]; 
            cip->regs.V[0xF] = cip->regs.V[(inst.oprand >> 8)] == r; 
        }
        break;        
        case SHR:   {
            bool a = cip->regs.V[(inst.oprand >> 8)] & 0x1; 
            cip->regs.V[(inst.oprand >> 8)] >>= 1; 
            cip->regs.V[0xF] = a;
        }
        break;
        case SHL:  {
            bool a = cip->regs.V[(inst.oprand >> 8)] & 0x80; 
            cip->regs.V[(inst.oprand >> 8)] <<= 1; 
            cip->regs.V[0xF] = a;
        }
        break;    
        case JVNEQ: {
            int a =  cip->regs.V[(inst.oprand >> 8)];
            int b =  cip->regs.V[(inst.oprand >> 4) & 0x0F];
            if(a != b) {
                cip->ip += 2; 
            }

        }
        break;
        case SETI:   cip->regs.I = inst.oprand;    break;
        case JMV0:   cip->ip     = cip->regs.V[0] + inst.oprand;    break;
        case RND:    cip->regs.V[(inst.oprand >> 8)] = rand() % (inst.oprand & 0x0FF);    break;
        case KEYD:
            if(cip->keyboard[cip->regs.V[(inst.oprand >> 8)]]) {
                cip->ip += 2;;
            }       
        break;
        case KEYU:  
            if(!cip->keyboard[cip->regs.V[(inst.oprand >> 8)]]) {
                cip->ip += 2;;
            }       
        break;
        case GETDT:
            cip->regs.V[(inst.oprand >> 8)] = (int) cip->delay_timer;
        break;
        case GETK:
            cip->blocked = true;
            cip->ip -= 2;
            if(cip->keyboard[cip->regs.V[(inst.oprand >> 8)]]) {
                cip->blocked = false;
            }       
        break;
        
        case SETDT:      
            cip->delay_timer =  cip->regs.V[(inst.oprand >> 8)];
        break;
        case SETST:      
            cip->sound_timer =  cip->regs.V[(inst.oprand >> 8)];
        break;
        case ADDI:       
            cip->regs.I +=  cip->regs.V[(inst.oprand >> 8)];
        break;
        case BCD: {
            int a =  cip->regs.V[(inst.oprand >> 8)];
            cip->memory[cip->regs.I + 0] = (int) a / 100;
            cip->memory[cip->regs.I + 1] = (int) (a % 100) / 10 ;
            cip->memory[cip->regs.I + 2] = (int) a % 10;
        }      
        break;
        case DUMP: 
        {
            uint8_t end = inst.oprand >> 8;
            for (size_t i = 0; i <= end; i++) {
                cip->memory[cip->regs.I + i] = cip->regs.V[i];
            }
        }      
        break;
        case LOAD:
        {
            uint8_t end = inst.oprand >> 8;
            for (size_t i = 0; i <= end; i++) {
                cip->regs.V[i] = cip->memory[cip->regs.I + i];
            }
        }
        break;
        case DRW:     
            cip->display_changed = true;   
            int x = cip->regs.V[inst.oprand >> 8];
            int y = cip->regs.V[(inst.oprand >> 4) & 0x0F];
            int h = inst.oprand & 0x00F;


            for (size_t hi = 0; hi < h; hi++) {
                int y_pos = (y + hi) % 32;
                uint8_t a = cip->memory[cip->regs.I + hi];
                uint8_t byte = 
                                (( a >> 7)      << 0)  | 
                                (((a >> 6) & 1) << 1)  |
                                (((a >> 5) & 1) << 2)  |
                                (((a >> 4) & 1) << 3)  |
                                (((a >> 3) & 1) << 4)  |
                                (((a >> 2) & 1) << 5)  |
                                (((a >> 1) & 1) << 6)  |
                                (((a >> 0) & 1) << 7);

                if(x % 8 == 0) {
                    cip->display_refresh[y_pos * 8 + (int)(x / 8)    ] ^= byte;
                    if(cip->display_refresh[y_pos * 8 + (int)(x / 8)    ] != byte) {
                        cip->regs.V[0xF] = 1;
                    }
                } else {
                    cip->display_refresh[y_pos * 8 + (int)(x / 8)    ] ^= (byte << (x % 8));
                    cip->display_refresh[y_pos * 8 + (int)(x / 8) + 1] ^= (byte >> (8 - x % 8));
                    
                    // TODO: is this even right?
                    if (((cip->display_refresh[y_pos * 8 + (int)(x / 8)] << (x % 8)) | 
                    ((cip->display_refresh[y_pos * 8 + (int)(x / 8) + 1]) >> (8 - x % 8))) != byte) {
                        cip->regs.V[0xF] = 1;
                    }

                }
            }
        break;
        case SETISPR: 
        {
            uint8_t x = cip->regs.V[inst.oprand >> 8];
            assert(0 <= x && x <= 0xF && "Error: setting I to wrong character sprite");

            cip->memory[3 * 17 + 0] = cip->memory[3 * x + 0] >> 4;
            cip->memory[3 * 17 + 1] = cip->memory[3 * x + 0] & 0x0F;

            cip->memory[3 * 17 + 2] = cip->memory[3 * x + 1] >> 4;
            cip->memory[3 * 17 + 3] = cip->memory[3 * x + 1] & 0x0F;

            cip->memory[3 * 17 + 4] = cip->memory[3 * x + 2] >> 4;
            cip->memory[3 * 17 + 5] = cip->memory[3 * x + 2] & 0x0F;
            cip->regs.I = 17 * 3; 
        }                         
        break;  
        default: assert(0 && "Unreachable unknown inst"); break;
    }

}


void cip8_step(Cip8* cip) {
    Inst inst = cip8_get_inst(CURR_INST(cip));
    if(PRINT_INST){ 
        cip8_print_inst(*cip,inst);
    }
    

    cip->ip += 2;
    cip8_execute(cip,inst);
}
void  cip8_run(Cip8* cip) {
    for (size_t i = 0; i < MAX_EXCUTED_INST; i++) {
        cip8_step(cip);        
    }
}



void cip8_clear_display(Cip8* cip) {
    for(size_t y = 0; y < 32; y++) {
        for(size_t x = 0; x < 8; x++) {
            cip->display_refresh[x + y * 8] = 0x00;
        }
    }
}
void cip8_sdl_from_mem_to_texture(const Cip8 cip,SDL_Surface* surface,SDL_Texture* texture) {
    SDL_Rect rect = {0,0,1,1};
    SDL_FillRect(surface,0,BACKGROUND);
    for (size_t y = 0; y < 32; y++) {
        for (size_t x = 0; x < 8; x++) {
            uint8_t byte = cip.display_refresh[x + y * 8];  
            for (size_t b = 0; b < 8; b++) {
                if(byte & (1 << b)) { 
                    rect.x = x * 8 + b;
                    rect.y = y;
                    SDL_FillRect(surface,&rect,FOREGROUND);
                }
            }
        }
    }

    SDL_UpdateTexture(texture,0,surface->pixels,surface->pitch);
}
void cip8_from_mem_to_terminal(const Cip8 cip) { 
    printf("\033[2J\033[H");
    for (size_t y = 0; y < 32; y++) {
        for (size_t x = 0; x < 8; x++) {
            uint8_t byte = cip.display_refresh[x + y * 8];  
            for (size_t b = 0; b < 8; b++) {
                if(byte & (1 << b)) {
                    printf("#");
                } else {
                    printf(".");
                }
            }
        }
        printf("\n");
    }
}

#endif
