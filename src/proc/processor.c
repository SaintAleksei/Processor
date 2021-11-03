#include "setup.h"
#include "processor.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static void        proc_seterr   (proc_t *proc, enum PROC_ERR err, const char *str);
static const char *proc_strerror (enum PROC_ERR err);

static int  cmd_read  (proc_t *proc);
static int  cmd_exec  (proc_t *proc);
static void cmd_log   (proc_t *proc);

static int push_exec (proc_t *proc);
static int pop_exec  (proc_t *proc);
static int add_exec  (proc_t *proc); 
static int sub_exec  (proc_t *proc); 
static int mul_exec  (proc_t *proc); 
static int cmp_exec  (proc_t *proc);
static int div_exec  (proc_t *proc); 
static int ret_exec  (proc_t *proc); 
static int hlt_exec  (proc_t *proc); 
static int call_exec (proc_t *proc); 
static int jmp_exec  (proc_t *proc); 
static int je_exec   (proc_t *proc); 
static int jl_exec   (proc_t *proc); 
static int jle_exec  (proc_t *proc); 
static int jmt_exec  (proc_t *proc);
static int jfl_exec  (proc_t *proc);
static int in_exec   (proc_t *proc);
static int out_exec  (proc_t *proc);
static int unkn_exec (proc_t *proc);

static void push_log (proc_t *proc);
static void pop_log  (proc_t *proc);
static void add_log  (proc_t *proc);
static void sub_log  (proc_t *proc);
static void mul_log  (proc_t *proc);
static void div_log  (proc_t *proc);
static void cmp_log  (proc_t *proc);
static void hlt_log  (proc_t *proc);
static void ret_log  (proc_t *proc);
static void in_log   (proc_t *proc);
static void out_log  (proc_t *proc);
static void call_log (proc_t *proc);
static void jmp_log  (proc_t *proc);
static void je_log   (proc_t *proc);
static void jl_log   (proc_t *proc);
static void jle_log  (proc_t *proc);
static void jmt_log  (proc_t *proc);
static void jfl_log  (proc_t *proc);
static void unkn_log (proc_t *proc);

static const struct proc_cmdtable_elem
{
    uint8_t code;
    int   (*exec) (proc_t *proc);
    void  (*log)  (proc_t *proc);
} cmdtable[PROC_CMDCOUNT] =
{
    {CMD_PUSH, push_exec, push_log},
    {CMD_POP , pop_exec , pop_log },
    {CMD_ADD , add_exec , add_log },
    {CMD_SUB , sub_exec , sub_log },
    {CMD_MUL , mul_exec , mul_log },
    {CMD_DIV , div_exec , div_log },
    {CMD_CMP , cmp_exec , cmp_log },
    {CMD_HLT , hlt_exec , hlt_log },
    {CMD_RET , ret_exec , ret_log },
    {CMD_IN  , in_exec  , in_log  },
    {CMD_OUT , out_exec , out_log },
    {CMD_CALL, call_exec, call_log},
    {CMD_JMP , jmp_exec , jmp_log },
    {CMD_JE  , je_exec  , je_log  },
    {CMD_JL  , jl_exec  , jl_log  },
    {CMD_JLE , jle_exec , jle_log },
    {CMD_JMT , jmt_exec , jmt_log },
    {CMD_JFL , jfl_exec , jfl_log },
    {CMD_UNKN, unkn_exec, unkn_log},
};

int proc_create (proc_t *proc, const char *filename)
{
    assert (proc);
    assert (filename);

    FILE *stream = NULL;
    char *errstr = NULL;

    memset (proc, 0, sizeof (*proc) );

    do
    {
        errno = 0;

        stream = fopen (filename, "r");

        if (!stream)
            break;
        if (fseek (stream, 0, SEEK_END) == -1)
            break;
        if ( (proc->code.size = ftell (stream) ) == (uint64_t) -1)
            break;
        if (fseek (stream, 0, SEEK_SET) == -1)
            break;

        proc->code.data = calloc (1, proc->code.size + 0x10);
        if (!proc->code.data)
            break;

        if (fread (proc->code.data, 1, proc->code.size, stream) != proc->code.size) 
        {
            errstr = "Can't read data from file";
            break;
        }
        if (fclose (stream) == EOF)
            break;

        proc->memory = calloc (PROC_MEMSIZE + 1, sizeof (*proc->memory) );
        if (!proc->memory)
            break;

        proc->stack.stkint = calloc (PROC_STKSIZE + 1, sizeof (*proc->stack.stkint) );
        if (!proc->stack.stkint)
            break;

        proc->stack.stkret = calloc (PROC_STKSIZE + 1, sizeof (*proc->stack.stkret) );
        if (!proc->stack.stkret)
            break;

        proc->log = fopen ("proc.log", "w");

        if (!proc->log)
            break;

        return EXIT_SUCCESS;
    }
    while (0);
    
    if (!errstr)
        errstr = strerror (errno);

    if (stream)
        fclose (stream);
    free (proc->code.data);
    free (proc->memory);
    free (proc->stack.stkint);
    free (proc->stack.stkret);

    memset (proc, 0, sizeof (*proc) );

    proc_seterr (proc, PROC_ERRCREATE, errstr);

    return EXIT_FAILURE;
}

void proc_delete (proc_t *proc)
{
    assert (proc);

    if (proc->log)
        fclose (proc->log);
    free (proc->code.data);
    free (proc->memory);
    free (proc->stack.stkint);
    free (proc->stack.stkret);

    memset (proc, 0, sizeof (*proc) );
}

static const char *proc_strerror (enum PROC_ERR err)
{
    switch (err)
    {
        case PROC_NOERR:
            return "No error";
        case PROC_ERRCREATE:
            return "Creation error";
        case PROC_ERRIP:
            return "Bad ip";
        case PROC_ERRUNKN:
            return "Unknown command";
        case PROC_ERRPUSH:
            return "Can't execute push: stack is full";
        case PROC_ERRPOP:
            return "Can't execute pop: stack is empty";
        case PROC_ERRADD:
            return "Can't execute add: not enough values in stack";
        case PROC_ERRSUB:
            return "Can't execute sub: not enough values in stack";
        case PROC_ERRMUL:
            return "Can't execute mul: not enough values in stack";
        case PROC_ERRDIV:
            return "Can't execute div: not enough values in stack";
        case PROC_ERRCMP:
            return "Can't execute cmp: not enough values in stack";
        case PROC_ERRCALL:
            return "Can't execute call: stack is full";
        case PROC_ERRRET:
            return "Can't execute ret: stack is empty";
    }

    return "Undefined error"; 
}

void proc_error (proc_t *proc)
{
    assert (proc);

    fprintf (stderr, "%s", proc_strerror (proc->error.err) );

    if (proc->error.str)
        fprintf (stderr, ": %s", proc->error.str);

    fprintf (stderr, "\n");
}

static void proc_seterr (proc_t *proc, enum PROC_ERR err, const char *str)
{
    assert (proc);
    
    proc->error.err = err;
    proc->error.str = str;
    proc->status    = PROC_STERR;
}


int proc_run (proc_t *proc)
{
    assert (proc); 
    
    proc->status = PROC_STRUN;

    while (proc->status == PROC_STRUN)
    {
        if (cmd_read (proc) )
            break;

        cmd_log (proc);

        if (cmd_exec (proc) )
            break;
    }

    return (proc->status == PROC_STHLT) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int cmd_read (proc_t *proc)
{
    assert (proc);
    assert (proc->code.data);

    if (proc->code.ip >= proc->code.size)
    {
        proc_seterr (proc, PROC_ERRIP, NULL);
        return EXIT_FAILURE;
    }

    proc->cmd.code   = *( (uint8_t *) (proc->code.data + proc->code.ip) );
    proc->cmd.flgreg = (proc->cmd.code & CMD_FLGREG) ? 1 : 0;
    proc->cmd.flgmem = (proc->cmd.code & CMD_FLGMEM) ? 1 : 0;
    proc->cmd.code  &= ~(CMD_FLGREG | CMD_FLGMEM);
    proc->cmd.arg    = *( (union val *) (proc->code.data + proc->code.ip + 1) );

    for (size_t i = 0; i < PROC_CMDCOUNT && cmdtable[i].exec; i++)
        if (proc->cmd.code == cmdtable[i].code)
        {
            proc->cmd.id = i;

            return EXIT_SUCCESS;
        }
    
    proc->cmd.code   = CMD_UNKN;
    proc->cmd.flgreg = 0;
    proc->cmd.flgmem = 0;

    return EXIT_SUCCESS;
}

static int cmd_exec (proc_t *proc)
{
    assert (proc);
    assert (cmdtable[proc->cmd.id].exec);

    return (proc->cmd.code == CMD_UNKN) ? unkn_exec (proc) : cmdtable[proc->cmd.id].exec (proc);
}

static void cmd_log (proc_t *proc)
{
    assert (proc);
    assert (cmdtable[proc->cmd.id].log);

    if (proc->cmd.code == CMD_UNKN)
        unkn_log (proc);
    else
        cmdtable[proc->cmd.id].log (proc);
}

static int pop_exec (proc_t *proc)
{
    assert (proc);
    assert (proc->memory);
    assert (proc->stack.spint < PROC_STKSIZE);

    if (proc->stack.spint == 0)
    {
        proc_seterr (proc, PROC_ERRPOP, NULL);

        return EXIT_FAILURE;
    }

    proc->stack.spint--;

    if (proc->cmd.flgreg)
    {
        if (proc->cmd.flgmem)
            proc->memory[proc->regs[proc->cmd.arg.vu8].vu64 % PROC_MEMSIZE].v64 = proc->stack.stkint[proc->stack.spint];
        else
            proc->regs[proc->cmd.arg.vu8].v64 = proc->stack.stkint[proc->stack.spint];

        proc->code.ip += 2;
    }
    else
    {
        if (proc->cmd.flgmem)
        {
            proc->memory[proc->cmd.arg.vu64 % PROC_MEMSIZE].v64 = proc->stack.stkint[proc->stack.spint];
            proc->code.ip += 9;
        }
        else
            proc->code.ip += 1;
    }

    return EXIT_SUCCESS;
}

static int push_exec (proc_t *proc)
{
    assert (proc);
    assert (proc->memory);

    if (proc->stack.spint >= PROC_STKSIZE)
    {
        proc_seterr (proc, PROC_ERRPUSH, NULL);

        return EXIT_FAILURE;
    }
    

    if (proc->cmd.flgreg)
    {
        if (proc->cmd.flgmem)
            proc->stack.stkint[proc->stack.spint] = proc->memory[proc->regs[proc->cmd.arg.vu8].vu64 % PROC_MEMSIZE].v64;
        else
            proc->stack.stkint[proc->stack.spint] = proc->regs[proc->cmd.arg.vu8].v64;

        proc->code.ip += 2;
    }
    else
    {
        if (proc->cmd.flgmem)
            proc->stack.stkint[proc->stack.spint] = proc->memory[proc->cmd.arg.vu64 % PROC_MEMSIZE].v64;
        else
            proc->stack.stkint[proc->stack.spint] = proc->cmd.arg.v64;

        proc->code.ip += 9;
    }

    proc->stack.spint++;

    return EXIT_SUCCESS;
}

static int add_exec (proc_t *proc)
{
    assert (proc);
    assert (proc->stack.spint < PROC_STKSIZE);

    if (proc->stack.spint < 2)
    {
        proc_seterr (proc, PROC_ERRADD, NULL);

        return EXIT_FAILURE;
    }

    proc->stack.stkint[proc->stack.spint-2] += proc->stack.stkint[proc->stack.spint-1];
    proc->stack.spint--;

    proc->code.ip++;

    return EXIT_SUCCESS;
}

static int sub_exec (proc_t *proc)
{
    assert (proc);
    assert (proc->stack.spint < PROC_STKSIZE);

    if (proc->stack.spint < 2)
    {
        proc_seterr (proc, PROC_ERRSUB, NULL);


        return EXIT_FAILURE;
    }

    proc->stack.stkint[proc->stack.spint-2] -= proc->stack.stkint[proc->stack.spint-1];
    proc->stack.spint--;

    proc->code.ip++;

    return EXIT_SUCCESS;
}

static int mul_exec (proc_t *proc)
{
    assert (proc);
    assert (proc->stack.spint < PROC_STKSIZE);

    if (proc->stack.spint < 2)
    {
        proc_seterr (proc, PROC_ERRMUL, NULL);

        return EXIT_FAILURE;
    }

    proc->stack.stkint[proc->stack.spint-2] *= proc->stack.stkint[proc->stack.spint-1];
    proc->stack.spint--;

    proc->code.ip++;

    return EXIT_SUCCESS;
}

static int div_exec (proc_t *proc)
{
    assert (proc);
    assert (proc->stack.spint < PROC_STKSIZE);

    if (proc->stack.spint < 2)
    {
        proc_seterr (proc, PROC_ERRDIV, NULL);

        return EXIT_FAILURE;
    }

    int64_t buff = proc->stack.stkint[proc->stack.spint-2] % proc->stack.stkint[proc->stack.spint-1];
    proc->stack.stkint[proc->stack.spint-1] = proc->stack.stkint[proc->stack.spint-2] / proc->stack.stkint[proc->stack.spint-1];
    proc->stack.stkint[proc->stack.spint-2] = buff;

    proc->code.ip++;

    return EXIT_SUCCESS;
}

static int cmp_exec (proc_t *proc)
{
    assert (proc);
    assert (proc->stack.spint < PROC_STKSIZE);

    if (proc->stack.spint < 2)
    {
        proc_seterr (proc, PROC_ERRCMP, NULL);

        return EXIT_FAILURE;
    }

    if (proc->stack.stkint[proc->stack.spint-2] == proc->stack.stkint[proc->stack.spint-1] )
        proc->cmp = PROC_CMPEQ;
    else if (proc->stack.stkint[proc->stack.spint-2] < proc->stack.stkint[proc->stack.spint-1] )
        proc->cmp = PROC_CMPLESS;
    else
        proc->cmp = PROC_CMPGREAT;

    proc->code.ip++;

    return EXIT_SUCCESS;
}

static int ret_exec (proc_t *proc)
{
    assert (proc);
    assert (proc->stack.spret < PROC_STKSIZE);

    if (proc->stack.spret == 0)
    {
        proc_seterr (proc, PROC_ERRRET, NULL);

        return EXIT_FAILURE;
    }

    proc->stack.spret--; 
    proc->code.ip = proc->stack.stkret[proc->stack.spret];

    return EXIT_SUCCESS;
}

static int hlt_exec (proc_t *proc)
{
    assert (proc);

    proc->status = PROC_STHLT;

    return EXIT_SUCCESS;
}

static int call_exec (proc_t *proc)
{
    assert (proc);

    if (proc->stack.spret >= PROC_STKSIZE)
    {
        proc_seterr (proc, PROC_ERRCALL, NULL);

        return EXIT_FAILURE;
    }

    proc->stack.stkret[proc->stack.spret++] = proc->code.ip + 9;
    proc->code.ip = proc->cmd.arg.vu64;

    return EXIT_SUCCESS;
}

static int jmp_exec (proc_t *proc)
{
    assert (proc);

    proc->code.ip = proc->cmd.arg.vu64;

    return EXIT_SUCCESS;
}

static int je_exec (proc_t *proc)
{
    assert (proc);

    if (proc->cmp == PROC_CMPEQ)
        proc->code.ip = proc->cmd.arg.vu64;
    else
        proc->code.ip += 9;

    return EXIT_SUCCESS;
}

static int jl_exec (proc_t *proc)
{
    assert (proc);

    if (proc->cmp == PROC_CMPLESS)
        proc->code.ip = proc->cmd.arg.vu64;
    else
        proc->code.ip += 9;

    return EXIT_SUCCESS;
}

static int jle_exec (proc_t *proc)
{
    assert (proc);

    if (proc->cmp == PROC_CMPLESS || proc->cmp == PROC_CMPEQ)
        proc->code.ip = proc->cmd.arg.vu64;
    else
        proc->code.ip += 9;

    return EXIT_SUCCESS;
}

static int jmt_exec (proc_t *proc)
{
    assert (proc);
    
    if (proc->stack.spret == 0)
        proc->code.ip = proc->cmd.arg.vu64;
    else
        proc->code.ip += 9;

    return EXIT_SUCCESS;
}

static int jfl_exec (proc_t *proc)
{
    assert (proc);
    
    if (proc->stack.spret >= PROC_STKSIZE)
        proc->code.ip = proc->cmd.arg.vu64;
    else
        proc->code.ip += 9;

    return EXIT_SUCCESS;
}

static int out_exec (proc_t *proc)
{
    assert (proc);

    printf ("%ld\n", proc->regs[0].v64);

    proc->code.ip += 1;
    
    return EXIT_SUCCESS;
}

static int in_exec (proc_t *proc)
{
    assert (proc);

    scanf ("%ld", &proc->regs[0].v64);

    proc->code.ip += 1;
    
    return EXIT_SUCCESS;
}


static int unkn_exec (proc_t *proc)
{
    assert (proc);

    proc_seterr (proc, PROC_ERRUNKN, NULL); 

    return EXIT_FAILURE;
}

static void push_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: push", proc->code.ip);

    if (proc->cmd.flgmem)
    {
        if (proc->cmd.flgreg)
            fprintf (proc->log, " [r%hhu] = [0x%016lx];\n", 
                     proc->cmd.arg.vu8, proc->regs[proc->cmd.arg.vu8].vu64 % PROC_MEMSIZE);
        else
            fprintf (proc->log, " [0x%016lx];\n", 
                     proc->cmd.arg.vu64 % PROC_MEMSIZE);
    }
    else
    {
        if (proc->cmd.flgreg)
            fprintf (proc->log, " r%hhu = %ld;\n",
                     proc->cmd.arg.vu8, proc->regs[proc->cmd.arg.vu8].v64);
        else
            fprintf (proc->log, " %ld;\n", proc->cmd.arg.v64);
    }

    fflush (proc->log);
}

static void pop_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: pop", proc->code.ip);

    if (proc->cmd.flgmem)
    {
        if (proc->cmd.flgreg)
            fprintf (proc->log, " [r%hhu] = [0x%016lx];\n", 
                     proc->cmd.arg.vu8, proc->regs[proc->cmd.arg.vu8].vu64 % PROC_MEMSIZE);
        else
            fprintf (proc->log, " [0x%016lx];\n", 
                     proc->cmd.arg.vu64 % PROC_MEMSIZE);
    }
    else
    {
        if (proc->cmd.flgreg)
            fprintf (proc->log, " r%hhu = %ld;\n",
                     proc->cmd.arg.vu8, proc->regs[proc->cmd.arg.vu8].v64);
        else
            fprintf (proc->log, ";\n");
    }

    fflush (proc->log);
}

static void add_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: add;\n", proc->code.ip);
    fflush (proc->log);
}

static void sub_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: sub;\n", proc->code.ip);
    fflush (proc->log);
}

static void mul_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: mul;\n", proc->code.ip);
    fflush (proc->log);
}

static void div_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: div;\n", proc->code.ip);
    fflush (proc->log);
}

static void cmp_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: cmp;\n", proc->code.ip);
    fflush (proc->log);
}

static void ret_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: ret;\n", proc->code.ip);
    fflush (proc->log);
}

static void hlt_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: hlt;\n", proc->code.ip);
    fflush (proc->log);
}

static void in_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: in;\n", proc->code.ip);
    fflush (proc->log);
}

static void out_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: out;\n", proc->code.ip);
    fflush (proc->log);
}

static void jmp_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: jmp 0x%016lx;\n", proc->code.ip, proc->cmd.arg.vu64);
    fflush (proc->log);
}

static void call_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: call 0x%016lx;\n", proc->code.ip, proc->cmd.arg.vu64);
    fflush (proc->log);
}

static void je_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: je 0x%016lx;\n", proc->code.ip, proc->cmd.arg.vu64);
    fflush (proc->log);
}

static void jl_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: jl 0x%016lx;\n", proc->code.ip, proc->cmd.arg.vu64);
    fflush (proc->log);
}

static void jle_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: jle 0x%016lx;\n", proc->code.ip, proc->cmd.arg.vu64);
    fflush (proc->log);
}

static void jmt_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: jmt 0x%016lx;\n", proc->code.ip, proc->cmd.arg.vu64);
    fflush (proc->log);
}

static void jfl_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);

    fprintf (proc->log, "0x%016lx: jfl 0x%016lx;\n", proc->code.ip, proc->cmd.arg.vu64);
    fflush (proc->log);
}

static void unkn_log (proc_t *proc)
{
    assert (proc);
    assert (proc->log);
    
    fprintf (proc->log, "0x%016lx: unknown;\n", proc->code.ip);
    fflush (proc->log);
}
