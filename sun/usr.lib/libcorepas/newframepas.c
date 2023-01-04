#ifndef lint
static char sccsid[] = "@(#)newframepas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int new_frame();

int newframe(dummy)
int dummy;
	{
	return(new_frame());
	}
