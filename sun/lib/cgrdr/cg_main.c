#ifndef lint
static	char sccsid[] = "@(#)cg_main.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

# define FORT
/* this forces larger trees, etc. */
# include "cpass2.h"

unsigned int offsz;
unsigned int caloff();
mainp2( argc, argv ) 
char *argv[]; 
{
	int files;
	register long x;
	register NODE *p;
	long newftnno;

	offsz = caloff();
	files = p2init( argc, argv );
	tinit();
	intr_map_init();
		
	if( files ){
		while( files < argc && argv[files][0] == '-' ) {
			++files;
			}
		if( files > argc ) return( nerrors );
	} else {
			uerror( "no ir input file");
	}
	do_ir_archive(argv[files], xdebug, usesky);
	return nerrors ;
}
