#ifndef CMD_H_INCLUDED
#define CMD_H_INCLUDED

#include <stdint.h>

enum CMD_CODES
{
    CMD_HLT = 0x0,
    CMD_ADD = 0x1,
    CMD_SUB = 0x2,
    CMD_MUL = 0x3,
    CMD_DIV = 0x4,
    CMD_CMP = 0x5,
    CMD_RET = 0x6,
    CMD_JMP = 0x7, 
    CMD_CALL = 0x8,
    CMD_JE = 0x9,
    CMD_JL = 0x10,
    CMD_JLE = 0x11,
    CMD_PUSH = 0x12,
    CMD_POP = 0x13,
    CMD_IN = 0x14,
    CMD_OUT = 0x15,
};

enum CMD_FLAGS
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
    char     str[8];
    int8_t   v8;
    uint8_t  vu8;
    int16_t  v16;
    uint16_t vu16;
    int32_t  v32;
    uint64_t vu32;
    int64_t  v64;
    uint64_t vu64;
    double   vdb;
};

#endif
