#ifndef CMD_H_INCLUDED
#define CMD_H_INCLUDED

#include <stdint.h>

#define PROC_GENCODE\
    PROC_GENCMD(push, CMD_PUSH)\
    PROC_GENCMD(pop , CMD_POP )\
    PROC_GENCMD(add , CMD_ADD )\
    PROC_GENCMD(sub , CMD_SUB )\
    PROC_GENCMD(mul , CMD_MUL )\
    PROC_GENCMD(div , CMD_DIV )\
    PROC_GENCMD(ret , CMD_RET )\
    PROC_GENCMD(hlt , CMD_HLT )\
    PROC_GENCMD(jmp , CMD_JMP )\
    PROC_GENCMD(call, CMD_CALL)\
    PROC_GENCMD(je  , CMD_JE  )\
    PROC_GENCMD(jl  , CMD_JL  )\
    PROC_GENCMD(jle , CMD_JLE )\
    PROC_GENCMD(jmt , CMD_JMT )\
    PROC_GENCMD(jfl , CMD_JFL )\
    PROC_GENCMD(in  , CMD_IN  )\
    PROC_GENCMD(out , CMD_OUT )\

#define PROC_GENCMD(cmd, CODE)\
    CODE,

enum PROC_CMDCODES
{
    PROC_GENCODE
    CMD_UNKN = 0xFC,
};

#undef PROC_GENCMD

enum PROC_CMDFLAGS
{
    CMD_FLGREG = 0x80,
    CMD_FLGMEM = 0x40,
};

enum PROC_CONSTS
{
    PROC_CMDCOUNT = 0x100,
    PROC_REGCOUNT = 0x100,
    PROC_STKSIZE  = 0x10000,
    PROC_MEMSIZE  = 0x10000,
};

union val
{
    int8_t   v8;
    uint8_t  vu8;
    int16_t  v16;
    uint16_t vu16;
    int32_t  v32;
    uint64_t vu32;
    int64_t  v64;
    uint64_t vu64;
    double   vdb;
    char     str[8];
};

#endif
