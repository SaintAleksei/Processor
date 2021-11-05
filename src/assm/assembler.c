#include "setup.h"
#include "assembler.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define ASSM_ERR(errcode, errstr)\
    do\
    {\
        assert (assm);\
        assm->error.err  = errcode;\
        assm->error.func = __PRETTY_FUNCTION__;\
        if (assm->text.word && assm->text.wordid < assm->text.wordsize)\
            assm->error.word = assm->text.word[assm->text.wordid].str;\
        assm->error.str  = errstr;\
        return EXIT_FAILURE;\
    }\
    while (0)

static int         assm_resize     (assm_t *assm);
static const char *assm_strerror   (enum ASSM_ERRORS err);

static int word_handler (assm_t *assm);

static int res_handler   (assm_t *assm);
static int label_handler (assm_t *assm);
static int func_handler  (assm_t *assm);

#define PROC_GENCMD(name, CODE, ...)\
    static int name##_##handler (assm_t *assm);

PROC_GENCODE

#undef PROC_GENCMD

#define PROC_GENCMD(name, CODE, ...)\
    {#name, name##_##handler},

struct handler_table_elem
{
    const char *name;
    int       (*handler) (assm_t *assm);
} cmdtable[PROC_CMDCOUNT] =
{
    PROC_GENCODE
    {"res"  , res_handler  },
    {"label", label_handler},
    {"func" , func_handler },
};

#undef PROC_GENCMD


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

        assm->text.word = calloc (assm->text.wordsize, sizeof (*assm->text.word) );

        if (!assm->text.word)
            break;

        assm->text.wordsize = 0;
        flag                = 1;
        for (size_t i = 0; i < assm->text.buffsize; i++)
            if (assm->text.buff[i] && flag)
            {
                assm->text.word[assm->text.wordsize].str  = assm->text.buff + i;
                assm->text.word[assm->text.wordsize].size = strlen (assm->text.buff + i);
                assm->text.wordsize++;
                flag = 0;
            }
            else if (!assm->text.buff[i] && !flag)
                flag = 1;

        assm->text.wordsize++;
        assm->text.word = realloc (assm->text.word, assm->text.wordsize * sizeof (*assm->text.word) );

        if (!assm->text.word)
            break;
    
        assm->text.word[assm->text.wordsize-1].str = assm->text.buff + assm->text.buffsize;

        assm->code.size = 0x20;
        assm->code.data = calloc (1, assm->code.size);
        if (!assm->code.data)
            break;

        assm->labeltable.capacity = 1;
        assm->labeltable.data     = calloc (assm->labeltable.capacity, sizeof (*assm->labeltable.data));
        if (!assm->labeltable.data)
            break;

        assm->restable.capacity = 1;
        assm->restable.data     = calloc (assm->restable.capacity, sizeof (*assm->restable.data));
        if (!assm->restable.data)
            break;

        assm->functable.capacity = 1;
        assm->functable.data     = calloc (assm->functable.capacity, sizeof (*assm->functable.data));
        if (!assm->functable.data)
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
    free (assm->labeltable.data);
    free (assm->restable.data);
    free (assm->functable.data);

    memset (assm, 0, sizeof (*assm) );

    ASSM_ERR (ASSM_ERRSYSTEM, errstr);
}

void assm_delete (assm_t *assm)
{
    assert (assm);

    if (assm->log)
        fclose (assm->log);
    free (assm->text.buff);
    free (assm->text.word);
    free (assm->code.data);  
    free (assm->labeltable.data);
    free (assm->restable.data);
    free (assm->functable.data);

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

    ASSM_ERR (ASSM_ERRSYSTEM, errptr);
}

void assm_error (assm_t *assm)
{
    assert (assm);

    fprintf (stderr, "ERROR: ");

    if (assm->error.func)
        fprintf (stderr, "%s: ", assm->error.func);

    fprintf (stderr, "%s", assm_strerror (assm->error.err) );
    
    if (assm->error.word)
        fprintf (stderr, ": \"%s\"", assm->error.word);

    if (assm->error.str)
        fprintf (stderr, ": %s", assm->error.str);

    fprintf (stderr, "\n");
}

int assm_translate (assm_t *assm)
{
    assert (assm);
    assert (assm->text.word);

    if (assm->error.err)
        return EXIT_FAILURE;
    
    uint8_t flag      = 0;
    assm->text.wordid = 0;
    assm->code.ip     = 0;
    assm->passnum     = ASSM_PASS1;
    while (assm->text.wordid < assm->text.wordsize)
    {
        assert (assm->text.wordid < assm->text.wordsize);
        assert (assm->text.word[assm->text.wordid].str);

        if (!*assm->text.word[assm->text.wordid].str)
            break;

        if (word_handler (assm) )
            return EXIT_FAILURE;

        if (assm_resize (assm) )
            return EXIT_FAILURE;

        flag = 0;
        for (size_t i = 0; !flag && i < PROC_CMDCOUNT && cmdtable[i].handler; i++)
            if (!strncmp (assm->text.word[assm->text.wordid].str, cmdtable[i].name, STRSIZE) )
            {
                flag = 1;
                if (cmdtable[i].handler (assm) )
                    return EXIT_FAILURE;
            }

        if (!flag)
            ASSM_ERR (ASSM_ERRCOMMAND, NULL);
    }

    assm->text.wordid = 0;
    assm->code.ip     = 0;
    assm->passnum     = ASSM_PASS2;
    while (assm->text.wordid < assm->text.wordsize)
    {
        assert (assm->text.wordid < assm->text.wordsize);
        assert (assm->text.word[assm->text.wordid].str);

        if (!*assm->text.word[assm->text.wordid].str)
            break;

        flag = 0;
        for (size_t i = 0; !flag && i < PROC_CMDCOUNT && cmdtable[i].handler; i++)
            if (!strncmp (assm->text.word[assm->text.wordid].str, cmdtable[i].name, STRSIZE) )
            {
                flag = 1;
                if (cmdtable[i].handler (assm) )
                    return EXIT_FAILURE;
            }
    }
    
    return EXIT_SUCCESS;
}

static int word_handler (assm_t *assm)
{
    assert (assm);
    assert (assm->text.word);
    assert (assm->text.wordid < assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid].str);

    if (assm->text.word[assm->text.wordid].size >= STRSIZE)
        ASSM_ERR (ASSM_ERRWORD, "Too long");
    
    return EXIT_SUCCESS;
}

static int assm_resize (assm_t *assm)
{
    assert (assm);
    assert (assm->code.data);
    assert (assm->labeltable.data);
    assert (assm->restable.data);
    assert (assm->functable.data);
    
    void *ptr = NULL;

    if (assm->code.ip + 0x10 >= assm->code.size)
    {
        ptr = realloc (assm->code.data, (assm->code.ip + 0x10) * 2);

        if (!ptr)
            ASSM_ERR (ASSM_ERRSYSTEM, "Can't realloc code");

        assm->code.size  = (assm->code.ip + 0x10) * 2;
        assm->code.data  = ptr;

        memset (assm->code.data + assm->code.ip, 0, assm->code.size - assm->code.ip);
    }

    if (assm->labeltable.size >= assm->labeltable.capacity)
    {
        ptr = realloc (assm->labeltable.data, assm->labeltable.size * 2 * sizeof (*assm->labeltable.data) );

        if (!ptr)
            ASSM_ERR (ASSM_ERRSYSTEM, "Can't realloc labels table");

        assm->labeltable.capacity = assm->labeltable.size * 2;
        assm->labeltable.data     = ptr;

        memset (assm->labeltable.data + assm->labeltable.size, 0, 
                (assm->labeltable.capacity - assm->labeltable.size) * sizeof (*assm->labeltable.data) );
    }

    if (assm->restable.size >= assm->restable.capacity)
    {
        ptr = realloc (assm->restable.data, assm->restable.size * 2 * sizeof (*assm->restable.data) );

        if (!ptr)
            ASSM_ERR (ASSM_ERRSYSTEM, "Can't realloc reserves table");

        assm->restable.capacity = assm->restable.size * 2;
        assm->restable.data     = ptr;

        memset (assm->restable.data + assm->restable.size, 0, 
                (assm->restable.capacity - assm->restable.size) * sizeof (*assm->restable.data) );
    }

    if (assm->functable.size >= assm->functable.capacity)
    {
        ptr = realloc (assm->functable.data, assm->functable.size * 2 * sizeof (*assm->functable.data) );

        if (!ptr)
            ASSM_ERR (ASSM_ERRSYSTEM, "Can't realloc functions table");

        assm->functable.capacity = assm->functable.size * 2;
        assm->functable.data     = ptr;  

        memset (assm->functable.data + assm->functable.size, 0, 
                (assm->functable.capacity - assm->functable.size) * sizeof (*assm->functable.data) );
    }

    return EXIT_SUCCESS;
}

static const char *assm_strerror (enum ASSM_ERRORS err)
{
    switch (err)
    {
        case ASSM_NOERR:
            return "No error";
        case ASSM_ERRWORD:
            return "Bad word";
        case ASSM_ERRCOMMAND:
            return "Unknown command";
        case ASSM_ERRLABEL:
            return "Bad label";
        case ASSM_ERRRES:
            return "Bad reserve";
        case ASSM_ERRFUNC:
            return "Bad function";
        case ASSM_ERRARG:
            return "Bad argument";
        case ASSM_ERRSYSTEM:
            return "System error";
    } 
    
    return "Undefined error";
}

static int label_handler (assm_t *assm)
{
    assert (assm);
    assert (!assm->error.err);
    assert (assm->labeltable.data);
    assert (assm->labeltable.size < assm->labeltable.capacity);
    assert (assm->text.wordid + 1 < assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid+1].str);

    if (assm->passnum == ASSM_PASS2)
    {
        assm->text.wordid += 2;

        return EXIT_SUCCESS;
    }

    assm->text.wordid++;

    const char *word = assm->text.word[assm->text.wordid].str;
    size_t       len = assm->text.word[assm->text.wordid].size;

    assert (len < STRSIZE);

    for (size_t i = 0; i < len; i++)
        if (!isalnum (word[i]) )
            ASSM_ERR (ASSM_ERRLABEL, "Bad syntax");

    for (size_t i = 0; i < assm->labeltable.size; i++)
        if (!strncmp (assm->labeltable.data[i].name, word, STRSIZE) )
            ASSM_ERR (ASSM_ERRLABEL, "Redefinition");

    strncpy (assm->labeltable.data[assm->labeltable.size].name, word, STRSIZE);
    assm->labeltable.data[assm->labeltable.size].ip = assm->code.ip;

    assm->labeltable.size++;

    assm->text.wordid++;

    return EXIT_SUCCESS;
}

static int func_handler (assm_t *assm)
{
    assert (assm);
    assert (!assm->error.err);
    assert (assm->functable.data);
    assert (assm->functable.size < assm->functable.capacity);
    assert (assm->text.wordid + 1 < assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid+1].str);

    if (assm->passnum == ASSM_PASS2)
    {
        assm->text.wordid += 2;

        return EXIT_SUCCESS;
    }

    assm->text.wordid++;

    const char *word = assm->text.word[assm->text.wordid].str;
    size_t       len = assm->text.word[assm->text.wordid].size;

    assert (len < STRSIZE);

    for (size_t i = 0; i < len; i++)
        if (!isalnum (word[i]) )
            ASSM_ERR (ASSM_ERRLABEL, "Bad syntax");

    for (size_t i = 0; i < assm->functable.size; i++)
        if (!strncmp (assm->functable.data[i].name, word, STRSIZE) )
            ASSM_ERR (ASSM_ERRLABEL, "Redefinition");

    strncpy (assm->functable.data[assm->functable.size].name, word, STRSIZE);
    assm->functable.data[assm->functable.size].ip = assm->code.ip;

    assm->functable.size++;
    
    assm->text.wordid++;

    return EXIT_SUCCESS;
}

static int res_handler (assm_t *assm)
{
    assert (assm);
    assert (!assm->error.err);
    assert (assm->restable.data);
    assert (assm->restable.size < assm->restable.capacity);
    assert (assm->text.wordid + 1 < assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid+1].str);

    if (assm->passnum == ASSM_PASS2)
    {
        assm->text.wordid += 2;

        return EXIT_SUCCESS;
    }

    assm->text.wordid++;

    const char *word = assm->text.word[assm->text.wordid].str;
    int          len = (int) assm->text.word[assm->text.wordid].size;
    int        count = 0;
    int          ret = 0;
    char      symbol = 0; 
    uint64_t   size  = 0;

    assert (len < STRSIZE);

    ret = sscanf (word, "%[a-zA-Z0-9]%c%lu%n", 
                        assm->restable.data[assm->restable.size].name,
                        &symbol, &size, &count);

    if (ret != 3 || symbol != ':' || count != len)
        ASSM_ERR (ASSM_ERRRES, "Bad syntax");

    if (size == 0)
        ASSM_ERR (ASSM_ERRRES, "Can't reserve 0 size");

    for (size_t i = 0; i < assm->restable.size; i++)
        if (!strncmp (assm->restable.data[assm->restable.size].name, assm->restable.data[i].name, STRSIZE) )
            ASSM_ERR (ASSM_ERRRES, "Redefinition");

    if (assm->restable.size)
        assm->restable.data[assm->restable.size].addr = assm->restable.data[assm->restable.size-1].addr +
                                                        assm->restable.data[assm->restable.size-1].size;
    else
        assm->restable.data[assm->restable.size].addr = 0;

    assm->restable.data[assm->restable.size].size = size;

    if (assm->restable.data[assm->restable.size].addr +
        assm->restable.data[assm->restable.size].size >= PROC_MEMSIZE)
        ASSM_ERR (ASSM_ERRRES, "Not enough memory");

    assm->restable.size++;

    assm->text.wordid++;

    return EXIT_SUCCESS;
}

static int push_handler (assm_t *assm)
{
    assert (assm);
    assert (!assm->error.err);
    assert (assm->text.word);
    assert (assm->text.wordid + 1 < assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid+1].str);
    assert (assm->code.ip + 0x10 < assm->code.size);
    assert (assm->code.data);
    assert (assm->restable.data);
    assert (assm->restable.size < assm->restable.capacity);

    assm->text.wordid++;

    const char *word          = assm->text.word[assm->text.wordid].str;
    int         len           = (int) assm->text.word[assm->text.wordid].size;
    char        name[STRSIZE] = "";
    char        symbol        = 0;
    int         count         = 0;
    int         ret           = 0;
    union val   arg           = {};

    ret = sscanf (word, "[r%hhu%c%n", &arg.vu8, &symbol, &count);

    if (ret == 2 && symbol == ']' && count == len)
    {
        *( (uint8_t *) (assm->code.data + assm->code.ip) )     = CMD_PUSH | CMD_FLGREG | CMD_FLGMEM;
        *( (uint8_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu8;

        assm->code.ip += 2;
        assm->text.wordid++;

        return EXIT_SUCCESS;
    }

    ret = sscanf (word, "[%lu%c%n", &arg.vu64, &symbol, &count);

    if (ret == 2 && symbol == ']' && count == len)
    {
        if (arg.vu64 >= PROC_MEMSIZE)
            ASSM_ERR (ASSM_ERRARG, "Not enough memory");

        *( (uint8_t *)  (assm->code.data + assm->code.ip) )     = CMD_PUSH | CMD_FLGMEM;
        *( (uint64_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu64;

        assm->code.ip += 9;
        assm->text.wordid++;

        return EXIT_SUCCESS;
    }
    
    ret = sscanf (word, "r%hhu%n", &arg.vu8, &count);

    if (ret == 1 && count == len)
    {
        *( (uint8_t *) (assm->code.data + assm->code.ip) )     = CMD_PUSH | CMD_FLGREG;
        *( (uint8_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu8;

        assm->code.ip += 2;
        assm->text.wordid++;

        return EXIT_SUCCESS;
    }

    ret = sscanf (word, "%ld%n", &arg.v64, &count);

    if (ret == 1 && count == len)
    {
        *( (uint8_t *)  (assm->code.data + assm->code.ip) )     = CMD_PUSH;
        *( (int64_t  *) (assm->code.data + assm->code.ip + 1) ) = arg.v64;

        assm->code.ip += 9;
        assm->text.wordid++;

        return EXIT_SUCCESS;
    }

    ret = sscanf (word, "[%[a-zA-Z0-9]%c%n", name, &symbol, &count);

    if (ret == 2 && symbol == ']' && count == len)
    {
        for (size_t i = 0; i < assm->restable.size; i++)
        {
            assert (assm->restable.data[i].name);

            if (!strncmp (assm->restable.data[i].name, name, STRSIZE) )
            {
                *( (uint8_t *)  (assm->code.data + assm->code.ip) )     = CMD_PUSH | CMD_FLGMEM;
                *( (uint64_t *) (assm->code.data + assm->code.ip + 1) ) = assm->restable.data[i].addr;

                assm->code.ip += 9;
                assm->text.wordid++;
                
                return EXIT_SUCCESS;
            }
        }

        ASSM_ERR (ASSM_ERRARG, "Unknown name");
    }

    ret = sscanf (word, "%[a-zA-Z0-9]%n", name, &count);

    if (ret == 1 && count == len)
    {
        for (size_t i = 0; i < assm->restable.size; i++)
        {
            assert (assm->restable.data[i].name);

            if (!strncmp (assm->restable.data[i].name, name, STRSIZE) )
            {
                *( (uint8_t *)  (assm->code.data + assm->code.ip) )     = CMD_PUSH;
                *( (uint64_t *) (assm->code.data + assm->code.ip + 1) ) = assm->restable.data[i].addr;

                assm->code.ip += 9;
                assm->text.wordid++;
                
                return EXIT_SUCCESS;
            }
        }

        ASSM_ERR (ASSM_ERRARG, "Unknown name");
    }

    ASSM_ERR (ASSM_ERRARG, "Bad syntax");
}

static int pop_handler (assm_t *assm)
{
    assert (assm);
    assert (!assm->error.err);
    assert (assm->text.word);
    assert (assm->text.wordid + 1 < assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid+1].str);
    assert (assm->code.ip + 0x10 < assm->code.size);
    assert (assm->code.data);
    assert (assm->restable.data);
    assert (assm->restable.size < assm->restable.capacity);

    assm->text.wordid++;

    const char *word          = assm->text.word[assm->text.wordid].str;
    int         len           = (int) assm->text.word[assm->text.wordid].size;
    char        name[STRSIZE] = "";
    char        symbol        = 0;
    int         count         = 0;
    int         ret           = 0;
    union val   arg           = {};

    ret = sscanf (word, "[r%hhu%c%n", &arg.vu8, &symbol, &count);

    if (ret == 2 && symbol == ']' && count == len)
    {
        *( (uint8_t *) (assm->code.data + assm->code.ip) )     = CMD_POP | CMD_FLGREG | CMD_FLGMEM;
        *( (uint8_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu8;

        assm->code.ip += 2;
        assm->text.wordid++;

        return EXIT_SUCCESS;
    }

    ret = sscanf (word, "[%lu%c%n", &arg.vu64, &symbol, &count);

    if (ret == 2 && symbol == ']' && count == len)
    {
        if (arg.vu64 >= PROC_MEMSIZE)
            ASSM_ERR (ASSM_ERRARG, "Not enough memory");

        *( (uint8_t *)  (assm->code.data + assm->code.ip) )     = CMD_POP | CMD_FLGMEM;
        *( (uint64_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu64;

        assm->code.ip += 9;
        assm->text.wordid++;

        return EXIT_SUCCESS;
    }
    
    ret = sscanf (word, "r%hhu%n", &arg.vu8, &count);

    if (ret == 1 && count == len)
    {
        *( (uint8_t *) (assm->code.data + assm->code.ip) )     = CMD_POP | CMD_FLGREG;
        *( (uint8_t *) (assm->code.data + assm->code.ip + 1) ) = arg.vu8;

        assm->code.ip += 2;
        assm->text.wordid++;

        return EXIT_SUCCESS;
    }

    ret = sscanf (word, "[%[a-zA-Z0-9]%c%n", name, &symbol, &count);

    if (ret == 2 && symbol == ']' && count == len)
    {
        for (size_t i = 0; i < assm->restable.size; i++)
        {
            assert (assm->restable.data[i].name);

            if (!strncmp (assm->restable.data[i].name, name, STRSIZE) )
            {
                *( (uint8_t *)  (assm->code.data + assm->code.ip) )     = CMD_POP | CMD_FLGMEM;
                *( (uint64_t *) (assm->code.data + assm->code.ip + 1) ) = assm->restable.data[i].addr;

                assm->code.ip += 9;
                assm->text.wordid++;
                
                return EXIT_SUCCESS;
            }
        }

        ASSM_ERR (ASSM_ERRARG, "Unknown name");
    }

    assm->code.ip += 1;
    assm->text.wordid++;

    return EXIT_SUCCESS;
}

static int call_handler (assm_t *assm)
{
    assert (assm);
    assert (!assm->error.err);
    assert (assm->text.word);
    assert (assm->text.wordid + 1< assm->text.wordsize);
    assert (assm->text.word[assm->text.wordid+1].str);
    assert (assm->functable.data);
    assert (assm->functable.size < assm->functable.capacity);
    assert (assm->code.data);
    assert (assm->code.ip + 0x10 < assm->code.size);

    assm->text.wordid++;

    for (size_t i = 0; i < assm->functable.size; i++)
    {
        assert (assm->functable.data[i].name);

        if (!strncmp (assm->functable.data[i].name, assm->text.word[assm->text.wordid].str, STRSIZE) )
        {
            *( (uint8_t  *) (assm->code.data + assm->code.ip)     ) = CMD_CALL;
            *( (uint64_t *) (assm->code.data + assm->code.ip + 1) ) = assm->functable.data[i].ip;

            assm->code.ip += 9;
            assm->text.wordid++;

            return EXIT_SUCCESS;
        }
    }

    if (assm->passnum == ASSM_PASS2)
        ASSM_ERR (ASSM_ERRARG, "Unknown name");

    assm->code.ip += 9;
    assm->text.wordid++;

    return EXIT_SUCCESS;
}

#define ASSM_GEN_CMD_HANDLER(name, CODE)\
static int name##_handler (assm_t *assm)\
{\
    assert (assm);\
    assert (!assm->error.err);\
    assert (assm->code.data);\
    assert (assm->code.ip + 0x10 < assm->code.size);\
\
    *( (uint8_t *) (assm->code.data + assm->code.ip) ) = CODE;\
\
    assm->text.wordid += 1;\
    assm->code.ip     += 1;\
\
    return EXIT_SUCCESS;\
}

ASSM_GEN_CMD_HANDLER (add, CMD_ADD)
ASSM_GEN_CMD_HANDLER (sub, CMD_SUB)
ASSM_GEN_CMD_HANDLER (mul, CMD_MUL)
ASSM_GEN_CMD_HANDLER (div, CMD_DIV)
ASSM_GEN_CMD_HANDLER (hlt, CMD_HLT)
ASSM_GEN_CMD_HANDLER (ret, CMD_RET)
ASSM_GEN_CMD_HANDLER (in , CMD_IN)
ASSM_GEN_CMD_HANDLER (out, CMD_OUT)

#define ASSM_GEN_JMP_HANDLER(command, CODE)\
static int command##_handler (assm_t *assm)\
{\
    assert (assm);\
    assert (!assm->error.err);\
    assert (assm->text.word);\
    assert (assm->text.wordid + 1< assm->text.wordsize);\
    assert (assm->text.word[assm->text.wordid+1].str);\
    assert (assm->labeltable.data);\
    assert (assm->labeltable.size < assm->labeltable.capacity);\
    assert (assm->code.data);\
    assert (assm->code.ip + 0x10 < assm->code.size);\
\
    assm->text.wordid++;\
\
    for (size_t i = 0; i < assm->labeltable.size; i++)\
    {\
        assert (assm->labeltable.data[i].name);\
\
        if (!strncmp (assm->labeltable.data[i].name, assm->text.word[assm->text.wordid].str, STRSIZE) )\
        {\
            *( (uint8_t  *) (assm->code.data + assm->code.ip)     ) = CODE;\
            *( (uint64_t *) (assm->code.data + assm->code.ip + 1) ) = assm->labeltable.data[i].ip;\
\
            assm->code.ip += 9;\
            assm->text.wordid++;\
\
            return EXIT_SUCCESS;\
        }\
    }\
\
    if (assm->passnum == ASSM_PASS2)\
        ASSM_ERR (ASSM_ERRARG, "Unknown name");\
\
    assm->code.ip += 9;\
    assm->text.wordid++;\
\
    return EXIT_SUCCESS;\
}

ASSM_GEN_JMP_HANDLER (jmp, CMD_JMP)
ASSM_GEN_JMP_HANDLER (je , CMD_JE )
ASSM_GEN_JMP_HANDLER (jl , CMD_JL )
ASSM_GEN_JMP_HANDLER (jle, CMD_JLE)
ASSM_GEN_JMP_HANDLER (jmt, CMD_JMT)
ASSM_GEN_JMP_HANDLER (jfl, CMD_JFL)

#undef ASSM_ERR
#undef ASSM_GEN_JMP_HANDLER
#undef ASSM_GEN_CMD_HANDLER
