/*
 * @(#)getnum.c 2.4 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * getnum.c
 *
 * get a number from input
 */

unsigned char peekchar(), getone();

/* get a hex number */
int
getnum()
{
	register int v = 0;
	register int hexval;

	while ((hexval= ishex(peekchar()))>=0) {
			v= (v<<4)| hexval;
			getone();
	}
	return(v);
}
