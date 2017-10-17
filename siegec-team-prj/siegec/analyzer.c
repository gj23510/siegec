
#include "externVars.h"
#include "commonEnums.h"
#include "restrictions.h"
#include "analyzer.h"
#include "interpreter.h"
#include "libc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct intern_func_type {
	const char *f_name;
	int (*p)(void);  
} intern_func[] = {
	{"print", (int ( *)(void))print},
	{"getnum", (int ( *)(void))getnum},
	{"getche", (int ( *)(void))call_getche},
	{"putch", (int ( *)(void))call_put_char},
	{"puts", (int ( *)(void))call_put_string},
	{"", 0}  
};

void eval_exp(int *value) {
	get_token();
	if(!*token) {
		sntx_err(NO_EXP);
		return;
	}
	if(*token == ';') {
		*value = 0;
		return;
	}
	eval_exp0(value);
	putback(); 
}

void eval_exp0(int *value) {
	char temp[ID_LEN]; 
	register int temp_tok;
	if(token_type == IDENTIFIER) {
		if(is_var(token)) {
		
			if(is_arr(token)) {
				sntx_err(SYNTAX); // todo: make message: redefinition of array is illegal
			}
			strcpy(temp, token);
			temp_tok = token_type;
			get_token();
			if(*token == '=') { 
				get_token();
				eval_exp0(value);  
				assign_var(temp, *value); 
				return;
			} else {
				putback();  
				strcpy(token, temp);
				token_type = temp_tok;
			}
		} else if(is_arr(token)) {
			strcpy(temp, token);
			temp_tok = token_type;
			get_token();
			if(*token == '[') {
				get_token();
				eval_exp0(value); 
				int arr_index = *value;
				get_token(); // '=' or ';'
				if(*token == '=') {  
					get_token();
					eval_exp0(value); 
					assign_arr_element(temp, arr_index, *value);
					return;
				} else { 
					putback();  
					strcpy(token, temp);
					token_type = temp_tok;
				}
			} else {
				sntx_err(ASSIGN_ARRAY_ILLEGAL);
			}
		}
	}
	eval_exp1(value);
}

void eval_exp1(int *value) {
	int partial_value;
	register char op;
	char relops[7] = {
		LT, LE, GT, GE, EQ, NE, 0
	};
	eval_exp2(value);
	op = *token;
	if(strchr(relops, op)) {
		get_token();
		eval_exp2(&partial_value);
		switch(op) {  
		case LT:
			*value = *value < partial_value;
			break;
		case LE:
			*value = *value <= partial_value;
			break;
		case GT:
			*value = *value > partial_value;
			break;
		case GE:
			*value = *value >= partial_value;
			break;
		case EQ:
			*value = *value == partial_value;
			break;
		case NE:
			*value = *value != partial_value;
			break;
		}
	}
}

void eval_exp2(int *value) {
	register char  op;
	int partial_value;
	eval_exp3(value);
	while((op = *token) == '+' || op == '-') {
		get_token();
		eval_exp3(&partial_value);
		switch(op) { 
		case '-':
			*value = *value - partial_value;
			break;
		case '+':
			*value = *value + partial_value;
			break;
		}
	}
}

void eval_exp3(int *value) {
	register char  op;
	int partial_value;// t;
	eval_exp4(value);
	while((op = *token) == '*' || op == '/' || op == '%') {
		get_token();
		eval_exp4(&partial_value);
		switch(op) {
		case '*':
			*value = *value * partial_value;
			break;
		case '/':
			if(partial_value == 0) {
				sntx_err(DIV_BY_ZERO);
			}
			*value = (*value) / partial_value;
			break;
		case '%':
			*value = (*value) % partial_value;
			break;
		}
	}
}

void eval_exp4(int *value) {
	register char  op;
	op = '\0';
	if(*token == '+' || *token == '-') {
		op = *token;
		get_token();
	}
	eval_exp5(value);
	if(op) {
		if(op == '-') {
			*value = -(*value);
		}
	}
}

void eval_exp5(int *value) {
	if(*token == '(') {
		get_token();
		eval_exp0(value);  
		if(*token != ')') {
			sntx_err(PAREN_EXPECTED);
		}
		get_token();
	} else {
		atom(value);
	}
}

void atom(int *value) {
	int i;
	switch(token_type) {
	case IDENTIFIER:
		i = internal_func(token);
		if(i!= -1) {  
			*value = (*intern_func[i].p)();
		} else if(find_func(token)) {
		
			call();
			*value = ret_value;
		} else if(arr_exists(token)) {
			*value = find_arr_element(token, *value);
		} else {
			*value = find_var(token); 
		}
		get_token();
		return;
	case NUMBER: 
		*value = atoi(token);
		get_token();
		return;
	case DELIMITER:
		if(*token == '\'') {
			*value = *prog;
			prog++;
			if(*prog!='\'') {
				sntx_err(QUOTE_EXPECTED);
			}
			prog++;
			get_token();
			return ;
		}
		if(*token==')') {
			return;  
		} else {
			sntx_err(SYNTAX); 
		}
	default:
		sntx_err(SYNTAX); 
	}
}

int internal_func(char *s) {
	int i;
	for(i=0; intern_func[i].f_name[0]; i++) {
		if(!strcmp(intern_func[i].f_name, s)) {
			return i;
		}
	}
	return -1;
}

void putback(void) {
	char *t;
	t = token;
	for(; *t; t++) {
		prog--;
	}
}

int get_token(void) {
	register char *temp;
	token_type = 0;
	tok = 0;
	temp = token;
	*temp = '\0';
	
	while(iswhite(*prog) && *prog) {
		++prog;
	}
	while(*prog == '\r' || *prog == '\n') {
		++prog;
		// COMPILER_SPECIFIC
		while(iswhite(*prog) && *prog) {
			++prog;
		}
	}

	if(*prog == '\0') {
		*token = '\0';
		tok = FINISHED;
		return (token_type = DELIMITER);
	}

	if(strchr("{}", *prog)) { 
		*temp = *prog;
		temp++;
		*temp = '\0';
		prog++;
		return (token_type = BLOCK);
	}

	if(strchr("[]", *prog)) {
		*temp = *prog;
		temp++;
		*temp = '\0';
		prog++;
		return (token_type = ARRAY);
	}

	if(*prog == '/') {
		if(*(prog+1) == '*') {
			prog += 2;
			do {
				while(*prog != '*') {
					prog++;
				}
				prog++;
			} while(*prog != '/');
			prog++;
		}
	}

	if(strchr("!<>=", *prog)) {
		switch(*prog) {
		case '=':
			if(*(prog+1) == '=') { // ==
				prog++;
				prog++;
				*temp = EQ;
				temp++;
				*temp = EQ;
				temp++;
				*temp = '\0';
			}
			break;
		case '!':
			if(*(prog+1) == '=') { // !=
				prog++;
				prog++;
				*temp = NE;
				temp++;
				*temp = NE;
				temp++;
				*temp = '\0';
			}
			break;
		case '<':
			if(*(prog+1) == '=') { // <=
				prog++;
				prog++;
				*temp = LE;
				temp++;
				*temp = LE;
			} else {               // <
				prog++;
				*temp = LT;
			}
			temp++;
			*temp = '\0';
			break;
		case '>':
			if(*(prog+1) == '=') { // >=
				prog++;
				prog++;
				*temp = GE;
				temp++;
				*temp = GE;
			} else {              // >
				prog++;
				*temp = GT;
			}
			temp++;
			*temp = '\0';
			break;
		}
		if(*token) {
			return(token_type = DELIMITER);
		}
	}

	if(strchr("+-*^/%=;(),'", *prog)) {
		*temp = *prog;
		prog++;
		temp++;
		*temp = '\0';
		return (token_type = DELIMITER);
	}

	if(*prog=='"') {
		prog++;
		while(*prog != '"' && *prog != '\r') {
			*temp++ = *prog++;
		}
		if(*prog == '\r') {
			sntx_err(SYNTAX);
		}
		prog++;
		*temp = '\0';
		return (token_type = STRING);
	}

	if(isdigit(*prog)) {
		while(!isdelim(*prog)) {
			*temp++ = *prog++;
		}
		*temp = '\0';
		return (token_type = NUMBER);
	}

	if(isalpha(*prog)) {
		while(!isdelim(*prog)) {
			*temp++ = *prog++;
		}
		token_type = TEMP;
	}
	*temp = '\0';

	if(token_type==TEMP) {
		tok = look_up(token); 
		if(tok) {
			token_type = KEYWORD; 
		} else {
			token_type = IDENTIFIER;
		}
	}
	return token_type;
}

void sntx_err(int error) {
	char *p, *temp;
	int linecount = 0;
	register int i;
	static const char *e[]= {
		"syntax error",
		"Unbalanced parenthesis",
		"poker face",
		"Expect an equal sign",
		"invariant",
		" Invalid argument",
		"Semicolon expected",
		"unbalanced braces",
		"Function not defined",
		"The expected type qualifier",
		"Invest too much of a function call",
		"Return Statement function",
		"Expected bracket",
		"Expected while",
		"Expected close quotation",
		"Anthony ",
		"Too many local variables",
		"divide by zero",
		"Array assignment errors",
		"array index is out of the border",
		"no quality",
		"Expected array brackets [ or ]",
		"Precount",
		"Many of the local Array"
	};
	printf("\n%s", e[error]);
	p = p_buf;
	while(p != prog) { 
		p++;
		if(*p == '\r' || *p == '\n') {
			linecount++;
		}
	}
	printf(" in line %d\n", linecount);
	temp = p;
	for(i=0; i < 20 && p > p_buf && *p != '\n'; i++, p--);
	for(i=0; i < 30 && p <= temp; i++, p++) {
		printf("%c", *p);
	}
	longjmp(e_buf, 1);
}

int isdelim(char c) {
	if(strchr(" !;,+-<>'/*%^=()[]", c) || c == 9 ||
	   c == '\r' || c == 0) {
		return 1;
	}
	return 0;
}

int iswhite(char c) {
	if(c == ' ' || c == '\t') {
		return 1;
	} else {
		return 0;
	}
}

int look_up(char *s) {
	register int i;
	char *p;

	p = s;
	while(*p) {
		*p = tolower(*p);
		p++;
	}

	for(i=0; *table[i].command; i++) {
		if(!strcmp(table[i].command, s)) {
			return table[i].tok;
		}
	}
	return 0;
}
