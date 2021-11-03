#ifndef CMDTABLE_H_INCLUDED
#define CMDTABLE_H_INCLUDED

struct cmdtable_elem
{
    const char *name;
    uint8_t     code;
    int       (*handler) (void *arg);
    int       (*exec)    (void *arg);
};

extern const struct cmdtable_elem cmdtable[PROC_CMDCOUNT];

#endif
