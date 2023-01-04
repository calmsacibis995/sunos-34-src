#ifndef lint
static	char sccsid[] = "@(#)scanner.c 1.4 84/12/22 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include "scanner.h"

static	int	peekc;		/* one character look ahead */
static 	Token	ptok = EMPTY;	/* one token look ahead */
static	int	linenum = 1;	/* current line number */
static	char	white_space[100];/* white space buffer - makes output nicer */
static	char	ident[500];	/* buffer for identifiers */
static	char	orig_file[500];	/* original source file */
static	char	srcfile[500];	/* current file */
static	char	*cp;		/* pointer within ident[] */
static	Boolean	good_file;	/* flag for when in the correct src file */

/*
 * Scanner 
 * This is your basic adhoc scanner.
 * It makes a few assumptions and takes a few liberties
 * because we are really only interested in the control flow
 * of a program and not the lowest level details.
 */
Token
gettoken(infp)
FILE	*infp;
{
	int	c;
	Token	tok;

	if (ptok != EMPTY) {
		tok = ptok;
		ptok = EMPTY;
		return(tok);
	}
	cp = ident;
	c = skipspace(infp);
	if (isalpha(c) || c == '_') {
		getid(c, infp);
		return(ID);
	}
	if (isdigit(c)) {
		getnum(infp);
		return(NUM);
	}
	switch(c) {
	case '/':
		c = nextc(infp);
		if (c == '*') {
			comment(infp);
			return(gettoken(infp));
		}
		if (c == '=') {
			return(ASGDIV);
		}
		putback(c);
		return(DIV);

	case '+':
		c = nextc(infp);
		if (c == '+') {
			return(AUTOINC);
		}
		if (c == '=') {
			return(ASGADD);
		}
		putback(c);
		return(ADD);

	case '-':
		c = nextc(infp);
		if (c == '-') {
			return(AUTODEC);
		}
		if (c == '=') {
			return(ASGSUB);
		}
		putback(c);
		return(SUB);

	case '*':
		c = nextc(infp);
		if (c == '=') {
			return(ASGMUL);
		}
		putback(c);
		return(MUL);

	case '%':
		c = nextc(infp);
		if (c == '=') {
			return(ASGMOD);
		}
		putback(c);
		return(MOD);

	case '<':
		c = nextc(infp);
		if (c == '<') {
			c = nextc(infp);
			if (c == '=') {
				return(ASGLS);
			}
			putback(c);
			return(LS);
		}
		putback(c);
		return(LT);

	case '>':
		c = nextc(infp);
		if (c == '>') {
			c = nextc(infp);
			if (c == '=') {
				return(ASGRS);
			}
			putback(c);
			return(RS);
		}
		putback(c);
		return(GE);

	case '=':
		c = nextc(infp);
		if (c == '=') {
			return(EQ);
		}
		putback(c);
		return(EQUALS);

	case '!':
		c = nextc(infp);
		if (c == '=') {
			return(NE);
		}
		putback(c);
		return(NOT);
	
	case '|':
		c = nextc(infp);
		if (c == '|') {
			return(LOGOR);
		}
		if (c == '=') {
			return(ASGOR);
		}
		putback(c);
		return(BITOR);

	case '&':
		c = nextc(infp);
		if (c == '&') {
			return(LOGAND);
		}
		if (c == '=') {
			return(ASGAND);
		}
		putback(c);
		return(BITAND);

	case '^':
		c = nextc(infp);
		if (c == '=') {
			return(ASGXOR);
		}
		putback(c);
		return(BITXOR);

	case '"':
	case '\'':
		quotes(c, infp);
		return(STRING);

	case '\n':
		linenum++;
		return((Token) c);

	case '#':
		newfile(infp);
		return(gettoken(infp));

	case EOF:
		printf("%s", white_space);
		return(EOFT);

	default:
		return((Token) c);
	}
}

/*
 * nextc - get the next character
 */
static
nextc(infp)
FILE	*infp;
{
	int	c;

	if (peekc) {
		c = peekc;
		peekc = 0;
		return(c);
	}
	c = getc(infp);
	*cp++ = c;
	*cp = EOS;
	return(c);
}
		
/*
 * skipspace - skip white space
 */
static
skipspace(infp)
FILE 	*infp;
{
	int	c;
	char	*ws;
	
	ws = white_space;
	do {
		if (ws >= &white_space[sizeof(white_space) - 1]) {
			*ws = EOS;
			printf("%s", white_space);
			ws = white_space;
		}
		c = nextc(infp);
		*ws++ = c;
		if (c == '\n') {
			linenum++;
		}
	} while (isspace(c));
	*--ws = EOS;
	cp = ident;
	*cp++ = c;
	*cp = EOS;
	return(c);
}

/*
 * getid - get an identifier
 */
static
getid(firstc, infp)
int	firstc;
FILE	*infp;
{
	int	c;
	
	c = firstc;
	cp = ident;
	*cp++ = c;
	do {
		c = nextc(infp);
	} while (isalnum(c) || c == '_');
	putback(c);
}

/*
 * getnum - scan past a constant
 * The tricky part involves floating point constants.
 */
static
getnum(infp)
FILE	*infp;
{
	int	c;

	c = getint(infp); 
	if (c == '.') {
		c = getint(infp); 
	}
	if (c == 'e' || c == 'E') {
		c = getint(infp); 
	}
	putback(c);
}

/*
 * getint - get an integer constant
 */
static
getint(infp)
FILE	*infp;
{
	int	c;
	
	do {
		c = nextc(infp);
	} while(isdigit(c));
	return(c);
}

/*
 * comment - scan past a comment
 */
static
comment(infp)
FILE	*infp;
{
	int	c;

	c = nextc(infp);
	while (c != EOF) {
		if (c == '*') {
			c = nextc(infp);
			if (c == '/') {
				return;
			}
			putback(c);
		}
		c = nextc(infp);
	}
}

/*
 * quotes - skip over string and character constants
 */
static
quotes(quot, infp)
int	quot;
FILE	*infp;
{
	int	c;

	do {
		c = nextc(infp);
		if (c == '\\') {
			c = nextc(infp);
			c = nextc(infp);
		}
	} while (c != quot);
}

/*
 * getident - get the latest identifier
 */
char *
getident()
{
	return(ident);
}

/*
 * putback - put one character back
 */
static
putback(c)
int	c;
{
	if (peekc) {
		error("Attempt to puch back a second character\n");
	}
	peekc = c;
	*--cp = EOS;
}

/*
 * prevtok - one token push back
 */
prevtok(tok)
Token	tok;
{
	if (ptok != EMPTY) {
		error("Attempt to push back a second token\n");
	}
	ptok = tok;
}

/*
 * prtok - print the current token
 */
prtok()
{
	printf("%s%s", white_space, ident);
}

/*
 * ws - return the current white space
 */
char	*
ws()
{
	return(white_space);
}

/*
 * lineno - return the current line number
 */
lineno()
{
	return(linenum);
}

/*
 * filename - return the current file name
 */
char *
filename()
{
	return(srcfile);
}

/*
 * newfile - a '# lineno "filename"' line has been encountered.
 * Keep a list of each file that has been opened.
 */
newfile(infp)
FILE	*infp;
{
	char	*qp;
	char	line[512];
	char	fname[100];

	fgets(line, sizeof(line), infp);
	printf("%s", white_space);
	printf("# %s", line);
	linenum--;
	sscanf(line, "%d %s", &linenum, fname);
	strcpy(srcfile, &fname[1]);
	qp = rindex(srcfile, '"');
	*qp = EOS;
	if (strcmp(srcfile, orig_file) == 0) {
		good_file = True;
	} else {
		good_file = False;
	}
}

/*
 * original_file
 */
original_file(name)
char	*name;
{
	strcpy(orig_file, name);
}

/* 
 * is_src_ok - are we currently in the right source file
 */
Boolean
is_src_ok()
{
	return(good_file);
}
