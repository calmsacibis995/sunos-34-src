#ifndef lint
static  char sccsid[] = "@(#)adbgen1.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Read in "high-level" adb script and emit C program.
 * The input may have specifications within {} which
 * we analyze and then emit C code to generate the
 * ultimate adb acript.
 * We are just a filter; no arguments are accepted.
 */

#include <stdio.h>

#define	streq(s1, s2)	(strcmp(s1, s2) == 0)

#define	LINELEN  	1024	/* max line length expected in input */
#define	STRLEN		128	/* for shorter strings */
#define	NARGS		5	/* number of emitted subroutine arguments */

/*
 * Types of specifications in {}.
 */
#define	PRINT   	0	/* print member name with format */
#define	INDIRECT	1	/* fetch member value */
#define	OFFSETOK	2	/* insist that the offset is ok */
#define	SIZEOF		3	/* print sizeof struct */
#define	END		4	/* get offset to end of struct */
#define	OFFSET		5	/* just emit offset */

char struct_name[STRLEN];	/* struct name */
char member[STRLEN];		/* member name */
char format[STRLEN];		/* adb format spec */
char arg[NARGS][STRLEN];	/* arg list for called subroutine */

int line_no = 1;		/* input line number - for error messages */
int specsize;			/* size of {} specification - 1 or 2 parts */

extern char *index();

main(argc, argv)
	int argc;
	char **argv;
{
	register char *cp;
	char *start_printf();
	register int c;

	copy_init_lines();
	gets(struct_name);
	line_no++;
	/*
	 * Basically, the generated program is just an ongoing printf
	 * with breaks for {} format specifications.
	 */
	printf("\n");
	printf("main()\n");
	printf("{\n");
	if (argc > 1 && strcmp(argv[1], "-w") == 0) {
		printf("\textern int warnings;\n\n\twarnings = 0;\n");
	}
	cp = start_printf();
	while ((c = getchar()) != EOF) {
		switch (c) {
		case '"':
			*cp++ = '\\';	/* escape ' in string */
			*cp++ = '"';
			break;
		case '\n':
			line_no++;
			*cp++ = '\\';	/* escape newline in string */
			*cp++ = 'n';
			break;
		case '{':
			emit_printf(cp);
			read_spec();
			generate();
			cp = start_printf();
			break;
		default:
			*cp++ = c;
			break;
		}
		if (cp - arg[1] >= STRLEN - 10) {
			emit_printf(cp);
			cp = start_printf();
		}
	}
	emit_printf(cp);
	/* terminate program */
	printf("}\n");
	exit(0);
}

/*
 * Get started on printf of ongoing adb script.
 */
char *
start_printf()
{
	register char *cp;

	strcpy(arg[0], "\"%s\"");
	cp = arg[1];
	*cp++ = '"';
	return(cp);
}

/*
 * Emit call to printf to print part of ongoing adb script.
 */
emit_printf(cp)
	char *cp;
{
	*cp++ = '"';
	*cp = '\0';
	emit_call("printf", 2);
}

/*
 * Copy initial lines (up to first null line) to C program.
 * This is for includes of header files and defines that
 * may be necessary.
 */
copy_init_lines()
{
	char buf[LINELEN];

	for (;;) {
		if (gets(buf) == NULL) {
			fprintf(stderr, "Premature EOF\n");
			exit(1);
		}
		line_no++;
		if (buf[0] == '\0') {
			break;
		}
		puts(buf);
	}
}

/*
 * Read {} specification.
 * The first part (up to a comma) is put into "member".
 * The second part, if present, is put into "format".
 */
read_spec()
{
	register char *cp;
	register int c;

	cp = member;
	specsize = 1;
	while ((c = getchar()) != '}') {
		switch (c) {
		case EOF:
			fprintf(stderr, "Unexpected EOF inside {}\n");
			exit(1);
		case '\n':
			fprintf(stderr, "Newline not allowed in {}, line %d\n",
				line_no);
			exit(1);
		case ',':
			if (specsize == 2) {
				fprintf(stderr, "Excessive commas in {}, ");
				fprintf(stderr, "line %d\n", line_no);
				exit(1);
			}
			specsize = 2;
			*cp = '\0';
			cp = format;
			break;
		default:
			*cp++ = c;
			break;
		}
	}
	*cp = '\0';
	if (cp == member) {
		specsize = 0;
	}
}

/*
 * Decide what type of input specification we have.
 */
get_type()
{
	if (specsize == 1) {
		if (streq(member, "SIZEOF")) {
			return(SIZEOF);
		}
		if (streq(member, "OFFSETOK")) {
			return(OFFSETOK);
		}
		if (streq(member, "END")) {
			return(END);
		}
		return(OFFSET);
	}
	if (specsize == 2) {
		if (member[0] == '*') {
			return(INDIRECT);
		}
		return(PRINT);
	}
	fprintf(stderr, "Invalid specification, line %d\n", line_no);
	exit(1);
}

/*
 * Generate the appropriate output for an input specification.
 */
generate()
{
	switch (get_type()) {
	case PRINT:
		emit_print();
		break;
	case OFFSET:
		emit_offset();
		break;
	case INDIRECT:
		emit_indirect();
		break;
	case OFFSETOK:
		emit_offsetok();
		break;
	case SIZEOF:
		emit_sizeof();
		break;
	case END:
		emit_end();
		break;
	default:
		fprintf(stderr, "Internal error in generate\n");
		exit(1);
	}
}

/*
 * Emit calls to set the offset and print a member.
 */
emit_print()
{
	emit_offset();
	/*
	 * Emit call to "format" subroutine
	 */
	sprintf(arg[0], "\"%s\"", member);
	sprintf(arg[1], "sizeof ((struct %s *)0)->%s",
		struct_name, member);
	sprintf(arg[2], "\"%s\"", format);
	emit_call("format", 3);
}

/*
 * Emit calls to set the offset and print a member.
 */
emit_offset()
{
	/* 
	 * Emit call to "offset" subroutine
	 */
	sprintf(arg[0], "(int) &(((struct %s *)0)->%s)", struct_name, member);
	emit_call("offset", 1);
}

/*
 * Emit call to indirect routine.
 */
emit_indirect()
{
	sprintf(arg[0], "(int) &(((struct %s *)0)->%s)", struct_name,member+1);
	sprintf(arg[1], "sizeof ((struct %s *)0)->%s", struct_name, member+1);
	sprintf(arg[2], "\"%s\"", format);	/* adb register name */
	sprintf(arg[3], "\"%s\"", member);
	emit_call("indirect", 4);
}

/*
 * Emit call to "offsetok" routine.
 */
emit_offsetok()
{
	emit_call("offsetok", 0);
}

/*
 * Emit call to printf the sizeof the structure.
 */
emit_sizeof()
{
	sprintf(arg[0], "\"0t%%d\"");
	sprintf(arg[1], "sizeof (struct %s)", struct_name);
	emit_call("printf", 2);
}

/*
 * Emit call to set offset to end of struct.
 */
emit_end()
{
	sprintf(arg[0], "sizeof (struct %s)", struct_name);
	emit_call("offset", 1);
}

/*
 * Emit call to subroutine name with nargs arguments from arg array.
 */
emit_call(name, nargs)
	char *name;
	int nargs;
{
	register int i;

	printf("\t%s(", name);		/* name of subroutine */
	for (i = 0; i < nargs; i++) {
		if (i > 0) {
			printf(", ");	/* argument separator */
		}
		printf("%s", arg[i]);	/* argument */
	}
	printf(");\n");			/* end of call */
}
