#ifndef ASSEMBLER_H_INCLUDED
#define ASSEMBLER_H_INCLUDED

#include "setup.h"
#include <stdlib.h>
#include <stdio.h>

#define STRSIZE 0x40

enum ASSM_ERRORS
{
    ASSM_NOERR,
    ASSM_ERRWORD,
    ASSM_ERRCOMMAND,
    ASSM_ERRLABEL,
    ASSM_ERRFUNC,
    ASSM_ERRRES,
    ASSM_ERRARG,
    ASSM_ERRSYSTEM,
};

enum ASSM_PASSNUM
{
    ASSM_PASS1,
    ASSM_PASS2,
};
    
struct assm_word
{
    const char *str;
    size_t      size;
};

struct assm_text
{
    char             *buff;
    size_t            buffsize;
    struct assm_word *word;
    size_t            wordsize;
    size_t            wordid;
    char              name[STRSIZE];
};

struct assm_code
{
    void     *data;
    uint64_t  size;
    uint64_t  ip;
};

struct assm_labeltable_elem
{
    char     name[STRSIZE]; 
    uint64_t ip;
};

struct assm_labeltable
{
    struct assm_labeltable_elem *data;
    size_t                       capacity;
    size_t                       size;
};

struct assm_restable_elem
{
    char     name[STRSIZE];
    uint64_t addr;
    uint64_t size;
};

struct assm_restable
{
    struct assm_restable_elem *data;
    size_t                     capacity;
    size_t                     size;
};

struct assm_functable_elem
{
    char     name[STRSIZE];
    uint64_t ip;
};

struct assm_functable
{
    struct assm_functable_elem *data;
    size_t                      capacity;
    size_t                      size;
};

struct assm_error
{
    enum ASSM_ERRORS  err;
    const char       *func;
    const char       *word;
    const char       *str;
};

typedef struct assembler
{
    struct assm_text          text;
    struct assm_labeltable    labeltable;
    struct assm_restable      restable;
    struct assm_functable     functable;
    struct assm_code          code;
    struct assm_error         error;
    enum   ASSM_PASSNUM       passnum;
    FILE                     *log;
} assm_t;

int  assm_create    (assm_t *assm, const char *name);
int  assm_translate (assm_t *assm);
int  assm_write     (assm_t *assm);
void assm_error     (assm_t *assm);
void assm_delete    (assm_t *assm);

#endif
