#ifndef lint
static	char sccsid[] = "@(#)onepass_init.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

# define FORT
/* this forces larger trees, etc. */
# include "cpass2.h"

unsigned int offsz;
unsigned int caloff();
extern int stmtprofflag;

cg_init( argstring)
char *argstring;
{
	int files;
	register long x;
	register NODE *p;
	long newftnno;
	int argc = 0;
	char *argv[100], **argvp = argv, *cp;

	/* break argstring into an argv */
	*argvp++ = "cg";
	argc++;

	while(*argstring == ' ') {
		argstring++;
	}

	cp = argstring;
	if(*cp) for(;;) {
		while(*cp != ' ' && *cp ) {
			cp++;
		}
		argc ++;
		*argvp++ = argstring;
		if(*cp == '\0') break;
		*cp++ = '\0';
		while(*cp == ' ' && *cp ) cp++;
		if(*cp == '\0') break;
		argstring = cp;
	}
	*argvp = 0;

	offsz = caloff();
	p2init( argc, argv );
	tinit();
	intr_map_init();
		
}

cg_end()
{
	if(stmtprofflag) {
		stmtprof_eof();
	}
	if (fltused) {
		floatnote();
	}
}

/* to satisfy reference in  util2 main() */
mainp2(){}
