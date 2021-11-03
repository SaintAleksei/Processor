#ifndef CMD_H_INCLUDED
#define CMD_H_INCLUDED

#include <stdint.h>

enum CMD_CODES
{
    CMD_HLT,
    CMD_ADD,
    CMD_SUB,
    CMD_MUL,
    CMD_DIV,
    CMD_CMP,
    CMD_RET,
    CMD_JMP,
    CMD_CALL,
    CMD_JE,
    CMD_JL,
    CMD_JLE,
    CMD_JMT,
    CMD_JFL,
    CMD_PUSH,
    CMD_POP,
    CMD_IN,
    CMD_OUT,
    CMD_UNKN = 0xFC,
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
