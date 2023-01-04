#ifndef lint 
static char sccsid[] = "@(#)rpc_scan.c 1.1 86/09/25 (C) 1986 SMI";
#endif
 
/*
 * rpc_scan.c, Scanner for the RPC protocol compiler
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include "rpc_scan.h"
#include "rpc_util.h"

#define commentstart(p)	(*(p) == '/' && *((p) + 1) == '*')
#define commentend(p)	(*(p) == '*' && *((p) + 1) == '/') 

static int pushed = 0;			/* is a token pushed */
static token lasttok;			/* last token, if pushed */
static int scan_print;			/* print out directives? */

/*
 * turn printing on or off
 */
void
scanprint(sw) 
	int sw; 
{
	scan_print = sw;
}


/*
 * scan expecting 1 given token
 */
void
scan(expect,tokp)
	tok_kind expect;
	token *tokp;
{
	get_token(tokp);
	if (tokp->kind != expect) {
		expected1(expect);
	}
}

/*
 * scan expecting 2 given tokens
 */
void
scan2(expect1,expect2,tokp)
	tok_kind expect1;
	tok_kind expect2;
	token *tokp;
{
	get_token(tokp);
	if (tokp->kind != expect1 && tokp->kind != expect2) {
		expected2(expect1,expect2);
	}
}

/*
 * scan expecting 3 given token
 */
void
scan3(expect1,expect2,expect3,tokp)
	tok_kind expect1;
	tok_kind expect2;
	tok_kind expect3;
	token *tokp;
{
	get_token(tokp);
	if (tokp->kind != expect1 && tokp->kind != expect2 
			&& tokp->kind != expect3) {
		expected3(expect1,expect2,expect3);
	}
}


/*
 * scan expecting a constant, possibly symbolic
 */
void
scan_num(tokp)
	token *tokp;
{
	get_token(tokp);
	switch (tokp->kind) {
	case TOK_CONST:	
	case TOK_IDENT:
		break;	
	default:
		error("constant or identifier expected");
	}
}


/*
 * Peek at the next token
 */
void
peek(tokp)
	token *tokp;
{
	get_token(tokp);
	unget_token(tokp);
}


/*
 * Peek at the next token and
 * scan it if it matches what you expect
 */
int
peekscan(expect,tokp)
	tok_kind expect;
	token *tokp;
{
	peek(tokp);
	if (tokp->kind == expect) {
		get_token(tokp);
		return(1);
	}
	return(0);
}



/*
 * Get the next token, printing out any directive that
 * are encountered.
 */
void
get_token(tokp)
	token *tokp;
{

	if (pushed) {
		pushed = 0;
		*tokp = lasttok;
		return;
	}
	for (;;) {	
		if (*where == 0) {
			if (! fgets(curline,MAXLINESIZE,fin)) {
				tokp->kind = TOK_EOF;
				*where = 0;
				return;
			}
			where = curline;
			linenum++;
		} else if (isspace(*where)) {
			whitespace();
		} else if (commentstart(where)) {
			decomment();
		} else if (directive(where)) {
			printdirective(where);
			*where = 0;
		} else {
			break;
		}
	}
	/* 
	 * 'where' is not whitespace, comment, or directive
	 * Must be a token!
	 */
	switch (*where) {
	case ':':	tokp->kind = TOK_COLON; where++; break;	
	case ';':	tokp->kind = TOK_SEMICOLON; where++; break;
	case ',':	tokp->kind = TOK_COMMA; where++; break;
	case '=':	tokp->kind = TOK_EQUAL; where++; break;
	case '*':	tokp->kind = TOK_STAR; where++; break;
	case '[':	tokp->kind = TOK_LBRACKET; where++; break;
	case ']':	tokp->kind = TOK_RBRACKET; where++; break;
	case '{':	tokp->kind = TOK_LBRACE; where++; break;
	case '}':	tokp->kind = TOK_RBRACE; where++; break;
	case '(':	tokp->kind = TOK_LPAREN; where++; break;
	case ')':	tokp->kind = TOK_RPAREN; where++; break;

	case '0': 	 
	case '1': 
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		tokp->kind = TOK_CONST; 
		findconst(&where,&tokp->str);
		break;


	default:
		if ( ! (isalpha(*where) || *where == '_')) {
			char buf[100];
			char *p;

			sprintf(buf,"illegal character in file: ");
			p = buf + strlen(buf);
			if (isprint(*where)) {
				sprintf(p,"%c",*where);	
			} else {
				sprintf(p,"%d",*where);
			}
			error(buf);
		}
		findkind(&where,tokp);
		break;
	}
}



static
unget_token(tokp)
	token *tokp;
{
	lasttok = *tokp;
	pushed = 1;
}


static
findconst(str,val)
	char **str;
	char **val;
{
	char *p;
	int size;

	p = *str;
	do {
		*p++;
	} while (isdigit(*p));
	size = p - *str;
	*val = alloc(size + 1);
	strncpy(*val,*str,size);
	(*val)[size] = 0;
	*str = p;
}



static token symbols[] = {
	{ TOK_UNION,	"union" },
	{ TOK_SWITCH,	"switch" }, 
	{ TOK_CASE,	"case" },
	{ TOK_DEFAULT,	"default" },
	{ TOK_STRUCT,		"struct" },
	{ TOK_TYPEDEF,	"typedef" },
	{ TOK_ENUM,	"enum" },
	{ TOK_ARRAY,	"array" },
	{ TOK_OPAQUE,	"opaque" },
	{ TOK_BOOL,	"bool" },
	{ TOK_VOID,	"void" },
	{ TOK_CHAR,	"char" },
	{ TOK_INT,	"int" },
	{ TOK_UNSIGNED,	"unsigned" },
	{ TOK_SHORT,	"short" },
	{ TOK_LONG,	"long" },
	{ TOK_FLOAT,	"float" },
	{ TOK_DOUBLE,	"double" },
	{ TOK_STRING,	"string" },
	{ TOK_PROGRAM,	"program" },
	{ TOK_VERSION,	"version" },
	{ TOK_EOF,	"??????"},
};


static
findkind(mark,tokp)
	char **mark;
	token *tokp;
{

	int len;
	token *s;
	char *str;

	str = *mark;
	for (s = symbols; s->kind != TOK_EOF; s++) {
		len = strlen(s->str);
		if (strncmp(str,s->str,len) == 0) {
			if (!isalnum(str[len]) && str[len] != '_') {
				tokp->kind = s->kind;
				tokp->str = s->str;
				*mark = str + len;
				return;	
			}
		}
	}
	tokp->kind = TOK_IDENT;
	for (len = 0; isalnum(str[len]) || str[len] == '_'; len++)
		;
	tokp->str = alloc(len+1);
	strncpy(tokp->str,str,len);
	tokp->str[len] = 0;
	*mark = str + len;
}

static
whitespace()
{
	while (isspace(*where))
		where++;
}

static
decomment()
{
	for (where += 2; ! commentend(where) ; where++) {
		if (*where == 0) {
			if (! fgets(curline,MAXLINESIZE,fin)) {
				error("unterminated comment");
			}
			linenum++;
			where = curline - 1;
		}
	}
	where += 2;
}


static
directive(line)
	char *line;
{
	return(line == curline && *line == '#');
}
 
static
printdirective(line)
	char *line;
{
	char *s;

	for (s = line + strlen(line) - 1; s >= line; s--) {
		if (commentend(s)) {
			break;	
		} else if (commentstart(s)) {
			where = s;
			*where++ = '\n';
			*where = 0;	
			if (scan_print) {
				fprintf(fout,line);
			}
			decomment();
			return;
		} 
	}
	if (scan_print) {
		fprintf(fout,line);
	}
}				
