#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "scanner.h"
#include "strings.h"
#include "typedef.h"

char	*decltbl[] = {
	"int",
	"char",
	"struct",
	"register", 
	"short",
	"long",
	"unsigned",
	"extern",
	"static",
	"union",
	"enum",
	"float",
	"double",
	"void",
	"auto",
	NULL
};

static	Boolean	more;
char	*cmdname;

static	FILE	*input();
static	FILE	*output();

main(argc, argv)
int	argc;
char	*argv[];
{
	int	func_lineno;
	FILE	*infp;

	cmdname = argv[0];
	argc--;
	argv++;
	if (argc != 3) {
		fprintf(stderr, "Usage: %s cppfile srcfile outfile\n", cmdname);
		exit(1);
	}
	infp = input(argv[0]);
	original_file(argv[1]);
	dotd(argv[1]);
	output(argv[2], argv[1]);

	bb_decls();
	optional_global_decls(infp);
	while (more) {
		func_lineno = lineno();
		findtok(LCURLY, infp);
		function(infp, func_lineno);
		optional_global_decls(infp);
	}
	pr_bb_routine();
}

/*
 * Look for optional global declarations.
 * These are the same as local declarations
 * except lines like
 *	funcname();
 * with no type specifiers are allowed.
 */
optional_global_decls(infp)
FILE	*infp;
{
	Boolean	infunc;
	Token	tok;

	for (;;) {
		infunc = optional_decls(infp);
		if (!infunc) {
			/*
			 * Looking for:
			 *	funcname();
			 */
			tok = gettoken(infp);
			if (tok != ID) {
				prevtok(tok);
				break;
			}
			prtok();
			tok = gettoken(infp);
			if (tok != LPAREN) {
				prevtok(tok);
				break;
			}
			prtok();
			tok = gettoken(infp);
			if (tok != RPAREN) {
				prevtok(tok);
				break;
			}
			prtok();
			tok = gettoken(infp);
			if (tok != SEMI) {
				prevtok(tok);
				break;
			}
			prtok();
		}
	}
}

/*
 * optional_decls - there may or may not be some declarations here.
 */
Boolean	
optional_decls(infp)
FILE	*infp;
{
	Token	tok;
	char	*id;
	Boolean	indecls;
	Boolean	infunc;
	Boolean	cont;

	indecls = True;
	infunc = False;
	while (indecls) {
		tok = gettoken(infp);
		id = getident();
		if (is_type(id)) {
			prtok();
			cont = True;
			do {
				tok = gettoken(infp);
				prtok();
				switch(tok) {
				case LPAREN:
					if (is_func(infp)) {
						indecls = False;
						infunc = True;
					} else {
						findtok(SEMI, infp);
					}
					break;
	
				case LCURLY:
					matching(LCURLY, RCURLY, infp);
					findtok(SEMI, infp);
					break;
				
				case COMMA:
				case LBRACKET:
				case EQUALS:
					findtok(SEMI, infp);
					break;
	
				case SEMI:
					break;
		
				case EOFT:
					more = False;
					return(False);
				default:
					continue;
				}
				cont = False;
			} while(cont);
		} else if (strcmp(id, "typedef") == 0) {
			prtok();
			add_typedef(infp);
		} else {
			if (tok == EOFT) {
				more = False;
				return(False);
			}
			prevtok(tok);
			indecls = False;
		}
	}
	more = True;
	return(infunc);
}

/*
 * is_func - is this a function definition
 * This is some of the trickiest parts.
 * This distinction between definitions and declarations
 * can be subtle.
 * At this point we have seen a left parenthesis in a declaration.
 * Find the matching paren and use one token look ahead to
 * figure out what is going on.
 */
static Boolean
is_func(infp)
FILE	*infp;
{
	Token	tok;

	matching(LPAREN, RPAREN, infp);
	tok = gettoken(infp);
	prevtok(tok);
	switch(tok) {
	case COMMA: 			/* int func(), ... ; */
		return(False);

	case SEMI:			/* int func(); */
		return(False);

	case LCURLY:			/* int func() { */
		return(True);

	case ID:			/* int func(arg) int arg; { */
		return(True);
	
	case LPAREN:			/* int (*signal())(); */
		tok = gettoken(infp);
		prtok();
		return(is_func(infp));

	case EQUALS:			/* int (*statf)() = func; */
		return(False);

	default:
		error("is_func is confused %d %c %s\n", tok, tok, getident());
	}
	/* NOTREACHED */
}

/*
 * function - process a function
 */
static
function(infp, func_lineno)
FILE	*infp;
int	func_lineno;
{
	optional_decls(infp);
	prinit();
	basic_block();
	pr_basic_block(func_lineno);
	statement_list(infp);
}

/*
 * findtok - search for a specific token
 */
findtok(token, infp)
Token	token;
FILE	*infp;
{
	Token	tok;

	do {
		tok = gettoken(infp);
		if (tok == EOFT) {
			error("EOF encountered");
		}
		prtok();
	} while (token != tok);
}

/*
 * is_type_decl - is this a type declaration
 */
static Boolean
is_type(str)
char	*str;
{
	char	**sp;
	struct	type_def *td;
	
	for (sp = decltbl; *sp != NULL; sp++) {
		if (strcmp(str, *sp) == 0) {
			return(True);
		}
	}
	for (td = typedef_hdr; td != NULL; td = td->next) {
		if (strcmp(str, td->type_name) == 0) {
			return(True);
		}
	}
	return(False);
}

/*
 * add_typedef - add a typedef to the list.
 * The name of the typedef is the last ID token before the 
 * semicolon.
 */
static
add_typedef(infp)
FILE	*infp;
{
	Token	tok;
	char	name[100];
	struct	type_def *td;

	tok = gettoken(infp);
	prtok();
	while (tok != SEMI) {
		if (tok == LCURLY) {
			matching(LCURLY, RCURLY, infp);
		} else if (tok == ID) {
			strcpy(name, getident());
		}
		tok = gettoken(infp);
		prtok();
	}
	td = (struct type_def *) malloc(sizeof(struct type_def));
	td->type_name = strdup(name);
	td->next = typedef_hdr;
	typedef_hdr = td;
}

/*
 * input - open the input file
 */
static	FILE *
input(name)
char	*name;
{
	FILE	*fp;

	fp = fopen(name, "r");
	if (fp == NULL) {
		error("Cannot open %s", name);
	}
	return(fp);
}

/*
 * output - open the output file
 */
static	FILE *
output(name, srcfile)
char	*name;
char	*srcfile;
{
	FILE	*fp;

	fp = freopen(name, "w", stdout);
	if (fp == NULL) {
		error("Cannot open %s", name);
	}
	printf("# 1 \"%s\"\n", srcfile);
	return(fp);
}
