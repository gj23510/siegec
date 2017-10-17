#include "externVars.h"
#include "commonEnums.h"
#include "analyzer.h"
#include "libc.h"
#include <stdio.h>
#include <stdlib.h>

int call_put_char() {
	int value;
	eval_exp(&value);
	printf("%c", value);
	return value;
}

int call_put_string(void) {
	get_token();
	if(*token!='(') {
		sntx_err(PAREN_EXPECTED);
	}
	get_token();
	if(token_type!=STRING) {
		sntx_err(QUOTE_EXPECTED);
	}
	puts(token);
	get_token();
	if(*token!=')') {
		sntx_err(PAREN_EXPECTED);
	}
	get_token();
	if(*token!=';') {
		sntx_err(SEMI_EXPECTED);
	}
	putback();
	return 0;
}

int call_getche() {
	char ch;
	ch = getchar();
	while(*prog!=')') {
		prog++;
	}
	prog++;  
	return ch;
}

int getnum(void) {
	char s[80];
	gets(s);
	while(*prog != ')') {
		prog++;
	}
	prog++; 
	return atoi(s);
}

int print(char *s) {
	int i;
	get_token();
	if(*token!='(') {
		sntx_err(PAREN_EXPECTED);
	}
	get_token();
	if(token_type==STRING) { 
		printf("%s ", token);
	} else {
		putback();
		eval_exp(&i);
		printf("%d ", i);
	}
	get_token();
	if(*token!=')') {
		sntx_err(PAREN_EXPECTED);
	}
	get_token();
	if(*token!=';') {
		sntx_err(SEMI_EXPECTED);
	}
	putback();
	return 0;
}
