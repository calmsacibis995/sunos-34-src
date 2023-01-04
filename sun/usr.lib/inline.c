#ifndef lint
static char sccsid[] = "@(#)inline.c 1.1 86/09/25 SMI";
#endif

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 *  inline [-w] [-v] [sourcefile] [-o outputfile] [-i inlinefile] ...
 *
 *	This program is little more than a glorified sed script.
 *	It inline-expands call instructions in one or more <sourcefiles>
 *	from one or more <inlinefiles>.  If no <inlinefiles> are specified,
 *	the <sourcefiles> are simply concatenated to stdout.  If no
 *	<sourcefiles> are specified, the default input is stdin.
 *
 *	If [-w] is specified, inline displays warnings for duplicate
 *	definitions.
 *
 *	If [-v] is specified, inline displays names of routines that
 *	were actually inline-expanded in the sourcefile.
 *
 *	Each <inlinefile> contains one or more labeled assembly language
 *	fragments of the form:
 *
 *		.inline <name>,<argsize>
 *		...
 *		instructions
 *		...
 *		.end
 *
 *	where the instructions constitute an inline expansion of
 *	the named routine, and <argsize> denotes the number of bytes of
 *	arguments expected. The routine must follow the following rules:
 *
 *	1. Registers a0/a1/d0/d1 may be used freely.
 *	2. Other registers must be saved on entry and restored on exit.
 *	3. Results are returned in d0 or d0/d1.
 *	4. The routine must delete <argsize> bytes from the stack.
 *
 *	Rules 1-3 are from the Sun 68000 calling sequence.  Rule 4 exists
 *	so that inline can compensate (crudely) for the use of autoincrement
 *	addressing when using incoming arguments.
 *
 *	Warning: inline does not check for violations of the above
 *	rules.
 */

#define MAXARGS 100

char   *filename;
int     lineno;
int     nerrors;
char	*sourcenames[MAXARGS];
int	nsources = 0;
int	verbose = 0;
int	warnings = 0;

struct pats {
    char   *name;
    char   *replace;
    short  namelen;
    short  used;
    int    argsize;
    struct pats *left, *right;
};

/*
 * DUP is an error-recovery kludge for duplicate
 * routine defintions.  The second and subsequent
 * occurrences of of an inline routine are ignored.
 */
static struct pats duplicate;
#define DUP &duplicate

struct pats *ptab = NULL;

char   *callops[] = {
    "jbsr",
    "jsr",
    NULL
};

#define malloc malloc_chk
char *malloc();
extern int errno;
extern char *sys_errlist[];

/*VARARGS*/
extern int fprintf();

/*VARARGS1*/
cmderr(s, x1, x2, x3, x4)
    char *s;
{
    (void)fprintf(stderr, "inline: ");
    (void)fprintf(stderr, s, x1, x2, x3, x4);
    nerrors++;
}

/*VARARGS1*/
error(s, x1, x2, x3, x4)
    char *s;
{
    (void)fprintf(stderr, "%s, line %d: ", filename, lineno);
    (void)fprintf(stderr, s, x1, x2, x3, x4);
    nerrors++;
}

/*VARARGS1*/
werror(s, x1, x2, x3, x4)
    char *s;
{
    if (warnings) {
	(void)fprintf(stderr, "%s, line %d: ", filename, lineno);
	(void)fprintf(stderr, s, x1, x2, x3, x4);
    }
}

/*
 * add pattern to the set of expandable routines.
 * Return a pointer to the new pattern descriptor.
 * If a duplicate name is encountered, return DUP.
 */
struct pats *
insert(name, len)
    register char  *name;
    register int len;
{
    register struct pats **pp;
    register struct pats *p;
    register int n;

    pp = &ptab;
    p = *pp;
    while (p != NULL) {
	n = strncmp(name, p->name, len);
	if (n < 0) {
	    pp = &p->left;
	} else if (n > 0) {
	    pp = &p->right;
	} else if (len < p->namelen) {
	    pp = &p->left;
	} else {
	    werror("Duplicate definition: %s (ignored)\n", p->name);
	    return DUP;
	}
	p = *pp;
    }
    p = (struct pats*)malloc((unsigned)sizeof(struct pats));
    bzero(p, sizeof(struct pats));
    p->name = strncpy(malloc((unsigned)len+1), name, len);
    p->namelen = len;
    *pp = p;
    return (p);
}

struct pats *
lookup(name, len)
    register char  *name;
    register int    len;
{
    register int    n;
    register struct pats   *p;

    p = ptab;
    while (p != NULL) {
	n = strncmp(name, p->name, len);
	if (n < 0) {
	    p = p->left;
	} else if (n > 0) {
	    p = p->right;
	} else if (len < p->namelen) {
	    p = p->left;
	} else {
	    break;
	}
    }
    return (p);
}

char *patternbuf = NULL;
char linebuf[BUFSIZ];

#define nextch(cp,c) { c = *(cp)++; }
#define skipbl(cp,c) { while (isspace(c)) nextch(cp,c); }
#define findbl(cp,c) { while (!isspace(c)) nextch(cp,c); }

/*
 * scan constant
 * constant is passed by reference
 * function returns the number of chars scanned
 */

int
scanconst(cp,c,const)
    register char *cp;
    register char c;
    int *const;
{
    register int value, digit;
    char *temp;

    temp = cp;
    value = 0;
    if(c == '0' && *cp == 'x') {
	nextch(cp,c);
	nextch(cp,c);
	while (isxdigit(c)) {
	    c = tolower(c);
	    digit = (isdigit(c)? c - '0' : c - 'a' + 10);
	    value = value * 16 + digit;
	    nextch(cp,c);
	}
    } else if (c == '0') {
	nextch(cp,c);
	while (c >= '0' && c <= '7') {
	    value = value * 8 + (c - '0');
	    nextch(cp,c);
	}
    } else {
	while (isdigit(c)) {
	    value = value * 10 + (c - '0');
	    nextch(cp,c);
	}
    }
    *const = value;
    return(cp-temp);
}

void
readpats(fname)
    char   *fname;
{
    register    FILE * f;
    register char   c;
    register char  *cp;
    register char  *bufp;
    struct pats *pattern;
    struct stat statbuf;
    char *name;
    int len;
    int argsize;
    int filesize;

    filename = fname;
    lineno = 1;
    f = fopen(filename, "r");
    if (f == NULL) {
	cmderr("cannot open inline file '%s'\n", filename);
	return;
    }
    if (stat(filename, &statbuf) < 0) {
	cmderr("cannot stat inline file '%s': %s\n",
	    filename, sys_errlist[errno]);
	return;
    }
    filesize = statbuf.st_size;
    patternbuf = malloc(filesize + (BUFSIZ/2));
    pattern = NULL;
    while (fgets(linebuf, sizeof(linebuf), f)) {
	lineno++;
	cp = linebuf;
	nextch(cp,c);
	skipbl(cp,c);
	if (c == '.') {
	    if (strncmp (cp, "inline", 6) == 0) {
		/* .inline <name> starts a new routine */
		if (pattern != NULL) {
		    error("inline routines cannot be nested\n");
		    continue;
		}
		/* skip "inline" and white space */
		cp += 6;
		nextch(cp,c);
		skipbl(cp,c);
		/* scan routine name */
		if (c == '$' || c == '_' || isalpha(c)) {
		    name = cp - 1;
		    nextch(cp,c);
		    while (c == '$' || c == '_' || isalnum(c)) {
			nextch(cp,c);
		    }
		    pattern = insert(name, cp-name-1);
		    bufp = patternbuf;
		}
		skipbl(cp,c);
		if(c == ',') {
		    nextch(cp,c);
		    skipbl(cp,c);
		} else {
		    error("',' expected\n");
		}
		/* scan argument count */
		if(!isdigit(c)) {
		    error("argument count expected\n");
		} else {
		    cp += scanconst(cp,c,&pattern->argsize);
		}
		continue;
	    } else if (strncmp(cp, "end", 3) == 0) {
		/* .end terminates the current routine */
		if (pattern == NULL) {
		    error(".end without matching .inline\n");
		    continue;
		}
		*bufp++ = '\0';
		len = bufp - patternbuf;
		if (pattern != DUP) {
		    /*
		     * store pattern, but only the first time;
		     * i.e., ignore duplicate definitions
		     */
		    pattern->replace =
			strncpy(malloc((unsigned)len+1), patternbuf, len);
		}
		pattern = NULL;
		continue;
	    }
	}
	/* default action: stash the line */
	if (pattern != NULL) {
	    cp = linebuf;
	    while (c = *cp++) {
		*bufp++ = c;
	    }
	}
    }
    if (pattern != NULL) {
	error(".inline without matching .end\n");
    }
    free(patternbuf);
}

struct pats *
callinst(sourceline)
    char   *sourceline;
{
    register char *cp, **op;
    register char c;
    register char *name;
    register len;

    cp = sourceline;
    nextch(cp,c);
    skipbl(cp,c);
    if (!isalpha(c))
	return NULL;
    name = cp-1;
    while(isalnum(c))
	nextch(cp,c);
    len = cp-name-1;
    for (op = callops; *op != NULL; op++) {
	if (strncmp(*op, name, len) == 0) {
	    skipbl(cp,c);
	    name = cp-1;
	    while (isalnum(c) || c == '$' || c == '_')
		nextch(cp,c);
	    return lookup(name, cp-name-1);
	}
    }
    return (NULL);
}

adjust(pattern, output)
    struct pats *pattern;
    FILE *output;
{
    int n;

    n = pattern->argsize;
    if (n > 0) {
	(void)fprintf(output, "	%s	#%d,sp\n",
	    n <= 8 ? "subqw" : "subw", n);
    } else if (n < 0) {
	error(".inline routine %s has illegal argument size (%d)\n",
	    pattern->name, n);
    }
}

void
summary(p)
    register struct pats *p;
{
    if (p != NULL) {
	summary(p->left);
	summary(p->right);
	if (p->used) {
	    fprintf(stderr,"\t%s\n", p->name);
	}
    }
}

expand(input,output)
    register FILE *input;
    register FILE *output;
{
    register struct pats *pattern;

    while (fgets(linebuf, sizeof(linebuf), input)) {
	pattern = callinst(linebuf);
	if (pattern == NULL || pattern->replace == NULL) {
	    fputs(linebuf, output);
	} else {
	    pattern->used = 1;
	    fputs(pattern->replace, output);
	    adjust(pattern, output);
	}
    }
    if (verbose) {
	fprintf(stderr,"Expanded:\n");
	summary(ptab);
    }
}

main(argc, argv)
    int argc;
    char *argv[];
{
    int i;
    char *arg;
    FILE *input, *output;

    output = stdout;
    for (i = 1; i < argc; i++) {
	arg = argv[i];
	if (arg[0] == '-') {
	    if (arg[1] == 'i') {
		/* argv[i+1] is the name of an inline file */
		if (i+1 < argc) {
		    readpats(argv[i+1]);
		    i++;
		} else {
		    cmderr("filename expected after -i\n");
		    break;
		}
	    } else if (arg[1] == 'o') {
		/* argv[i+1] is the name of an output file */
		if (i+1 < argc) {
		    i++;
		    output = fopen(argv[i], "w");
		    if (output == NULL) {
			cmderr("cannot open output file '%s'\n", argv[i]);
			output = stdout;
		    }
		} else {
		    cmderr("filename expected after -o\n");
		    break;
		}
	    } else if (arg[1] == 'v') {
		verbose++;
	    } else if (arg[1] == 'w') {
		warnings++;
	    } else {
		cmderr("unknown option (%s)\n", arg);
		break;
	    }
	} else {
	    /* arg is the name of a source file */
	    if (nsources < MAXARGS) {
		sourcenames[nsources++] = arg;
	    } else {
		cmderr("too many arguments\n");
		break;
	    }
	}
    }
    if (nsources) {
	for(i = 0; i < nsources; i++) {
	    /* process source files in sequence */
	    input = fopen(sourcenames[i], "r");
	    if (input == NULL) {
		cmderr("cannot open source file '%s'\n", sourcenames[i]);
		continue;
	    }
	    expand(input, output);
	}
    } else {
	/* source defaults to standard input */
	expand(stdin, output);
    }
    exit(nerrors);
}

#undef malloc
char *
malloc_chk(n)
    unsigned n;
{
    extern char *malloc();
    char *p;

    p = malloc(n);
    if (p == NULL) {
	perror("inline");
	exit(1);
    }
    return p;
}
