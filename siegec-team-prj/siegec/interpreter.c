#include "externVars.h"
#include "commonEnums.h"
#include "restrictions.h"
#include "analyzer.h"
#include "interpreter.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
char token_type; 
char tok;        
char *prog;    
char *p_buf;     
char token[80];  
jmp_buf e_buf;    
int functos;     
int func_index; 
int gvar_index; 
int garr_index;  
int lvartos;     
int larrtos;     

struct commands table[] = { 
	{"if", IF}, 
	{"else", ELSE},
	{"for", FOR},
	{"do", DO},
	{"while", WHILE},
	{"char", CHAR},
	{"int", INT},
	{"return", RETURN},
	{"end", END},
	{"", END} 
};
struct func_type {
	char func_name[ID_LEN];
	int ret_type;
	char *loc; 
} func_table[NUM_FUNC];
int call_stack[NUM_FUNC];

struct var_type {
	char var_name[ID_LEN];
	int v_type;
	int value;
} global_vars[NUM_GLOBAL_VARS];

struct var_type local_var_stack[NUM_LOCAL_VARS];

struct array_type {
	char arr_name[ID_LEN];
	int arr_type;
	int *int_arr;
	char *char_arr;
	int length;
} global_arrays[NUM_GLOBAL_ARRAYS];
struct array_type local_arr_stack[NUM_LOCAL_ARRS];
int ret_value; 
void prescan(void) {
	char *p, *tp;
	char temp[32];
	int datatype;
	int brace = 0;  
	p = prog;
	func_index = 0;
	do {
		while(brace) { 
			get_token();
			if(*token == '{') {
				brace++;
			}
			if(*token == '}') {
				brace--;
			}
		}
		tp = prog; 
		get_token();
	
		if(tok==CHAR || tok==INT) {
			datatype = tok; 
			get_token();
			if(token_type == IDENTIFIER) {
				strcpy(temp, token);
				get_token();
				if(token_type == ARRAY) {
					prog = tp;
					decl_global_array();
				} else if(*token != '(') {
					prog = tp;
					decl_global_var();
				} else if(*token == '(') {  
					func_table[func_index].loc = prog;
					func_table[func_index].ret_type = datatype;
					strcpy(func_table[func_index].func_name, temp);
					func_index++;
					while(*prog != ')') {
						prog++;
					}
					prog++;
					
				} else {
					putback();
				}
			}
		} else if(*token == '{') {
			brace++;
		}
	} while(tok != FINISHED);
	prog = p;
}

int is_var(char *s) {
	register int i;
	
	for(i=lvartos-1; i >= call_stack[functos-1]; i--)
		if(!strcmp(local_var_stack[i].var_name, token)) {
			return 1;
		}

	for(i=0; i < NUM_GLOBAL_VARS; i++)
		if(!strcmp(global_vars[i].var_name, s)) {
			return 1;
		}
	return 0;
}

void assign_var(char *var_name, int value) {
	register int i;
	
	for(i=lvartos-1; i >= call_stack[functos-1]; i--)  {
		if(!strcmp(local_var_stack[i].var_name, var_name)) {
			local_var_stack[i].value = value;
			return;
		}
	}
	if(i < call_stack[functos-1])
	
		for(i=0; i < NUM_GLOBAL_VARS; i++)
			if(!strcmp(global_vars[i].var_name, var_name)) {
				global_vars[i].value = value;
				return;
			}
	sntx_err(NOT_VAR); 
}

void assign_arr_element(char *arr_name, int position, int value) {
	register int i;
	
	for(i=larrtos-1; i >= call_stack[functos-1]; i--)  {
		if(!strcmp(local_arr_stack[i].arr_name, arr_name)) {
			if(local_arr_stack[i].length <= position) {
				sntx_err(INDEX_OUT_OF_BOUNDS);
			}
			if(local_arr_stack[i].arr_type == INT) {
				int *tmpArrStart = local_arr_stack[i].int_arr;
				tmpArrStart = tmpArrStart + position;
				*tmpArrStart = value;
			} else if(local_arr_stack[i].arr_type == CHAR) {
				char *tmpArrStart = local_arr_stack[i].char_arr;
				tmpArrStart = tmpArrStart + position;
				*tmpArrStart = value;
			} else {
				printf("only arrays of type int or char are allowed. ");
				exit(1);
			}
			return;
		}
	}
	if(i < call_stack[functos-1])
	
		for(i=0; i < NUM_GLOBAL_ARRAYS; i++)
			if(!strcmp(global_arrays[i].arr_name, arr_name)) {
				if(global_arrays[i].length <= position) {
					sntx_err(INDEX_OUT_OF_BOUNDS);
				}
				if(global_arrays[i].arr_type == INT) {
					int *tmpArrStart = global_arrays[i].int_arr;
					tmpArrStart = tmpArrStart + position;
					*tmpArrStart = value;
				} else if(global_arrays[i].arr_type == CHAR) {
					char *tmpArrStart = global_arrays[i].char_arr;
					tmpArrStart = tmpArrStart + position;
					*tmpArrStart = value;
				} else {
					printf("only arrays of type int or char are allowed. ");
					exit(1);
				}
				return;
			}
	sntx_err(NOT_ARR);
}
int is_arr(char *s) {
	register int i;

	for(i=larrtos-1; i >= call_stack[functos-1]; i--)
		if(!strcmp(local_arr_stack[i].arr_name, token)) {
			return 1;
		}

	for(i=0; i < NUM_GLOBAL_ARRAYS; i++)
		if(!strcmp(global_arrays[i].arr_name, s)) {
			return 1;
		}
	return 0;
}

void decl_local(void) {
	struct var_type i;
	get_token();  
	i.v_type = tok;
	i.value = 0; 
	do {
		get_token();
		strcpy(i.var_name, token);
		local_var_push(i);
		get_token();
	} while(*token == ',');
	if(*token != ';') {
		sntx_err(SEMI_EXPECTED);
	}
}

void func_ret(void) {
	int value;
	value = 0;

	eval_exp(&value);
	ret_value = value;
}

void interp_block(void) {
	int value;
	char block = 0;
	char *tempProg;
	do {
		tempProg = prog;
		token_type = get_token();
	
		if(token_type == IDENTIFIER) {
			
			putback(); 
			eval_exp(&value); 
			if(*token!=';') {
				sntx_err(SEMI_EXPECTED);
			}
		} else if(token_type==BLOCK) {
			
			if(*token == '{') { 
				block = 1; 
			} else {
				return;
			} 
		} else 
			switch(tok) {
			case CHAR:
			case INT:   
				get_token(); // name
				get_token(); // [ or smth with local var
				if(token_type == ARRAY) {
					prog = tempProg;
					decl_local_array();
				} else {
					prog = tempProg;
					decl_local();
				}
				break;
			case RETURN:  
				func_ret();
				return;
			case IF:    
				exec_if();
				break;
			case ELSE:   
				find_eob(); 
                                 
				break;
			case WHILE:   
				exec_while();
				break;
			case DO:     
				exec_do();
				break;
			case FOR:   
				exec_for();
				break;
			case END:
				exit(0);
			}
	} while(tok != FINISHED && block);
}

void find_eob(void) {
	int brace;
	get_token();
	brace = 1;
	do {
		get_token();
		if(*token == '{') {
			brace++;
		} else if(*token == '}') {
			brace--;
		}
	} while(brace);
}

void local_arr_push(struct array_type *arr) {
	if(larrtos > NUM_LOCAL_ARRS) {
		sntx_err(TOO_MANY_LARRS);
	}
	local_arr_stack[larrtos] = *arr;
	larrtos++;
}
struct array_type *get_arr(char *name) {
	register int i;

	for(i=larrtos-1; i >= call_stack[functos-1]; i--) {
		if(!strcmp(local_arr_stack[i].arr_name, name)) {
			return &local_arr_stack[i];
		}
	}
	for(i=0; i < NUM_GLOBAL_ARRAYS; i++) {
		if(!strcmp(global_arrays[i].arr_name, name)) {
			return &global_arrays[i];
		}
	}
	return 0;
}

void call(void) {
	char *loc, *temp;
	int lvartemp;
	loc = find_func(token);
	if(loc == NULL) {
		sntx_err(FUNC_UNDEF); 
	} else {
		lvartemp = lvartos;  
		get_args(); 
		temp = prog; 
		func_push(lvartemp); 
                               
		prog = loc; 
		get_params(); 
		interp_block(); 
		prog = temp;
		lvartos = func_pop(); 
	}
}

void get_args(void) {
	int value, countVars, tempVars[NUM_PARAMS]; // in reverse order
	struct var_type i;

	struct array_type *tempArrs[NUM_PARAMS]; // todo: summ of arr and int params

	int countArrs = 0;
	countVars = 0;

	get_token();
	if(*token != '(') {
		sntx_err(PAREN_EXPECTED);
	}

	do {
		char *tempProg = prog;
		get_token();             // name or number
		if(*token == ')') {
			break;
		}
		char *copy = (char *)malloc(strlen(token) + 1);
		strcpy(copy, token);
		if(is_arr(token)) {
			get_token();         // '[' or ','
			if(*token == ',') {  // array is passed by pointer
				tempArrs[countArrs] = get_arr(copy);
				countArrs++;
				continue;        // array was pushed into local array stack
			} else {
				prog = tempProg;
			}
		} else {
			prog = tempProg;
		}
		free(copy);
		eval_exp(&value);
		tempVars[countVars] = value; 
		get_token();
		countVars++;
	} while(*token == ',');

	countVars--;

	for(; countVars >=0; countVars--) {
		i.value = tempVars[countVars];
		i.v_type = ARG;
		local_var_push(i);
	}
	countArrs--;
	for(; countArrs >= 0; countArrs--) {
		local_arr_push(tempArrs[countArrs]);
	}

}

void get_params(void) {
	struct var_type *var;
	int varIdx;
	struct array_type *arr;
	int arrIdx;
	char copyTokenType;
	varIdx = lvartos - 1;
	arrIdx = larrtos - 1;
	do {
		get_token();
		if(*token != ')') {
			if(tok != INT && tok != CHAR) {
				sntx_err(TYPE_EXPECTED);
			}
			copyTokenType = tok;
			get_token(); 
			char *nameCopy = (char *)malloc(strlen(token) + 1);
			strcpy(nameCopy, token);
			if(*token == ')') {
				break;
			}
			get_token();
			if(token_type == ARRAY) {
				arr = &local_arr_stack[arrIdx];
				arr->arr_type = copyTokenType;
				strcpy(arr->arr_name, nameCopy);
				get_token(); // ']'
				get_token(); // ','
				arrIdx--;
			} else {
				var = &local_var_stack[varIdx];
				var->v_type = copyTokenType;
				strcpy(var->var_name, nameCopy);
				varIdx--;
			}
		} else {
			break;
		}
	} while(*token == ',');
	if(*token != ')') {
		sntx_err(PAREN_EXPECTED);
	}
}

void func_push(int i) {
	if(functos>NUM_FUNC) {
		sntx_err(NEST_FUNC);
	}
	call_stack[functos] = i;
	functos++;
}

int func_pop(void) {
	functos--;
	if(functos < 0) {
		sntx_err(RET_NOCALL);
	}
	return call_stack[functos];
}

void local_var_push(struct var_type i) {
	if(lvartos > NUM_LOCAL_VARS) {
		sntx_err(TOO_MANY_LVARS);
	}
	local_var_stack[lvartos] = i;
	lvartos++;
}

char *find_func(char *name) {
	register int i;
	for(i=0; i < func_index; i++)
		if(!strcmp(name, func_table[i].func_name)) {
			return func_table[i].loc;
		}
	return NULL;
}

int load_program(char *p, char *fname) {
	FILE *fp;
	int i=0;
	if((fp=fopen(fname, "rb"))==NULL) {
		return 0;
	}
	i = 0;
	do {
		*p = getc(fp);
		p++;
		i++;
	} while(!feof(fp) && i<PROG_SIZE);
	if(*(p-2) == 0x1a) {
		*(p-2) = '\0';
	} 
	else {
		*(p-1) = '\0';
	}
	fclose(fp);
	return 1;
}

void decl_local_array(void) {
	struct array_type newLocalArr;
	get_token(); 
	int arrtype = tok;
	newLocalArr.arr_type = arrtype;
	get_token();
	strcpy(newLocalArr.arr_name, token);
	get_token(); // [
	if(*token != '[') {
		sntx_err(ARRAY_BRACE_EXPECTED);
	}
	get_token(); 
	if(token_type != NUMBER) {
		sntx_err(NUM_EXPECTED);
	}
	int arr_length = atoi(token);
	newLocalArr.length = arr_length;
	get_token(); // ]
	if(*token != ']') {
		sntx_err(ARRAY_BRACE_EXPECTED);
	}
	if(arrtype == INT) {
		newLocalArr.int_arr = (int *)malloc(sizeof(int) * arr_length);
	} else if(arrtype == CHAR) {
		newLocalArr.char_arr = (char *)malloc(sizeof(char) * arr_length);
	}
	get_token(); // ;
	if(*token != ';') {
		sntx_err(SEMI_EXPECTED);
	}
	if(lvartos > NUM_LOCAL_ARRS) {
		sntx_err(TOO_MANY_LARRS);
	}
	local_arr_stack[larrtos] = newLocalArr;
	larrtos++;
}
void decl_global_array(void) {
	get_token(); // тип
	int arrtype = tok;
	global_arrays[garr_index].arr_type = arrtype;
	get_token(); // имя
	strcpy(global_arrays[garr_index].arr_name, token);
	get_token(); // [
	if(*token != '[') {
		sntx_err(ARRAY_BRACE_EXPECTED);
	}
	get_token();
	if(token_type != NUMBER) {
		sntx_err(NUM_EXPECTED);
	}
	int arr_length = atoi(token);
	get_token(); // ]
	if(*token != ']') {
		sntx_err(ARRAY_BRACE_EXPECTED);
	}
	global_arrays[garr_index].length = arr_length;
	if(arrtype == INT) {
		global_arrays[garr_index].int_arr = (int *)malloc(sizeof(int) * arr_length);
	} else if(arrtype == CHAR) {
		global_arrays[garr_index].char_arr = (char *)malloc(sizeof(char) * arr_length);
	}
	get_token(); // ;
	if(*token != ';') {
		sntx_err(SEMI_EXPECTED);
	}
	garr_index++;
}

void decl_global_var(void) {
	int vartype;
	get_token();  
	vartype = tok; 
	do {
		global_vars[gvar_index].v_type = vartype;
		global_vars[gvar_index].value = 0; 
		get_token(); 
		strcpy(global_vars[gvar_index].var_name, token);
		get_token();
		gvar_index++;
	} while(*token == ',');
	if(*token != ';') {
		sntx_err(SEMI_EXPECTED);
	}
}

int find_arr_element(char *arr_name, int position) {
	register int i;

	for(i=larrtos-1; i >= call_stack[functos-1]; i--) {
		if(!strcmp(local_arr_stack[i].arr_name, token)) {
			if(local_arr_stack[i].length <= position) {
				sntx_err(INDEX_OUT_OF_BOUNDS);
			}
			if(local_arr_stack[i].arr_type == INT) {
				int *tmpArrStart = local_arr_stack[i].int_arr;
				tmpArrStart = tmpArrStart + position;
				return *tmpArrStart;
			} else if(local_arr_stack[i].arr_type == CHAR) {
				char *tmpArrStart = local_arr_stack[i].char_arr;
				tmpArrStart = tmpArrStart + position;
				return *tmpArrStart;
			} else {
				printf("int char");
				exit(1);
			}
		}
	}
	if(i < call_stack[functos-1])
		
		for(i=0; i < NUM_GLOBAL_ARRAYS; i++)
			if(!strcmp(global_arrays[i].arr_name, arr_name)) {
				if(global_arrays[i].length <= position) {
					sntx_err(INDEX_OUT_OF_BOUNDS);
				}
				if(global_arrays[i].arr_type == INT) {
					int *tmpArrStart = global_arrays[i].int_arr;
					tmpArrStart = tmpArrStart + position;
					return *tmpArrStart;
				} else if(global_arrays[i].arr_type == CHAR) {
					char *tmpArrStart = global_arrays[i].char_arr;
					tmpArrStart = tmpArrStart + position;
					return *tmpArrStart;
				} else {
					printf("only arrays of type int or char are allowed. ");
					exit(1);
				}
			}
	sntx_err(NOT_ARR);
	return -1;
}
int arr_exists(char *name) {
	register int i;
	
	for(i=larrtos-1; i >= call_stack[functos-1]; i--) {
		if(!strcmp(local_arr_stack[i].arr_name, name)) {
			return 1;
		}
	}
	for(i=0; i < NUM_GLOBAL_ARRAYS; i++) {
		if(!strcmp(global_arrays[i].arr_name, name)) {
			return 1;
		}
	}
	return 0;
}
void free_arr(void) {
	register int i;
	for(i=0; i < NUM_GLOBAL_ARRAYS; i++) {
		free(global_arrays[i].int_arr);
		free(global_arrays[i].char_arr);
	}
}

int find_var(char *s) {
	register int i;

	for(i=lvartos-1; i >= call_stack[functos-1]; i--)
		if(!strcmp(local_var_stack[i].var_name, token)) {
			return local_var_stack[i].value;
		}
	
	for(i=0; i < NUM_GLOBAL_VARS; i++)
		if(!strcmp(global_vars[i].var_name, s)) {
			return global_vars[i].value;
		}
	sntx_err(NOT_VAR); 
	return -1;
}

void exec_if(void) {
	int cond;
	eval_exp(&cond);
	if(cond) {
		interp_block();
	} else {
	
		find_eob();
		get_token();
		if(tok != ELSE) {
			putback(); 
			return;
		}
		interp_block();
	}
}

void exec_while(void) {
	int cond;
	char *temp;
	putback();
	temp = prog;  
	get_token();
	eval_exp(&cond); 
	if(cond) {
		interp_block();
	}  
                             
	else { 
		find_eob();
		return;
	}
	prog = temp; 
}

void exec_do(void) {
	int cond;
	char *temp;
	putback();
	temp = prog; 
	get_token(); 
	interp_block(); 
	get_token();
	if(tok != WHILE) {
		sntx_err(WHILE_EXPECTED);
	}
	eval_exp(&cond);
	if(cond) {
		prog = temp;
	} 
}

void exec_for(void) {
	int cond;
	char *temp, *temp2;
	int brace ;
	get_token();
	eval_exp(&cond);  
	if(*token != ';') {
		sntx_err(SEMI_EXPECTED);
	}
	prog++; 
	temp = prog;
	for(;;) {
		eval_exp(&cond); 
		if(*token != ';') {
			sntx_err(SEMI_EXPECTED);
		}
		prog++;
		temp2 = prog;
		
		brace = 1;
		while(brace) {
			get_token();
			if(*token == '(') {
				brace++;
			}
			if(*token == ')') {
				brace--;
			}
		}
		if(cond) {
			interp_block();
		}  
		else {  
			find_eob();
			return;
		}
		prog = temp2;
		eval_exp(&cond); 
		prog = temp;  
	}
}
