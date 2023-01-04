#ifndef lint
static	char sccsid[] = "@(#)bug.c 1.1 86/09/25 SMI"; /* from UCB */
#endif
typedef struct  exprnode {
	union {
		short value;
		char *symb;
	} val;
	char	isleaf;
	char	issymb;
	EXPR *lbra;
	EXPR *rbra;
} EXPR

typedef struct instrline {
	short tok;
	short bitskel;
	short arg1;
	union {
		short reg;
		EXPR *expr;
	} arg2;
} INST
