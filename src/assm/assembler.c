#include "setup.h"
#include "assembler.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

static void assm_clearerr   (assm_t *assm);
static void assm_seterr     (assm_t *assm, enum ASSM_ERRORS err, const char *str);
static int  label_handler   (assm_t *assm);
static int  command_handler (assm_t *assm);
static int  argno_handler   (assm_t *assm, uint8_t cmd);
static int  arglb_handler   (assm_t *assm, uint8_t cmd);

static int push_handler (assm_t *assm);
static int pop_handler  (assm_t *assm);
static int add_handler  (assm_t *assm);
static int sub_handler  (assm_t *assm);
static int mul_handler  (assm_t *assm);
static int div_handler  (assm_t *assm);
static int cmp_handler  (assm_t *assm);
static int hlt_handler  (assm_t *assm);
static int ret_handler  (assm_t *assm);
static int in_handler   (assm_t *assm);
static int out_handler  (assm_t *assm);
static int jmp_handler  (assm_t *assm);
static int call_handler (assm_t *assm);
static int je_handler   (assm_t *assm);
static int jl_handler   (assm_t *assm);
static int jle_handler  (assm_t *assm);
static int jmt_handler  (assm_t *assm);
static int jfl_handler  (assm_t *assm);

static const struct assm_cmdtable_elem
{
    const char *name;
    int       (*handler) (assm_t *assm);
} cmdtable[PROC_CMDCOUNT] =
{
    {"push", push_handler},
    {"pop" , pop_handler },
    {"add" , add_handler },
    {"sub" , sub_handler },
    {"mul" , mul_handler },
    {"div" , div_handler },
    {"cmp" , cmp_handler },
    {"hlt" , hlt_handler },
    {"ret" , ret_handler },
    {"in"  , in_handler  },
    {"out" , out_handler },
    {"jmp" , jmp_handler },
    {"call", call_handler},
    {"je"  , je_handler  },
    {"jl"  , jl_handler  },
    {"jle" , jle_handler },
    {"jmt" , jmt_handler },
    {"jfl" , jfl_handler },
};

int assm_create (assm_t *assm, const char *name)
{
    assert (assm);
    assert (name);

    FILE    *stream = NULL;
    char    *errstr = NULL;
    uint8_t  flag   = 0;

    do
    {
        errno  = 0;

        stream = fopen (name, "r");

        if (!stream)
            break;
        
        if (fseek (stream, 0L, SEEK_END) == -1)
            break;

        if ( (assm->text.buffsize = ftell (stream) ) == (size_t) -1)
            break;

        if (fseek (stream, 0L, SEEK_SET) == -1)
            break;
        
        assm->text.buff = calloc (assm->text.buffsize + 1, 1);

        if (!assm->text.buff)
            break;
    
        if (fread (assm->text.buff, 1, assm->text.buffsize, stream) != assm->text.buffsize)
        {
            errstr = "Can't read data from file";
            break;
        }

        if (fclose (stream) == EOF)
            break;

        for (size_t i = 0; i < assm->text.buffsize; i++) 
            if (isspace (assm->text.buff[i]) )
            {
                assm->text.wordsize++; 
                assm->text.buff[i] = '\0';
            }

        assm->text.word = calloc (assm->text.wordsize, sizeof (char **) );

        if (!assm->text.word)
            break;

        assm->text.wordsize = 0;
        flag = 1;
        for (size_t i = 0; i < assm->text.buffsize; i++)
            if (assm->text.buff[i] && flag)
            {
                assm->text.word[assm->text.wordsize++] = assm->text.buff + i;
                flag = 0;
            }
            else if (!assm->text.buff[i] && !flag)
                flag = 1;

        assm->text.word = realloc (assm->text.word, (assm->text.wordsize + 1) * sizeof (char **) );

        if (!assm->text.word)
            break;
    
        assm->text.word[assm->text.wordsize] = assm->text.buff + assm->text.buffsize;

        assm->code.size = 0x20;
        assm->code.data = calloc (1, assm->code.size);
        if (!assm->code.data)
            break;

        assm->labels.capacity = 1;
        assm->labels.data     = calloc (assm->labels.capacity, sizeof (*assm->labels.data));
        if (!assm->labels.data)
            break;

        assm->log = fopen ("assm.log", "w");

        if (!assm->log)
            break;

        strncpy (assm->text.name, name, STRSIZE);
        
        return EXIT_SUCCESS;
    }
    while (0);

    if (!errstr)
        errstr = strerror (errno);

    if (stream)
        fclose (stream);
    if (assm->log)
        fclose (assm->log);
    free (assm->text.buff);
    free (assm->text.word);
    free (assm->code.data);
    free (assm->labels.data);

    memset (assm, 0, sizeof (*assm) );

    assm_seterr (assm, ASSM_ERRCREATE, errstr);

    return EXIT_FAILURE;
}

void assm_delete (assm_t *assm)
{
    assert (assm);

    if (assm->log)
    {
        if (assm->error.err)
            fprintf (assm->log, "***** ERROR *****\n");
        else
            fprintf (assm->log, "***** SUCCESS *****\n");

        fclose (assm->log);
    }
    free (assm->text.buff);
    free (assm->text.word);
    free (assm->code.data);  
    free (assm->labels.data);

    memset (assm, 0, sizeof (*assm) );
}

int assm_write (assm_t *assm)
{
    assert (assm);

    if (assm->error.err)
        return EXIT_FAILURE;

    char  name[STRSIZE] = "";
    char *errptr        = NULL;
    FILE *stream        = NULL;
    char *ptr           = NULL;

    strncpy (name, assm->text.name, STRSIZE);

    ptr = strchr (name, '.');

    if (ptr)
        *ptr = '\0';

    strncat (name, ".proc", STRSIZE - 1);

    do 
    {
        errno  = 0;

        stream = fopen (name, "w");

        if (!stream)
            break;

        if (fwrite (assm->code.data, 1, assm->code.ip, stream) != assm->code.ip)
        {
            errptr = "Can't write data to file";
            break;
        }
        
        if (fclose (stream) == EOF)
            break;

        return EXIT_SUCCESS;
    }
    while (0);

    if (!errptr)
        errptr = strerror (errno);

    if (stream)
        fclose (stream);

    assm_seterr (assm, ASSM_ERRWRITE, errptr);

    return EXIT_FAILURE;
}

void assm_error (assm_t *assm)
{
    assert (assm);

    switch (assm->error.err)
    {
        case ASSM_NOERR:
            fprintf (stderr, "No error");
            break;
        case ASSM_ERRLABEL:
            fprintf (stderr, "Unknown label");
            break;
        case ASSM_ERRREDEF:
            fprintf (stderr, "Redefinition of label");
            break;
        case ASSM_ERRCMD:
            fprintf (stderr, "Bad command"); 
            break;
        case ASSM_ERRPUSHARG:
            fprintf (stderr, "Bad push argument");
            break;
        case ASSM_ERRSYSTEM:
            fprintf (stderr, "Sytem error");
            break;
        case ASSM_ERRCREATE:
            fprintf (stderr, "Creation error");
            break;
        case ASSM_ERRWRITE:
            fprintf (stderr, "Writing error");
            break;
        default:
            fprintf (stderr, "Undefined error");
    }

    if (assm->error.str)
        fprintf (stderr, ": %s", assm->error.str);

    fprintf (stderr, "\n");
}

void assm_seterr (assm_t *assm, enum ASSM_ERRORS err, const char *str)
{
    assert (assm);

    assm->error.err = err;
    assm->error.str = str;
}

void assm_clearerr (assm_t *assm)
{
    assm_seterr (assm, ASSM_NOERR, NULL);
}

int assm_translate (assm_t *assm)
{
    assert (assm);

    if (assm->error.err)
        return EXIT_FAILURE;

    fprintf (assm->log, "***** FIRST ASSEMBLE *****\n");

    assm->text.wordid = 0;
    assm->code.ip     = 0;
    while (assm->text.wordid < assm->text.wordsize)
    {
        assm_clearerr (assm);

        if (label_handler (assm) && command_handler (assm) && assm->error.err != ASSM_ERRLABEL)
            return EXIT_FAILURE;
    }

    fprintf (assm->log, "***** SECOND ASSEMBLE *****\n");

    assm->text.wordid = 0;
    assm->code.ip     = 0;
    while (assm->text.wordid < assm->text.wordsize)
    {
        assm_clearerr (assm);

        if (command_handler (assm) && (assm->error.err == ASSM_ERRLABEL || assm->error.err == ASSM_ERRSYSTEM) )
            return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

static int label_handler (assm_t *assm)
{
    assert (assm);
    assert (assm->labels.data);
    assert (assm->labels.size < assm->labels.capacity);
    assert (assm->text.word[assm->text.wordid]);

    if (assm->error.err)
        return EXIT_FAILURE;

    char *ptr  = strchr (assm->text.word[assm->text.wordid], ':');

    if (!ptr)
        return EXIT_FAILURE;
    else if (*(ptr + 1) )
        return EXIT_FAILURE;

    fprintf (assm->log, "0x%016lx:\n"
                        "\t[%lu]: \"%s\": <0x%016lx>;\n", 
                        assm->code.ip, assm->text.wordid, assm->text.word[assm->text.wordid], assm->code.ip);

    strncpy (assm->labels.data[assm->labels.size].name, assm->text.word[assm->text.wordid], STRSIZE);
    *strchr (assm->labels.data[assm->labels.size].name, ':') = '\0';

    for (size_t i = 0; i < assm->labels.size; i++)
        if (!strncmp (assm->labels.data[assm->labels.size].name, assm->labels.data[i].name, STRSIZE) )
        {
            assm_seterr (assm, ASSM_ERRREDEF, NULL);

            return EXIT_FAILURE;
        }

    assm->labels.data[assm->labels.size].addr = assm->code.ip;
    assm->labels.size++;

    if (assm->labels.size >= assm->labels.capacity)
    {
        void *ptr = realloc (assm->labels.data, assm->labels.capacity * 2 * sizeof (*assm->labels.data) );
    
        if (!ptr)
        { 
            assm_seterr (assm, ASSM_ERRSYSTEM, "Can't realloc labels array");

            return EXIT_FAILURE;
        }

        assm->labels.data      = ptr;
        assm->labels.capacity *= 2;
    }

    assm->text.wordid++;

    return EXIT_SUCCESS;
    
}

static int command_handler (assm_t *assm)
{
    assert (assm);
    assert (assm->code.data);
    assert (assm->text.word[assm->text.wordid]);

    if (assm->error.err)
        return EXIT_FAILURE;

    if (assm->code.ip + 0x10 >= assm->code.size)
    {
        void *ptr = realloc (assm->code.data, assm->code.size * 2);

        if (!ptr)
        {
            assm_seterr (assm, ASSM_ERRSYSTEM, "Can't realloc code");

            return EXIT_FAILURE;
        }

        assm->code.data  = ptr;
        assm->code.size *= 2;
    }

    uint8_t flag = 0;
    for (size_t i = 0; !flag && i < PROC_CMDCOUNT && cmdtable[i].handler; i++)
        if (!strncmp (assm->text.word[assm->text.wordid], cmdtable[i].name, STRSIZE) )
        {
            flag = 1;
            if (cmdtable[i].handler (assm) )
                return EXIT_FAILURE;
        }

    if (!flag)
    {
        fprintf (assm->log, "0x%016lx:\n"
                            "\t[%lu]: \"%s\": <-> (UNKNOWN COMMAND);\n", 
                            assm->code.ip, assm->text.wordid, assm->text.word[assm->text.wordid]);

        assm->text.wordid++;
    
        assm_seterr (assm, ASSM_ERRCMD, NULL);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int push_handler (assm_t *assm)
{
    assert (assm);
    assert (!assm->error.err);
    assert (assm->text.word);
    assert (assm->text.wordid < assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid]);
    assert (assm->text.word[assm->text.wordid+1]);
    assert (assm->code.ip + 0x10 < assm->code.size);
    assert (assm->code.data);
    assert (assm->log);

    union val arg = {};
    char symbol   = 0;

    fprintf (assm->log, "0x%016lx:\n", assm->code.ip);

    if (sscanf (assm->text.word[assm->text.wordid+1], "[r%hhu%c", &arg.vu8, &symbol) == 2 && symbol == ']')
    {
        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx> (CMD_FLGREG | CMD_FLGMEM);\n", 
                            assm->text.wordid, assm->text.word[assm->text.wordid],
                            CMD_PUSH | CMD_FLGREG | CMD_FLGMEM);

        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx>;\n", 
                            assm->text.wordid + 1, assm->text.word[assm->text.wordid+1], arg.vu8);

        *( (uint8_t *) (assm->code.data + assm->code.ip) )     = CMD_PUSH | CMD_FLGREG | CMD_FLGMEM;
        *( (uint8_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu8;

        assm->code.ip += 2;
    }
    else if (sscanf (assm->text.word[assm->text.wordid+1], "[%lu%c", &arg.vu64, &symbol) == 2 && symbol == ']')
    {
        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx> (CMD_FLGMEM);\n", 
                            assm->text.wordid, assm->text.word[assm->text.wordid],
                            CMD_PUSH | CMD_FLGMEM);

        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%016lx>;\n", 
                            assm->text.wordid + 1, assm->text.word[assm->text.wordid+1], arg.vu64 % PROC_MEMSIZE);

        *( (uint8_t *)  (assm->code.data + assm->code.ip) )     = CMD_PUSH | CMD_FLGMEM;
        *( (uint64_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu64 % PROC_MEMSIZE;

        assm->code.ip += 9;
    }
    else if (sscanf (assm->text.word[assm->text.wordid+1], "r%hhu", &arg.vu8) == 1)
    {
        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx> (CMD_FLGREG);\n", 
                            assm->text.wordid, assm->text.word[assm->text.wordid],
                            CMD_PUSH | CMD_FLGREG);

        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx>;\n", 
                            assm->text.wordid + 1, assm->text.word[assm->text.wordid+1], arg.vu8);

        *( (uint8_t *) (assm->code.data + assm->code.ip) )     = CMD_PUSH | CMD_FLGREG;
        *( (uint8_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu8;

        assm->code.ip += 2;
    }
    else if (sscanf (assm->text.word[assm->text.wordid+1], "%ld", &arg.v64) == 1)
    {
        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx> ();\n", 
                            assm->text.wordid, assm->text.word[assm->text.wordid],
                            CMD_PUSH | CMD_FLGMEM);

        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%016lx>;\n", 
                            assm->text.wordid + 1, assm->text.word[assm->text.wordid+1], arg.v64);

        *( (uint8_t *)  (assm->code.data + assm->code.ip) )     = CMD_PUSH;
        *( (int64_t  *) (assm->code.data + assm->code.ip + 1) ) = arg.v64;

        assm->code.ip += 9;
    }
    else
    {
        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx> ();\n", 
                            assm->text.wordid, assm->text.word[assm->text.wordid],
                            CMD_PUSH | CMD_FLGMEM);

        fprintf (assm->log, "\t[%lu]: \"%s\": <->; (BAD ARGUMENT);\n", 
                            assm->text.wordid + 1, assm->text.word[assm->text.wordid+1]);

        assm_seterr (assm, ASSM_ERRPUSHARG, assm->text.word[assm->text.wordid+1]);

        assm->text.wordid += 2;

        return EXIT_FAILURE;
    }

    assm->text.wordid += 2;

    return EXIT_SUCCESS;
}

int pop_handler (assm_t *assm)
{
    assert (assm);
    assert (!assm->error.err);
    assert (assm->text.word);
    assert (assm->text.wordid < assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid]);
    assert (assm->text.word[assm->text.wordid+1]);
    assert (assm->code.ip + 0x10 < assm->code.size);
    assert (assm->code.data);
    assert (assm->log);

    union val arg = {};
    char symbol   = 0;

    fprintf (assm->log, "0x%016lx:\n", assm->code.ip);

    if (sscanf (assm->text.word[assm->text.wordid+1], "[r%hhu%c", &arg.vu8, &symbol) == 2 && symbol == ']')
    {
        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx> (CMD_FLGREG | CMD_FLGMEM);\n", 
                            assm->text.wordid, assm->text.word[assm->text.wordid],
                            CMD_POP | CMD_FLGREG | CMD_FLGMEM);

        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx>;\n", 
                            assm->text.wordid + 1, assm->text.word[assm->text.wordid+1], arg.vu8);

        *( (uint8_t *) (assm->code.data + assm->code.ip) )     = CMD_POP | CMD_FLGREG | CMD_FLGMEM;
        *( (uint8_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu8;

        assm->code.ip     += 2;
        assm->text.wordid += 2;
    }
    else if (sscanf (assm->text.word[assm->text.wordid+1], "[%lu%c", &arg.vu64, &symbol) == 2 && symbol == ']')
    {
        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx> (CMD_FLGMEM);\n", 
                            assm->text.wordid, assm->text.word[assm->text.wordid],
                            CMD_POP | CMD_FLGMEM);

        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%016lx>;\n", 
                            assm->text.wordid + 1, assm->text.word[assm->text.wordid+1], arg.vu64 % PROC_MEMSIZE);

        *( (uint8_t *)  (assm->code.data + assm->code.ip) )     = CMD_POP | CMD_FLGMEM;
        *( (uint64_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu64 % PROC_MEMSIZE;

        assm->code.ip     += 9;
        assm->text.wordid += 2;
    }
    else if (sscanf (assm->text.word[assm->text.wordid+1], "r%hhu", &arg.vu8) == 1)
    {
        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx> (CMD_FLGREG);\n", 
                            assm->text.wordid, assm->text.word[assm->text.wordid],
                            CMD_POP | CMD_FLGREG);

        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx>;\n", 
                            assm->text.wordid + 1, assm->text.word[assm->text.wordid+1], arg.vu8);

        *( (uint8_t *) (assm->code.data + assm->code.ip) )     = CMD_POP | CMD_FLGREG;
        *( (uint8_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu8;

        assm->code.ip     += 2;
        assm->text.wordid += 2;
    }
    else
    {
        fprintf (assm->log, "\t[%lu]: \"%s\": <0x%02hhx> ();\n", 
                            assm->text.wordid, assm->text.word[assm->text.wordid],
                            CMD_POP);

        fprintf (assm->log, "\t[%lu]: \"%s\": <->; (NO ARGUMENT);\n", 
                            assm->text.wordid + 1, assm->text.word[assm->text.wordid+1]);

        *( (uint8_t *) (assm->code.data + assm->code.ip) ) = CMD_POP;

        assm->code.ip     += 1;
        assm->text.wordid += 1;
    }

    return EXIT_SUCCESS;
}

int argno_handler (assm_t *assm, uint8_t cmd)
{
    assert (assm);
    assert (!assm->error.err);
    assert (assm->text.word);
    assert (assm->text.wordid < assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid]);
    assert (assm->code.data);
    assert (assm->code.ip + 0x10 < assm->code.size);
    assert (assm->log);

    fprintf (assm->log, "0x%016lx:\n"
                        "\t[%lu]: \"%s\": <0x%02hhx>;\n", 
                        assm->code.ip, assm->text.wordid, assm->text.word[assm->text.wordid], cmd);

    *( (uint8_t *) (assm->code.data + assm->code.ip) ) = cmd;

    assm->text.wordid += 1;
    assm->code.ip     += 1;

    return EXIT_SUCCESS;
    
}

int arglb_handler (assm_t *assm, uint8_t cmd)
{
    assert (assm);
    assert (!assm->error.err);
    assert (assm->text.word);
    assert (assm->text.wordid < assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid]);
    assert (assm->text.word[assm->text.wordid+1]);
    assert (assm->labels.data);
    assert (assm->labels.size < assm->labels.capacity);
    assert (assm->code.data);
    assert (assm->code.ip + 0x10 < assm->code.size);
    assert (assm->log);

    fprintf (assm->log, "0x%016lx:\n"
                        "\t[%lu]: \"%s\": <0x%02hhx>;\n", 
                        assm->code.ip, assm->text.wordid, assm->text.word[assm->text.wordid], cmd);

    for (size_t i = 0; i < assm->labels.size; i++)
    {
        assert (assm->labels.data[i].name);

        if (!strncmp (assm->labels.data[i].name, assm->text.word[assm->text.wordid+1], STRSIZE) )
        {
            fprintf (assm->log, "\t[%lu]: \"%s\": <0x%016lx>;\n",
                                assm->text.wordid + 1, assm->text.word[assm->text.wordid + 1], 
                                assm->labels.data[i].addr);

            *( (uint8_t  *) (assm->code.data + assm->code.ip)     ) = cmd;
            *( (uint64_t *) (assm->code.data + assm->code.ip + 1) ) = assm->labels.data[i].addr;

            assm->code.ip     += 9; 
            assm->text.wordid += 2;

            return EXIT_SUCCESS;
        }    
    }

    fprintf (assm->log, "\t[%lu]: \"%s\": <-> (UNKNOWN LABEL);\n",
                        assm->text.wordid + 1, assm->text.word[assm->text.wordid + 1]);
    
    assm_seterr (assm, ASSM_ERRLABEL, assm->text.word[assm->text.wordid+1]);

    assm->code.ip     += 9;
    assm->text.wordid += 2;

    return EXIT_FAILURE;
}

int add_handler (assm_t *assm)
{
    return argno_handler (assm, CMD_ADD);
}

int sub_handler (assm_t *assm)
{
    return argno_handler (assm, CMD_SUB);
}

int mul_handler (assm_t *assm)
{
    return argno_handler (assm, CMD_MUL);
}

int div_handler (assm_t *assm)
{
    return argno_handler (assm, CMD_DIV);
}

int cmp_handler (assm_t *assm)
{
    return argno_handler (assm, CMD_CMP);
}

int hlt_handler (assm_t *assm)
{
    return argno_handler (assm, CMD_HLT);
}

int ret_handler (assm_t *assm)
{
    return argno_handler (assm, CMD_RET);
}

int in_handler (assm_t *assm)
{
    return argno_handler (assm, CMD_IN);
}

int out_handler (assm_t *assm)
{
    return argno_handler (assm, CMD_OUT);
}

int jmp_handler (assm_t *assm)
{
    return arglb_handler (assm, CMD_JMP);
}

int call_handler (assm_t *assm)
{
    return arglb_handler (assm, CMD_CALL);
}

int je_handler (assm_t *assm)
{
    return arglb_handler (assm, CMD_JE);
}

int jl_handler (assm_t *assm)
{
    return arglb_handler (assm, CMD_JL);
}

int jle_handler (assm_t *assm)
{
    return arglb_handler (assm, CMD_JLE);
}

int jmt_handler (assm_t *assm)
{
    return arglb_handler (assm, CMD_JMT);
}

int jfl_handler (assm_t *assm)
{
    return arglb_handler (assm, CMD_JFL);
}
