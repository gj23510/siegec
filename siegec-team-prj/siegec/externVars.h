#ifndef EXTERN_VAR_DECL_H
#define EXTERN_VAR_DECL_H
#include <setjmp.h>
extern char token_type;
extern char tok;        
extern char *prog;      
extern char token[80];  
extern char *p_buf;     
extern jmp_buf e_buf;   
extern int gvar_index;  
extern int garr_index;  
extern int lvartos;     
extern int larrtos;    
extern int functos;     
extern int ret_value;   
#endif //EXTERN_VAR_DECL_H
