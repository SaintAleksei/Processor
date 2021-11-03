#ifndef HANDLER_H_INCLUDED
#define HANDLER_H_INCLUDED

int push_handler (void *arg);
int pop_handler  (void *arg);
int add_handler  (void *arg);
int sub_handler  (void *arg);
int mul_handler  (void *arg);
int div_handler  (void *arg);
int cmp_handler  (void *arg);
int hlt_handler  (void *arg);
int ret_handler  (void *arg);
int call_handler (void *arg);
int jmp_handler  (void *arg);
int je_handler   (void *arg);
int jl_handler   (void *arg);
int jle_handler  (void *arg);
int in_handler   (void *arg);
int out_handler  (void *arg);

#endif
