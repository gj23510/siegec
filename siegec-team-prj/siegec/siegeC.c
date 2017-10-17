#include "common.h"
//#include <locale.h>
int main(int argc, char *argv[]) {
	printf("sigec  \n");
	printf("\n");
	if(argc != 2) {
		printf("no file\n");
		exit(1);
	}
	if((p_buf = (char *) malloc(PROG_SIZE))==NULL) {
		printf("xx exited");
		exit(1);
	}
	if(!load_program(p_buf, argv[1])) {
		exit(1);
	}
	if(setjmp(e_buf)) {
		exit(1);  
	}
	gvar_index = 0;  
	
	prog = p_buf;
	prescan(); 
           
	lvartos = 0;     
	functos = 0;     

	char *main_ = "main";
	prog = find_func(main_); 
	if(!prog) { 
		printf("main() exited\n");
		exit(1);
	}
	prog--; 
	strcpy(token, "main");
	call(); 
	free_arr();
	return 0;
}
