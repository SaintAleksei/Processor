#ifndef CMD_H_INCLUDED
#define CMD_H_INCLUDED

#include <stdint.h>

#define PROC_GEN_CODE\
    PROC_GEN_CMD(push, CMD_PUSH, pushtype)\
    PROC_GEN_CMD(pop , CMD_POP , poptype)\
    PROC_GEN_CMD(add , CMD_ADD , pushtype)\
    PROC_GEN_CMD(sub , CMD_SUB , pushtype)\
    PROC_GEN_CMD(mul , CMD_MUL , pushtype)\
    PROC_GEN_CMD(div , CMD_DIV , pushtype)\
    PROC_GEN_CMD(mod , CMD_MOD , pushtype)\
    PROC_GEN_CMD(cmp , CMD_CMP , pushtype)\
    PROC_GEN_CMD(ret , CMD_RET , stdtype)\
    PROC_GEN_CMD(hlt , CMD_HLT , stdtype)\
    PROC_GEN_CMD(jmp , CMD_JMP , jmptype)\
    PROC_GEN_CMD(call, CMD_CALL, calltype)\
    PROC_GEN_CMD(je  , CMD_JE  , jmptype)\
    PROC_GEN_CMD(jl  , CMD_JL  , jmptype)\
    PROC_GEN_CMD(jle , CMD_JLE , jmptype)\
    PROC_GEN_CMD(in  , CMD_IN  , stdtype)\
    PROC_GEN_CMD(out , CMD_OUT , stdtype)\

#define PROC_GEN_CMD(cmd, CODE, TYPE)\
    CODE,

enum PROC_CMDCODES
{
    PROC_GEN_CODE
    CMD_UNKN = 0xFC,
};

#undef PROC_GEN_CMD

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
