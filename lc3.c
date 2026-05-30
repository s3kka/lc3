#if _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>

// get a bitmask with n bit set
#define LC3_BITMASK(n)      ((1 << (n)) - 1)
// get a bitmask with a single bit set at i (0 as least-significant bit)
#define LC3_BIT(i)          (1 << (i))
// get a field of bits of value starting from start_pos (0 as least-significant bit) and to n bits
#define LC3_GET_BITS(value, start_pos, n) (((value) >> (start_pos)) & LC3_BITMASK(n))

#if !LC3_LOG_DISABLED
#define lc3_Log(...) printf(__VA_ARGS__)
#else
#define lc3_Log(...) 
#endif

#define LC3_MAX_MEMORY 65536 // maximum addressable by a uint16

typedef enum lc3_Register {
    // general purpose
    LC3_REG_R0 = 0,
    LC3_REG_R1,
    LC3_REG_R2,
    LC3_REG_R3,
    LC3_REG_R4,
    LC3_REG_R5,
    LC3_REG_R6,
    LC3_REG_R7,

    // program counter
    LC3_REG_PC,

    // condition flags
    LC3_REG_COND,

    LC3_REG_COUNT,
} lc3_Register;

typedef enum lc3_Opcode {
    LC3_OPCODE_ADD  = 0x1,    // addition
    LC3_OPCODE_AND  = 0x5,    // bitwise logical and
    LC3_OPCODE_BR   = 0x0,    // conditional branch
    LC3_OPCODE_JMP  = 0xC,    // jump
    LC3_OPCODE_JSR  = 0x4,    // jump register 
    LC3_OPCODE_LD   = 0x2,    // load
    LC3_OPCODE_LDI  = 0xA,    // load indirect
    LC3_OPCODE_LDR  = 0x6,    // load register
    LC3_OPCODE_LEA  = 0xE,    // load effective address
    LC3_OPCODE_NOT  = 0x9,    // bitwise not
    LC3_OPCODE_RTI  = 0x8,    // todo
    LC3_OPCODE_ST   = 0x3,    // store
    LC3_OPCODE_STI  = 0xB,    // store indirect
    LC3_OPCODE_STR  = 0x7,    // store register
    LC3_OPCODE_TRAP = 0xF,    // execute trap
    LC3_OPCODE_RES  = 0xD,    // reserved (unused)
} lc3_Opcode;

typedef enum lc3_Cond {
    LC3_COND_POS = 0x1, // positive
    LC3_COND_ZRO = 0x2, // zero
    LC3_COND_NEG = 0x4, // negative
} lc3_Cond;

typedef enum lc3_Trap
{
    LC3_TRAP_GETC = 0x20,  // get character from keyboard, not echoed onto the terminal 
    LC3_TRAP_OUT = 0x21,   // output a character 
    LC3_TRAP_PUTS = 0x22,  // output a word string 
    LC3_TRAP_IN = 0x23,    // get character from keyboard, echoed onto the terminal 
    LC3_TRAP_PUTSP = 0x24, // output a byte string 
    LC3_TRAP_HALT = 0x25   // halt the program 
} lc3_Trap;

typedef enum lc3_MemoryRegister
{
    LC3_MEMREG_KBSR = 0xFE00, // keyboard status 
    LC3_MEMREG_KBDR = 0xFE02  // keyboard data 
} lc3_MemoryRegister;

typedef struct lc3_State {
    uint16_t memory[LC3_MAX_MEMORY];
    uint16_t reg[LC3_REG_COUNT];

    bool is_running;
} lc3_State;

static lc3_State s_state = { 0 };

bool lc3_LoadBinary(const char* path);
bool lc3_CheckKey();
void lc3_Interrupt(int signal);
void lc3_DisableBuffering();
void lc3_EnableBuffering();

uint16_t lc3_MemRead(uint16_t reg);
void lc3_MemSet(uint16_t reg, uint16_t value);

uint16_t lc3_SignExtend(uint16_t x, int bit_count);
void lc3_UpdateCond(uint16_t reg);

void lc3_OpADD(uint16_t instr);
void lc3_OpAND(uint16_t instr);
void lc3_OpBR(uint16_t instr);
void lc3_OpJMP(uint16_t instr);
void lc3_OpJSR(uint16_t instr);
void lc3_OpLD(uint16_t instr);
void lc3_OpLDI(uint16_t instr);
void lc3_OpLDR(uint16_t instr);
void lc3_OpLEA(uint16_t instr);
void lc3_OpNOT(uint16_t instr);
void lc3_OpRTI(uint16_t instr);
void lc3_OpST(uint16_t instr);
void lc3_OpSTI(uint16_t instr);
void lc3_OpSTR(uint16_t instr);
void lc3_OpTRAP(uint16_t instr);
void lc3_OpRES(uint16_t instr);

int main(int argc, char* argv[]) 
{
    if (argc < 2 || argc > 2) {
        lc3_Log("usage: lc3 [binary-file1] \n");
        return 1;
    }

    if (!lc3_LoadBinary(argv[1])) {
        lc3_Log("lc3: [error] failed to load binary: %s \n", argv[1]);
        return 2;
    }
    lc3_Log("lc3: [info] loaded \"%s\" \n", argv[1]);

    signal(SIGINT, lc3_Interrupt);
    lc3_DisableBuffering();

    s_state.reg[LC3_REG_COND] = LC3_COND_ZRO;
    s_state.reg[LC3_REG_PC] = 0x3000;
    s_state.is_running = true;


    while (s_state.is_running) {
        uint16_t curr_instr = lc3_MemRead(s_state.reg[LC3_REG_PC]);
        uint16_t op = LC3_GET_BITS(curr_instr, 12, 4);
        s_state.reg[LC3_REG_PC]++;

        switch (op) {
            case LC3_OPCODE_ADD:  lc3_OpADD(curr_instr);  break;
            case LC3_OPCODE_AND:  lc3_OpAND(curr_instr);  break;
            case LC3_OPCODE_NOT:  lc3_OpNOT(curr_instr);  break;
            case LC3_OPCODE_BR:   lc3_OpBR(curr_instr);   break;
            case LC3_OPCODE_JMP:  lc3_OpJMP(curr_instr);  break;
            case LC3_OPCODE_JSR:  lc3_OpJSR(curr_instr);  break;
            case LC3_OPCODE_LD:   lc3_OpLD(curr_instr);   break;
            case LC3_OPCODE_LDI:  lc3_OpLDI(curr_instr);  break;
            case LC3_OPCODE_LDR:  lc3_OpLDR(curr_instr);  break;
            case LC3_OPCODE_LEA:  lc3_OpLEA(curr_instr);  break;
            case LC3_OPCODE_ST:   lc3_OpST(curr_instr);   break;
            case LC3_OPCODE_STI:  lc3_OpSTI(curr_instr);  break;
            case LC3_OPCODE_STR:  lc3_OpSTR(curr_instr);  break;
            case LC3_OPCODE_TRAP: lc3_OpTRAP(curr_instr); break;
            case LC3_OPCODE_RES:  lc3_OpRES(curr_instr);  break;
            case LC3_OPCODE_RTI:  lc3_OpRTI(curr_instr);  break;
            default:              lc3_Log("lc3: [warn] unknown opcode!"); break;
        }
    }

    lc3_EnableBuffering();
    return 0;
}

void lc3_OpADD(uint16_t instr)
{
    uint16_t dr = LC3_GET_BITS(instr, 9, 3);
    uint16_t sr1 = LC3_GET_BITS(instr, 6, 3);
    uint16_t imm_flag = LC3_GET_BITS(instr, 5, 1);

    if (imm_flag) {
        uint16_t imm5 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 5), 5);
        s_state.reg[dr] = s_state.reg[sr1] + imm5;
    } else {
        uint16_t sr2 = LC3_GET_BITS(instr, 0, 3);
        s_state.reg[dr] = s_state.reg[sr1] + s_state.reg[sr2];
    }   

    lc3_UpdateCond(dr);
}

void lc3_OpAND(uint16_t instr)
{
    uint16_t dr = LC3_GET_BITS(instr, 9, 3);
    uint16_t sr1 = LC3_GET_BITS(instr, 6, 3); 
    uint16_t imm_flag = LC3_GET_BITS(instr, 5, 1); 

    if (imm_flag) {
        uint16_t imm5 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 5), 5);
        s_state.reg[dr] = s_state.reg[sr1] & imm5;
    } else {
        uint16_t sr2 = LC3_GET_BITS(instr, 0, 3);
        s_state.reg[dr] = s_state.reg[sr1] & s_state.reg[sr2];
    }   

    lc3_UpdateCond(dr);
}

void lc3_OpBR(uint16_t instr)
{
    uint16_t cond = LC3_GET_BITS(instr, 9, 3);

    if (cond == 0 || cond & s_state.reg[LC3_REG_COND]) {
        uint16_t pc_offset9 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 9), 9);    
        s_state.reg[LC3_REG_PC] += pc_offset9;
    }
}

void lc3_OpJMP(uint16_t instr)
{
    uint16_t sr1 = LC3_GET_BITS(instr, 6, 3); 

    // also handles RET, since it's just a convenience instruction for JMP R7
    s_state.reg[LC3_REG_PC] = s_state.reg[sr1]; 
}

void lc3_OpJSR(uint16_t instr)
{
    uint16_t bit11 = LC3_GET_BITS(instr, 11, 1);
    
    s_state.reg[LC3_REG_R7] = s_state.reg[LC3_REG_PC];
    if (bit11) {
        uint16_t pc_offset11 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 11), 11);
        s_state.reg[LC3_REG_PC] += pc_offset11;
    } else {
        uint16_t sr1 = LC3_GET_BITS(instr, 6, 3);
        s_state.reg[LC3_REG_PC] = s_state.reg[sr1];
    }
}

void lc3_OpLD(uint16_t instr)
{
    uint16_t dr = LC3_GET_BITS(instr, 9, 3);
    uint16_t pc_offset9 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 9), 9);

    s_state.reg[dr] = lc3_MemRead(s_state.reg[LC3_REG_PC] + pc_offset9);

    lc3_UpdateCond(dr);
}

void lc3_OpLDI(uint16_t instr)
{
    uint16_t dr = LC3_GET_BITS(instr, 9, 3);
    uint16_t pc_offset9 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 9), 9);

    s_state.reg[dr] = lc3_MemRead(lc3_MemRead(s_state.reg[LC3_REG_PC] + pc_offset9));

    lc3_UpdateCond(dr);
}

void lc3_OpLDR(uint16_t instr)
{
    uint16_t dr = LC3_GET_BITS(instr, 9, 3);
    uint16_t sr1 = LC3_GET_BITS(instr, 6, 3);
    uint16_t offset6 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 6), 6);

    s_state.reg[dr] = lc3_MemRead(s_state.reg[sr1] + offset6);

    lc3_UpdateCond(dr);
}

void lc3_OpLEA(uint16_t instr)
{
    uint16_t dr = LC3_GET_BITS(instr, 9, 3);
    uint16_t pc_offset9 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 9), 9);

    s_state.reg[dr] = s_state.reg[LC3_REG_PC] + pc_offset9;

    lc3_UpdateCond(dr);
}

void lc3_OpNOT(uint16_t instr)
{
    uint16_t dr = LC3_GET_BITS(instr, 9, 3);
    uint16_t sr1 = LC3_GET_BITS(instr, 6, 3);

    s_state.reg[dr] = ~s_state.reg[sr1];    
    
    lc3_UpdateCond(dr);
}

void lc3_OpRTI(uint16_t instr)
{
    (void)instr;
    lc3_Log("lc3: [warn] LC3_OPCODE_RTI not implemented! instruction skipped \n");
}

void lc3_OpST(uint16_t instr)
{
    uint16_t sr = LC3_GET_BITS(instr, 9, 3);
    uint16_t pc_offset9 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 9), 9);

    lc3_MemSet(s_state.reg[LC3_REG_PC] + pc_offset9, s_state.reg[sr]);
}

void lc3_OpSTI(uint16_t instr)
{
    uint16_t sr = LC3_GET_BITS(instr, 9, 3);
    uint16_t pc_offset9 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 9), 9);

    lc3_MemSet(lc3_MemRead(s_state.reg[LC3_REG_PC] + pc_offset9), s_state.reg[sr]);
}

void lc3_OpSTR(uint16_t instr)
{
    uint16_t sr = LC3_GET_BITS(instr, 9, 3);
    uint16_t sr1 = LC3_GET_BITS(instr, 6, 3);
    uint16_t offset6 = lc3_SignExtend(LC3_GET_BITS(instr, 0, 6), 6);

    lc3_MemSet(s_state.reg[sr1] + offset6, s_state.reg[sr]);
}

void lc3_OpTRAP(uint16_t instr)
{
    uint16_t trapvect8 = LC3_GET_BITS(instr, 0, 8);

    s_state.reg[LC3_REG_R7] = s_state.reg[LC3_REG_PC];  
    switch (trapvect8) {
        case LC3_TRAP_GETC: {
            s_state.reg[LC3_REG_R0] = (uint16_t)getchar();
            
            lc3_UpdateCond(LC3_REG_R0);
        } break;
        case LC3_TRAP_OUT: {
            // could be done with just a printf("%c") i think
            putc((char)s_state.reg[LC3_REG_R0], stdout); 

            fflush(stdout);
        } break;
        case LC3_TRAP_PUTS: {
            uint16_t* c = s_state.memory + s_state.reg[LC3_REG_R0];
            while (*c != 0) {
                putc((char)*c, stdout);
                c++;
            }

            fflush(stdout);
        } break;
        case LC3_TRAP_IN: {
            char c = getchar();
            s_state.reg[LC3_REG_R0] = (uint16_t)c;
            putc(c, stdout);
            fflush(stdout);
            
            lc3_UpdateCond(LC3_REG_R0);
        } break;
        case LC3_TRAP_PUTSP: {
            uint16_t* c = s_state.memory + s_state.reg[LC3_REG_R0];
            while (*c != 0) {
                char c1 = LC3_GET_BITS(*c, 0, 8);
                putc(c1, stdout);
                char c2 = LC3_GET_BITS(*c, 8, 8);
                if (c2 != 0) {
                    putc(c2, stdout);
                }

                c++;
            }
            fflush(stdout);
        } break;
        case LC3_TRAP_HALT: {
            s_state.is_running = 0;
        } break;
        default: lc3_Log("lc3: [warn] unknown trapcode!"); break;
    }
}

void lc3_OpRES(uint16_t instr)
{
    (void)instr;
    lc3_Log("lc3: [warn] illegal opcode! instruction skipped \n");
}

uint16_t lc3_SignExtend(uint16_t x, int bit_count)
{
    if (LC3_GET_BITS(x, bit_count - 1, 1)) {
        x |= (0xFFFF << bit_count);
    }

    return x;
}

void lc3_UpdateCond(uint16_t reg)
{
    if (s_state.reg[reg] == 0) {
        s_state.reg[LC3_REG_COND] = LC3_COND_ZRO;
    } else if (LC3_GET_BITS(s_state.reg[reg], 15, 1)) {
        s_state.reg[LC3_REG_COND] = LC3_COND_NEG;
    } else {
        s_state.reg[LC3_REG_COND] = LC3_COND_POS;
    }
}

uint16_t lc3_SwapToLE16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

bool lc3_LoadBinary(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) { 
        return false; 
    }

    // the origin tells us where in memory to place the image
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = lc3_SwapToLE16(origin);
    
    uint16_t* ptr = s_state.memory + origin;
    size_t read = fread(ptr, sizeof(uint16_t), LC3_MAX_MEMORY - origin, file);
    
    // swap to little endian
    while (read > 0) {
        *ptr = lc3_SwapToLE16(*ptr);
        ptr++;
        read--;
    }

    fclose(file);
    return true;
}

uint16_t lc3_MemRead(uint16_t address)
{
    if (address == LC3_MEMREG_KBSR) {
        if (lc3_CheckKey()) {
            s_state.memory[LC3_MEMREG_KBSR] = 0x8000;
            s_state.memory[LC3_MEMREG_KBDR] = getchar();
        } else {
            s_state.memory[LC3_MEMREG_KBSR] = 0;
        }
    }

    return s_state.memory[address];
}

void lc3_MemSet(uint16_t address, uint16_t value)
{
    s_state.memory[address] = value;
}

#if _WIN32
    /* windows only */
    #include <Windows.h>
    #include <conio.h>  // _kbhit

    HANDLE hStdin = INVALID_HANDLE_VALUE;
    DWORD fdwMode, fdwOldMode;
#else
    /* unix only */
    #include <stdlib.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/termios.h>
    #include <sys/mman.h>

    struct termios original_tio;
#endif

void lc3_DisableBuffering()
{
#if _WIN32
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwOldMode);
    fdwMode = fdwOldMode ^ ENABLE_ECHO_INPUT ^ ENABLE_LINE_INPUT;
    SetConsoleMode(hStdin, fdwMode); 
    FlushConsoleInputBuffer(hStdin); 
#else
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
#endif
}

void lc3_EnableBuffering()
{
#if _WIN32
    SetConsoleMode(hStdin, fdwOldMode);
#else
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
#endif
}

bool lc3_CheckKey()
{
#if _WIN32
    return _kbhit() != 0;
#else
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
#endif
}

void lc3_Interrupt(int signal)
{
    (void)signal;
    lc3_EnableBuffering();
    printf("\n");
    exit(-2);
}

