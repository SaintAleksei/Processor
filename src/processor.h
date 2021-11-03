#ifndef PROCESSOR_H_INCLUDED
#define PROCESSOR_H_INCLUDED

#include "setup.h"

enum PROC_ERR
{
    PROC_NOERR, 
    PROC_ERRCOMMAND,
    PROC_ERRCREATE,
    PROC_ERRIP, 
    PROC_ERRSPINT,
    PROC_ERRSPDB,
    PROC_ERRSPRET,
    PROC_ERRADD,
    PROC_ERRSUB,
    PROC_ERRMUL,
    PROC_ERRDIV,
    PROC_ERRCMP,
};

enum PROC_STAT
{
    PROC_STHLT,
    PROC_STRUN,
    PROC_STERR,
};

enum PROC_CMPVAL
{
    PROC_CMPEQ,
    PROC_CMPGREAT,
    PROC_CMPLESS,
};

struct proc_code
{
    void     *data;
    uint64_t  size;
    uint64_t  ip;
};

struct proc_stack
{
    int64_t   stkint[PROC_STKSIZE];
    uint64_t  stkret[PROC_STKSIZE];
    uint64_t  spint;
    uint64_t  spret;
};

struct proc_cmd
{
    uint8_t   code;
    uint8_t   flgreg;
    uint8_t   flgmem;
    union val arg;
};

struct proc_error
{
    enum PROC_ERR err;
    const char   *str;
};

typedef struct processor
{
    struct proc_code     code;
    struct proc_stack    stack;        
    union  val          *memory;
    struct proc_cmd      cmd;
    union  val           regs[PROC_REGCOUNT];
    enum   PROC_CMPVAL   cmp;
    enum   PROC_STAT     status;
    struct proc_error    error;
    FILE                *log;
} proc_t;

int  proc_create (proc_t *proc, const char *filename);
int  proc_run    (proc_t *proc);
void proc_delete (proc_t *proc);
void proc_error  (proc_t *proc);

#endif
