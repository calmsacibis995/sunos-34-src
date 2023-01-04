#ifndef lint
static	char sccsid[] = "@(#)bb.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "defs.h"

static	FILE	*dotd_fp;	/* The .d file for line numbers and counts */
static	char	dotd_file[500];	/* The name of the .d file */
static	int	bbnum;		/* number of basic blocks */
static 	Boolean	bb_print;	/* Flag for when we've seen a basic block */

/*
 * bb_decls - some declarations for the basic blocks
 */
bb_decls()
{
	printf("static\tunsigned int\t*___bb;\n");
	printf("static\tint\t___bb_init;\n");
	printf("extern\tint\t___tcov_init;\n");
}

/*
 * basic_block - we have the beginning of a basic block
 * Do not print a counting statement here, because some situations
 * look like the beginning of several basic blocks and in those
 * cases we want only one counting statement.
 */
basic_block()
{
	bb_print = True;
}

/*
 * pr_basic_block - print a basic block counting statement
 */
pr_basic_block(lno)
int	lno;
{
	if (bb_print) {
		printf("\t___bb[%d]++;", bbnum);
		bbnum++;
		fprintf(dotd_fp, "%d 0\n", (lno == 0) ? lineno() : lno);
		bb_print = False;
	}
}


/*
 * basic_block_comma - we have the beginning of a basic within a control struct
 * Print a basic block counter followed by the comma operator.  The 
 * expression within the control structure will follow the comma operator.
 */
basic_block_comma()
{
	printf("___bb[%d]++, ", bbnum);
	bbnum++;
	fprintf(dotd_fp, "%d 0\n", lineno());
	bb_print = False;
}

/*
 * prinit - print out a call to the init routine
 * This occurs for the very first basic block in a function.
 */
prinit()
{
	printf("\tif (!___bb_init) ___bb_init_func();");
	printf("\tif (!___tcov_init) ___tcov_init_func();");
}

/*
 * pr_bb_routine - print the routine to initialize the basic blocks.
 */
pr_bb_routine()
{
	if (bbnum > 0) {
		printf("static\n");
		printf("___bb_init_func()\n");
		printf("{\n");
		printf("\t___bb_init = 1;\n");
		printf("\t___bb = (unsigned int *) malloc(%d * sizeof(unsigned int));\n", bbnum);
		printf("\t___bb_link(\"%s\", ___bb, %d);\n", dotd_file, bbnum);
		printf("}\n");
	}
}

/*
 * dotd - create a name with a .d suffix and open the file
 */
dotd(srcname)
char	*srcname;
{
	char	*cp;
	FILE	*pfp;
	FILE	*popen();

	if (srcname[0] == '/') {
		strcpy(dotd_file, srcname);
	} else {
		pfp = popen("pwd", "r");
		fscanf(pfp, "%s", dotd_file);
		pclose(pfp);
		strcat(dotd_file, "/");
		strcat(dotd_file, srcname);
	}
	cp = rindex(dotd_file, '.');
	cp[1] = 'd';
	dotd_fp = fopen(dotd_file, "w");
	if (dotd_fp == NULL) {
		error("Unable to open %s", dotd_file);
	}
}
