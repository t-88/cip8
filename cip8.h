#ifndef CIP8_H_
#define CIP8_H_
#include <assert.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define ENABLE_PRINT_DEBUG false


#define PROGRAM_START 0x200
#define MAX_EXCUTED_INST 20
#define CURR_INST(cip) (cip->memory[cip->ip] << 8) | cip->memory[cip->ip + 1]

#define GET_N(code) (code) & 0xF
#define GET_NN(code) (code) & 0xFF
#define GET_NNN(code) (code)


#define GET_X(code) ((code) >> 4 * 2) & 0xF 
#define GET_Y(code) ((code) >> 4 * 1) & 0xF 
#define GET_VX(code) cip->regs.V[GET_X(code) ] 
#define GET_VY(code) cip->regs.V[GET_Y(code)] 
#define SET_FLAG(cip,val) (cip)->regs.V[0xF] = (val)
#define GET_KEY(i) cip->keyboard[(i)]



typedef double Timer; 
typedef unsigned short Addr;
typedef uint16_t OpCode; 


#define BACKGROUND 0x000000
#define FOREGROUND 0x00FFFF
#define MEMORY_SIZE 4096
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
    bool waiting_release;
} Cip8;


typedef enum  {
    OP_CALL,      
    OP_CLD, 
    OP_RET,
    OP_GOTO,
    OP_CALLS,
    OP_JVEQ, 
    OP_JVNEQ, 
    OP_JEQ,
    OP_MOV, 
    OP_ADD, 
    OP_ASS, 
    OP_OR, 
    OP_XOR, 
    OP_AND, 
    OP_ADDC, 
    OP_SUBC,
    OP_SHR, 
    OP_SUBR,
    OP_SHL,
    OP_JNEQ,
    OP_SETI,
    OP_JMV0,
    OP_RND,
    OP_DRW,
    OP_KEYD,
    OP_KEYU,
    OP_GETDT,
    OP_GETK,
    OP_SETDT,
    OP_SETST,
    OP_ADDI,
    SETISPR,     //spirte char??
    OP_BCD,
    OP_DUMP,
    OP_LOAD,
} Operation;
typedef struct {
    Operation op;
    uint16_t oprand;
} Inst;
typedef struct {
    uint8_t val[3];
} Char;

void cip8_init(Cip8* cip); 
void cip8_load_program(Cip8* cip, size_t size , OpCode* program);
void cip8_print_program(const Cip8 cip, size_t start,size_t count);
Inst cip8_compile_inst(OpCode code);
void cip8_print_inst(Cip8 cip,Inst inst);
void cip8_execute(Cip8* cip,Inst inst);
void cip8_step(Cip8* cip);
void  cip8_run(Cip8* cip);
void cip8_clear_display(Cip8* cip);
void cip8_sdl_from_mem_to_texture(const Cip8 cip,SDL_Surface* surface,SDL_Texture* texture);
void cip8_from_mem_to_terminal(const Cip8 cip); 
void cip8_write_char(Cip8* cip, uint8_t i);


void cip8_init(Cip8* cip) { 
    cip->ip = PROGRAM_START; 
    cip->sp = 0xEFF;
    cip->display_refresh = cip->memory + 0xF00;
    cip->call_stack      = cip->memory + 0xEA0;
    

    for (size_t i = 0; i < MEMORY_SIZE; i++)  cip->memory[i] = 0;

    cip->blocked = false;
    cip->halted = false;
    cip->waiting_release = false;
    cip->display_changed = false;

    
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
        cip->keyboard[i] = 0; // Reset keyboard 
        cip->regs.V[i] = 0;  // Reset Regs

        // OP_LOAD Font
        cip->memory[3 * i]   = chars[i].val[0];
        cip->memory[3 * i + 1] = chars[i].val[1];
        cip->memory[3 * i + 2] = chars[i].val[2];        
    }


    cip8_write_char(cip,0);

}

// every time i draw a font, i OP_LOAD it to memory location OP_AND point I to it 
void cip8_write_char(Cip8* cip, uint8_t i) { 
    for (size_t j = 0; j < 3; j++) {
        cip->memory[3 * 17 + j + 0] = cip->memory[3 * i + j] >> 4;
        cip->memory[3 * 17 + j + 1] = cip->memory[3 * i + j] & 0x0F;
    }
    cip->regs.I = 17 * 3;    
}


void cip8_load_program(Cip8* cip, size_t size , OpCode* program) {
    size_t ip = PROGRAM_START;

    // check if little endian AND OP_LOAD
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
void cip8_print_program(const Cip8 cip, size_t start,size_t count) {
    for (size_t i = 0; i < count * 2; i+=2) {
        printf("0x%02X%02X\n",  cip.memory[start + i], cip.memory[start + i + 1]);
    }
}
Inst cip8_compile_inst(OpCode code) {
    Inst inst;
    inst.oprand = code & 0x0FFF;

    switch ((code & 0xF000) >> (4 * 3))
    {
        case 0:
            switch (code & 0x00FF) {
                case 0xEE: inst.op = OP_RET;      break;                                          
                case 0xE0: inst.op = OP_CLD;      break;                                          
                default:
                    printf("op-code: 0x%02X\n",code);
                    assert(0 && "Unreachable unknown op-code 0X__");
                break;
            }
        break;
        case 1: inst.op = OP_GOTO;     break;
        case 2: inst.op = OP_CALLS;      break;
        case 3: inst.op = OP_JEQ;      break;
        case 4: inst.op = OP_JNEQ;     break;          
        case 5: inst.op = OP_JVEQ;     break;
        case 6: inst.op = OP_MOV;      break;   
        case 7: inst.op = OP_ADD;      break;                                          
        case 8: 
            switch (code & 0x000F) {
                case 0:     inst.op = OP_ASS;   break;
                case 1:     inst.op = OP_OR;    break;
                case 2:     inst.op = OP_AND;   break;           
                case 3:     inst.op = OP_XOR;   break;        
                case 4:     inst.op = OP_ADDC;  break;  
                case 5:     inst.op = OP_SUBC;  break;     
                case 6:     inst.op = OP_SHR;   break;           
                case 7:     inst.op = OP_SUBR;  break;            
                case 14:  inst.op = OP_SHL;   break;                                                                                                                               
                default: 
                    printf("op-code: 0x%X\n",code);
                    assert(0 && "Unreachable unknown op-code 8XX_");
                break;
            }
        break; 
        case 9: inst.op = OP_JVNEQ;      break;                                          
        case 10: inst.op = OP_SETI;      break;                                          
        case 11: inst.op = OP_JMV0;      break;                                          
        case 12: inst.op = OP_RND;      break;                                          
        case 13: inst.op = OP_DRW;      break;                                          
        case 14: 
            switch (code & 0x00FF) {
                case 0x9E: inst.op = OP_KEYD;      break;                                          
                case 0xA1: inst.op = OP_KEYU;      break;                                          
                default:
                    printf("op-code: 0x%X\n",code);
                    assert(0 && "Unreachable unknown op-code EX__");
                break;
            }
        break;        
        case 15: 
            switch (code & 0x00FF) {
                case 0x07: inst.op = OP_GETDT;      break;                                          
                case 0x0A: inst.op = OP_GETK;      break;                                          
                case 0x15: inst.op = OP_SETDT;      break;                                          
                case 0x18: inst.op = OP_SETST;      break;                                          
                case 0x1E: inst.op = OP_ADDI;      break;                                          
                case 0x29: inst.op = SETISPR;    break;                                          
                case 0x33: inst.op = OP_BCD;      break;                                          
                case 0x55: inst.op = OP_DUMP;      break;                                          
                case 0x65: inst.op = OP_LOAD;      break;                                          
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
    printf("0x%X     ",cip.ip);
    printf("0x%02X%02X     ",cip.memory[cip.ip] ,cip.memory[cip.ip+1]);
    switch (inst.op)
    {
        case OP_CLD:   printf("OP_CLD\n");  break;
        case OP_RET:   printf("OP_RET\n");  break; 
        
        case OP_CALLS: printf("OP_CALLS 0x%X\n",GET_NNN(inst.oprand));  break;
        case OP_SETI:  printf("OP_SETI  0x%X\n",GET_NNN(inst.oprand));     break;
        case OP_JMV0:  printf("OP_JMV0  0x%X\n",GET_NNN(inst.oprand));     break;
        case OP_GOTO:  printf("OP_GOTO  0x%X\n",GET_NNN(inst.oprand));     break;       

                                             
        case OP_JEQ:   printf("OP_JEQ   V%X 0x%X \n",GET_X(inst.oprand),GET_NN(inst.oprand));  break;         
        case OP_JNEQ:  printf("OP_JNEQ  V%X 0x%X\n", GET_X(inst.oprand),GET_NN(inst.oprand));  break;    
        case OP_MOV:   printf("OP_MOV   V%X 0x%X\n", GET_X(inst.oprand),GET_NN(inst.oprand));  break;    
        case OP_ADD:   printf("OP_ADD   V%X 0x%X\n", GET_X(inst.oprand),GET_NN(inst.oprand));  break;    
        case OP_RND:   printf("OP_RND   V%X V%X\n",  GET_X(inst.oprand),GET_NN(inst.oprand));  break;

        case OP_JVEQ:  printf("OP_JVEQ  V%X V%X\n", GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break;   
        case OP_ASS:   printf("OP_ASS   V%X V%X\n", GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break;
        case OP_OR:    printf("OP_OR    V%X V%X\n", GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break;
        case OP_AND:   printf("OP_AND   V%X V%X\n", GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break;
        case OP_XOR:   printf("OP_XOR   V%X V%X\n", GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break;
        case OP_ADDC:  printf("OP_ADDC  V%X V%X\n", GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break;
        case OP_SUBC:  printf("OP_SUBC  V%X V%X\n", GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break;
        case OP_SHR:   printf("OP_SHR   V%X V%X\n", GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break;
        case OP_SUBR:  printf("OP_SUBR  V%X V%X\n", GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break;
        case OP_SHL:   printf("OP_SHL   V%X V%X\n", GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break;          
        case OP_JVNEQ: printf("OP_JVNEQ V%X V%X\n",GET_X(inst.oprand),GET_Y(inst.oprand) & 0x0F);  break; 

        case OP_KEYD:  printf("OP_KEYD  V%X\n",GET_X(inst.oprand));   break;
        case OP_KEYU:  printf("OP_KEYU  V%X\n",GET_X(inst.oprand));   break;
        case OP_BCD:   printf("OP_BCD   V%X \n",GET_X(inst.oprand));   break;
        case OP_DUMP:  printf("OP_DUMP  V%X\n",GET_X(inst.oprand));   break;
        case OP_LOAD:  printf("OP_LOAD  V%X\n",GET_X(inst.oprand));   break;
        case OP_GETDT: printf("OP_GETDT V%X\n",GET_X(inst.oprand));  break;
        case OP_GETK:  printf("OP_GETK  V%X\n",GET_X(inst.oprand));  break;
        case OP_SETDT: printf("OP_SETDT V%X\n",GET_X(inst.oprand));  break;
        case OP_SETST: printf("OP_SETST V%X\n",GET_X(inst.oprand));  break;
        case OP_ADDI:  printf("OP_ADDI  V%X\n",GET_X(inst.oprand));  break;
        case SETISPR: printf("SETISPR V%X\n",GET_X(inst.oprand)); break;  

        case OP_DRW:   printf("OP_DRW   V%X V%X 0x%X\n",GET_X(inst.oprand),GET_Y(inst.oprand),GET_N(inst.oprand));  break;
        default: assert(0 && "Unreachable unknown inst"); break;
    }
}
void cip8_execute(Cip8* cip,Inst inst) {
    switch (inst.op)
    {
        case OP_CLD:  cip8_clear_display(cip); break;  
        case OP_GOTO: cip->ip = GET_NNN(inst.oprand); break; 
        case OP_MOV: GET_VX(inst.oprand) = GET_NN(inst.oprand);  break;
        case OP_ADD: GET_VX(inst.oprand) += GET_NN(inst.oprand); break;        

        case OP_RET: 
            cip->sp += 2;
            cip->ip = (cip->call_stack[cip->sp - 1] << 4) | (cip->call_stack[cip->sp]);
        break;  
        case OP_CALLS: 
            assert((cip->sp > 0xEA0) && "overflowing the stack");
            cip->call_stack[cip->sp - 1]     = (cip->ip & 0xFF0) >> 4;
            cip->call_stack[cip->sp]         = (cip->ip & 0xF);
            cip->sp -= 2;
            cip->ip = GET_NNN(inst.oprand);
        break;     
        case OP_JEQ: {
            int vx =  GET_VX(inst.oprand);
            int nn =  GET_NN(inst.oprand);
            if(vx == nn) {
                cip->ip += 2; 
            }
        }
        break;
        case OP_JNEQ: {
            int vx =  GET_VX(inst.oprand);
            int nn =  GET_NN(inst.oprand);
            if(vx != nn) {
                cip->ip += 2; 
            }
        }
        break;
        case OP_JVEQ: {
            int vx =  GET_VX(inst.oprand);
            int vy =  GET_VY(inst.oprand);
            if(vx == vy) {
                cip->ip += 2; 
            }
        }
        break;
        case OP_JVNEQ: {
            int vx =  GET_VX(inst.oprand);
            int vy =  GET_VY(inst.oprand);
            if(vx != vy) {
                cip->ip += 2; 
            }
        }        



        case OP_ASS:  GET_VX(inst.oprand) = GET_VY(inst.oprand); break;
        case OP_OR:   GET_VX(inst.oprand) |= GET_VY(inst.oprand);  break;
        case OP_AND: GET_VX(inst.oprand) &= GET_VY(inst.oprand);  break;
        case OP_XOR: GET_VX(inst.oprand) ^= GET_VY(inst.oprand);  break;


        case OP_ADDC: {
            uint16_t r = GET_VX(inst.oprand) + GET_VY(inst.oprand);   
            GET_VX(inst.oprand) += GET_VY(inst.oprand); 
            SET_FLAG(cip,GET_VX(inst.oprand) != r);
        }
        break;
        case OP_SUBC: {
            uint16_t r = GET_VX(inst.oprand) - GET_VY(inst.oprand);   
            GET_VX(inst.oprand) -= GET_VY(inst.oprand); 
            SET_FLAG(cip,GET_VX(inst.oprand) == r);
        }
        break;
        case OP_SUBR: {
            uint16_t r = GET_VY(inst.oprand) - GET_VX(inst.oprand);   
            GET_VX(inst.oprand) = GET_VY(inst.oprand) - GET_VX(inst.oprand); 
            SET_FLAG(cip,GET_VX(inst.oprand) == r);
        }
        break;        
        case OP_SHR:   {
            bool a = GET_VX(inst.oprand) & 0x1; 
            GET_VX(inst.oprand) >>= 1; 
            cip->regs.V[0xF] = a;
        }
        break;
        case OP_SHL:  {
            bool a = GET_VX(inst.oprand) & 0x80; 
            GET_VX(inst.oprand) <<= 1; 
            SET_FLAG(cip,a);
        }
        break;    

        case OP_SETI:   cip->regs.I = inst.oprand;    break;
        case OP_JMV0:   cip->ip     = cip->regs.V[0] + inst.oprand;    break;
        case OP_RND:    GET_VX(inst.oprand) = rand() % (inst.oprand & 0x0FF);    break;

        case OP_KEYD:
            if(GET_KEY(GET_VX(inst.oprand))) {
                cip->ip += 2;
            }       
        break;
        case OP_KEYU:  
            if(!GET_KEY(GET_VX(inst.oprand))) {
                cip->ip += 2;;
            }       
        break;
        case OP_GETK:
            cip->ip -= 2;
            for (size_t i = 0; i < 16; i++) {
                if(GET_KEY(i)) {
                    GET_VX(inst.oprand) = i;
                    cip->waiting_release = true;
                    break;
                }
            }
            if(!GET_KEY(GET_VX(inst.oprand)) && cip->waiting_release) {
                cip->ip += 2;
                cip->waiting_release = true;
            }
        break;


        case OP_GETDT: GET_VX(inst.oprand) = (int) cip->delay_timer; break;
        case OP_SETDT: cip->delay_timer =  GET_VX(inst.oprand); break;
        case OP_SETST: cip->sound_timer =  GET_VX(inst.oprand); break;
        case OP_ADDI:  cip->regs.I +=  GET_VX(inst.oprand);     break;

        case OP_BCD: {
            int vx =  GET_VX(inst.oprand);
            cip->memory[cip->regs.I + 0] = (int) vx / 100;
            cip->memory[cip->regs.I + 1] = (int) (vx % 100) / 10 ;
            cip->memory[cip->regs.I + 2] = (int) vx % 10;
        }      
        break;
        case OP_DUMP: 
        {
            uint8_t end = inst.oprand >> 8;
            for (size_t i = 0; i <= end; i++) {
                cip->memory[cip->regs.I + i] = cip->regs.V[i];
            }
        }      
        break;
        case OP_LOAD:
        {
            uint8_t end = inst.oprand >> 8;
            for (size_t i = 0; i <= end; i++) {
                cip->regs.V[i] = cip->memory[cip->regs.I + i];
            }
        }
        break;


        case OP_DRW:     
            cip->display_changed = true;   
            int x = cip->regs.V[inst.oprand >> 8];
            int y = GET_VY(inst.oprand);
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

        case SETISPR: {
            uint8_t vx = GET_VX(inst.oprand);
            assert(0 <= vx && vx <= 0xF && "Error: setting I to wrong character sprite");
            cip8_write_char(cip,vx);
        }                         
        break;  
        
        default:
            printf("ints: %X%X\n",inst.op,inst.oprand);
            assert(0 && "Unreachable unknown inst");
        break;
    }

}
void cip8_step(Cip8* cip) {
    Inst inst = cip8_compile_inst(CURR_INST(cip));
    if(ENABLE_PRINT_DEBUG){ 
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
