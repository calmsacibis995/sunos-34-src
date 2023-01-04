#ifndef lint 
static char sccsid[] = "@(#)rpc_util.c 1.1 86/09/25 (C) 1986 SMI";
#endif
 
/*
 * rpc_util.c, Utility routines for the RPC protocol compiler
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */
#include <stdio.h>
#include "rpc_scan.h"
#include "rpc_util.h"

char curline[MAXLINESIZE];		/* current read line */
char *where = curline;			/* current point in line */
int linenum = 0;				/* current line number */

char *infile;	/* input filename */
char *outfile;  /* output filename */
char *outfile2; /* possible other output filename */

FILE *fout;		/* file pointer of current output */
FILE *fin;		/* file pointer of current input */

list *printed;	/* list of printed routines */
list *defined;  /* list of defined things */

/*
 * Reinitialize the world
 */
reinitialize()
{
	bzero(curline,MAXLINESIZE);
	where = curline;
	linenum = 0;
	printed = NULL;
	defined = NULL;
}

/*
 * string equality
 */
streq(a,b)
	char *a;
	char *b;
{
	return(strcmp(a,b) == 0);
}

/*
 * find a value in a list
 */
char *
findval(lst,val,cmp)
	list *lst;
	char *val;
	int (*cmp)();
{
	for (; lst != NULL; lst = lst->next) {
		if ((*cmp)(lst->val,val)) {
			return(lst->val);
		}
	}    
	return(NULL);
}

/*
 * store a value in a list
 */
void
storeval(lstp,val)
	list **lstp;
	char *val;
{
	list **l;
	list *lst;

	for (l = lstp; *l != NULL; l = (list **) &(*l)->next) 
		;
	lst = ALLOC(list);
	lst->val = val;
	lst->next = NULL;
	*l = lst;
}


/*
 * print a useful (?) error message, and then die
 */
void
error(msg)
	char *msg;
{
	extern char *outfile;

	printwhere();
	fprintf(stderr,"%s, line %d: ",infile ? infile : "<stdin>", linenum);
	fprintf(stderr,"%s\n",msg);
	crash();
}

/*
 * Something went wrong, unlink any files
 * that we may have created and then die.
 */
crash()
{
	if (outfile) {
		unlink(outfile);
	}	
	if (outfile2) {
		unlink(outfile2);
	}
	exit(1);
}



static char expectbuf[100];
static char *toktostr();

/*
 * error, token encountered was not the expected one
 */
void
expected1(exp1)
	tok_kind exp1;
{
	sprintf(expectbuf,"expected '%s'",
		toktostr(exp1));
	error(expectbuf);
}

/*
 * error, token encountered was not one of two expected ones
 */
void
expected2(exp1,exp2)
	tok_kind exp1,exp2;
{
	sprintf(expectbuf,"expected '%s' or '%s'",
		toktostr(exp1),
		toktostr(exp2));
	error(expectbuf);
}

/*
 * error, token encountered was not one of 3 expected ones
 */
void
expected3(exp1,exp2,exp3)
	tok_kind exp1,exp2,exp3;
{
	sprintf(expectbuf,"expected '%s', '%s' or '%s'",
		toktostr(exp1),
		toktostr(exp2),
		toktostr(exp3));
	error(expectbuf);
}



static token tokstrings[] = {
	{ TOK_IDENT, "identifier" },
	{ TOK_CONST, "constant" },
	{ TOK_RPAREN, ")" },
	{ TOK_LPAREN, "(" },
	{ TOK_RBRACE, "}" },
	{ TOK_LBRACE, "{" },
	{ TOK_LBRACKET, "[" },
	{ TOK_RBRACKET, "]" },
	{ TOK_STAR, "*" },
	{ TOK_COMMA, "," },
	{ TOK_EQUAL, "=" },
	{ TOK_COLON, ":" },
	{ TOK_SEMICOLON, ";" },
	{ TOK_UNION, "union" },
	{ TOK_STRUCT, "struct" },
	{ TOK_SWITCH, "switch" },
	{ TOK_CASE,	"case" },
	{ TOK_DEFAULT, "default" },
	{ TOK_ENUM, "enum" },
	{ TOK_ARRAY, "array" },
	{ TOK_TYPEDEF, "typedef" },
	{ TOK_INT, "int" },
	{ TOK_SHORT, "short" },
	{ TOK_LONG, "long" },
	{ TOK_UNSIGNED, "unsigned" },
	{ TOK_DOUBLE, "double" },
	{ TOK_FLOAT, "float" },
	{ TOK_CHAR, "char" },
	{ TOK_STRING, "string" },
	{ TOK_OPAQUE, "opaque" },
	{ TOK_BOOL, "bool" },
	{ TOK_VOID, "void" },
	{ TOK_PROGRAM, "program" },
	{ TOK_VERSION, "version" },
	{ TOK_EOF, "??????" }
};
 
static char * 
toktostr(kind) 
	tok_kind kind; 
{ 
	token *sp;

	for (sp = tokstrings; sp->kind != TOK_EOF && sp->kind != kind; sp++)
		; 
	return(sp->str); 
}


static 
printbuf()
{
	char c;
	int i;
	int cnt;

#	define TABSIZE 4

	for (i = 0; c = curline[i]; i++) {
		if (c == '\t') {
			cnt = 8 - (i % TABSIZE);
			c = ' ';
		} else {
			cnt = 1;
		}
		while (cnt--) {	
			putc(c,stderr);
		} 
	}
}


static
printwhere()
{
	int i;
	char c;
	int cnt;

	printbuf();
	for (i = 0; i < where - curline; i++) {
		c = curline[i];
		if (c == '\t') {
			cnt = 8 - (i % TABSIZE);
		} else {
			cnt = 1;
		}
		while (cnt--) {
			putc('^',stderr);
		}
	}
	putc('\n',stderr);
}
