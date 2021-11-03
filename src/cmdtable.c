#include "setup.h"
#include "cmdtable.h"
#include "handler.h"
#include "exec.h"

const struct cmdtable_elem cmdtable[PROC_CMDCOUNT] = 
{
    {"push", CMD_PUSH, push_handler, push_exec},
    {"pop" , CMD_POP , pop_handler , pop_exec },
    {"add" , CMD_ADD , add_handler , add_exec },
    {"sub" , CMD_SUB , sub_handler , sub_exec },
    {"mul" , CMD_MUL , mul_handler , mul_exec },
    {"div" , CMD_DIV , div_handler , div_exec },
    {"ret" , CMD_RET , ret_handler , ret_exec },
    {"hlt" , CMD_HLT , hlt_handler , hlt_exec },
    {"call", CMD_CALL, call_handler, call_exec},
    {"jmp" , CMD_JMP , jmp_handler , jmp_exec },
    {"je"  , CMD_JE  , je_handler  , je_exec  },
    {"jl"  , CMD_JL  , jl_handler  , jl_exec  },
    {"jle" , CMD_JLE , jle_handler , jle_exec },
    {"cmp" , CMD_CMP , cmp_handler , cmp_exec },
    {"in"  , CMD_IN  , in_handler  , in_exec  },
    {"out" , CMD_OUT , out_handler , out_exec },
};
