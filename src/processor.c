#include "setup.h"
#include "processor.h"
#include "cmdtable.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static void proc_seterr (proc_t *proc, enum PROC_ERR err, const char *str);

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

        proc->memory = calloc (PROC_MEMSIZE, sizeof (*proc->memory) );
        if (!proc->memory)
            break;

        if (fread (proc->code.data, 1, proc->code.size, stream) != proc->code.size) 
        {
            errstr = "Can't read data from file";
            break;
        }
        if (fclose (stream) == EOF)
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

    memset (proc, 0, sizeof (*proc) );

    proc_seterr (proc, PROC_ERRCREATE, errstr);

    return EXIT_FAILURE;
}

void proc_delete (proc_t *proc)
{
    assert (proc);

    free (proc->code.data);
    free (proc->memory);

    memset (proc, 0, sizeof (*proc) );
}

void proc_error (proc_t *proc)
{
    assert (proc);

    switch (proc->error.err)
    {
        case PROC_NOERR:
            fprintf (stderr, "No error");
            break;
        case PROC_ERRCOMMAND:
            fprintf (stderr, "Unknown command");
            break;
        case PROC_ERRIP:
            fprintf (stderr, "Bad ip");
            break;
        case PROC_ERRSPINT:
            fprintf (stderr, "Bad stkint pointer");
            break;
        case PROC_ERRSPRET:
            fprintf (stderr, "Bad stkret pointer");
            break;
        case PROC_ERRADD:
            fprintf (stderr, "Add error");
            break;
        case PROC_ERRSUB:
            fprintf (stderr, "Sub error");
            break;
        case PROC_ERRMUL:
            fprintf (stderr, "Mul error");
            break;
        case PROC_ERRDIV:
            fprintf (stderr, "Div error");
            break;
        case PROC_ERRCMP:
            fprintf (stderr, "Cmp error");
            break;
        case PROC_ERRCREATE:
            fprintf (stderr, "Can't create processor");
            break;
        default:
            fprintf (stderr, "Undefined error");
    }

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

static int  cmd_read  (proc_t *proc);
static int  cmd_exec  (proc_t *proc);
static void cmd_log   (proc_t *proc);

int proc_run (proc_t *proc)
{
    assert (proc); 
    
    proc->status = PROC_STRUN;

    while (proc->status == PROC_STRUN)
    {
        if (cmd_read (proc) )
            break;

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

    return EXIT_SUCCESS;
}

static int cmd_exec (proc_t *proc)
{
    assert (proc);

    uint8_t stop = 0;
    for (size_t i = 0; !stop && i < PROC_CMDCOUNT && cmdtable[i].exec; i++)
        if (proc->cmd.code == cmdtable[i].code)
        {
            stop = 1;
            if (cmdtable[i].exec (proc) )
                return EXIT_FAILURE;
        }

    if (!stop)
    {
        proc_seterr (proc, PROC_ERRCOMMAND, NULL);

        fprintf (proc->log, "UNKNOWN COMMAND\n");

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int pop_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);
    assert (proc->memory);
    assert (proc->stack.spint < PROC_STKSIZE);

    if (proc->stack.spint == 0)
    {
        proc_seterr (proc, PROC_ERRSPINT, "Can't pop value from empty stack");

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

int push_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);
    assert (proc->memory);

    if (proc->stack.spint >= PROC_STKSIZE)
    {
        proc_seterr (proc, PROC_ERRSPINT, "Can't push value in full stack");

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

int add_exec (void *arg)
{
    proc_t *proc = arg;

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

int sub_exec (void *arg)
{
    proc_t *proc = arg;

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

int mul_exec (void *arg)
{
    proc_t *proc = arg;

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

int div_exec (void *arg)
{
    proc_t *proc = arg;

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

int cmp_exec (void *arg)
{
    proc_t *proc = arg;

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

int ret_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);
    assert (proc->stack.spret < PROC_STKSIZE);

    if (proc->stack.spret == 0)
    {
        proc_seterr (proc, PROC_ERRSPRET, "Can't pop value from empty stack");

        return EXIT_FAILURE;
    }

    proc->stack.spret--; 
    proc->code.ip = proc->stack.stkret[proc->stack.spret];

    return EXIT_SUCCESS;
}

int hlt_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);

    proc->status = PROC_STHLT;

    return EXIT_SUCCESS;
}

int call_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);

    if (proc->stack.spret >= PROC_STKSIZE)
    {
        proc_seterr (proc, PROC_ERRSPRET, "Can't push value in full stack");

        return EXIT_FAILURE;
    }

    proc->stack.stkret[proc->stack.spret++] = proc->code.ip + 9;
    proc->code.ip = proc->cmd.arg.vu64;

    return EXIT_SUCCESS;
}

int jmp_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);

    proc->code.ip = proc->cmd.arg.vu64;

    return EXIT_SUCCESS;
}

int je_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);

    if (proc->cmp == PROC_CMPEQ)
        proc->code.ip = proc->cmd.arg.vu64;
    else
        proc->code.ip += 9;

    return EXIT_SUCCESS;
}

int jl_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);

    if (proc->cmp == PROC_CMPLESS)
        proc->code.ip = proc->cmd.arg.vu64;
    else
        proc->code.ip += 9;

    return EXIT_SUCCESS;
}

int jle_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);

    if (proc->cmp == PROC_CMPLESS || proc->cmp == PROC_CMPEQ)
        proc->code.ip = proc->cmd.arg.vu64;
    else
        proc->code.ip += 9;

    return EXIT_SUCCESS;
}

int out_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);

    printf ("%ld\n", proc->regs[0].v64);

    proc->code.ip += 1;
    
    return EXIT_SUCCESS;
}

int in_exec (void *arg)
{
    proc_t *proc = arg;

    assert (proc);

    scanf ("%ld", &proc->regs[0].v64);

    proc->code.ip += 1;
    
    return EXIT_SUCCESS;
}
