#ifndef EXEC_H_INCLUDED
#define EXEC_H_INCLUDED

int push_exec (void *arg); 
int pop_exec  (void *arg); 
int add_exec  (void *arg); 
int sub_exec  (void *arg); 
int mul_exec  (void *arg); 
int cmp_exec  (void *arg);
int div_exec  (void *arg); 
int ret_exec  (void *arg); 
int hlt_exec  (void *arg); 
int call_exec (void *arg); 
int jmp_exec  (void *arg); 
int je_exec   (void *arg); 
int jl_exec   (void *arg); 
int jle_exec  (void *arg); 
int in_exec   (void *arg);
int out_exec  (void *arg);

#endif
