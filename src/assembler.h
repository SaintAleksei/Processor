#ifndef ASSEMBLER_H_INCLUDED
#define ASSEMBLER_H_INCLUDED

#include "setup.h"
#include <stdlib.h>
#include <stdio.h>

#define STRSIZE 0x40

enum ASSM_ERRORS
{
    ASSM_NOERR,
    ASSM_ERRLABEL,
    ASSM_ERRREDEF,
    ASSM_ERRCMD,
    ASSM_ERRPUSHARG,
    ASSM_ERRSYSTEM,
    ASSM_ERRCREATE,
    ASSM_ERRWRITE,
};

struct assm_text
{
    char    *buff;
    size_t   buffsize;
    char   **word;
    size_t   wordsize;
    size_t   wordid;
    char     name[STRSIZE];
};

struct assm_code
{
    void     *data;
    uint64_t  size;
    uint64_t  ip;
};

struct assm_label
{
    char     name[STRSIZE]; 
    uint64_t addr;
};

struct assm_labels_array
{
    struct assm_label *data;
    size_t             capacity;
    size_t             size;
};

struct assm_error
{
    enum ASSM_ERRORS  err;
    const char       *str;
};

typedef struct assembler
{
    struct assm_text          text;
    struct assm_labels_array  labels;
    struct assm_code          code;
    struct assm_error         error;
    FILE                     *log;
} assm_t;

int  assm_create    (assm_t *assm, const char *name);
int  assm_translate (assm_t *assm);
int  assm_write     (assm_t *assm);
void assm_error     (assm_t *assm);
void assm_delete    (assm_t *assm);

#endif
