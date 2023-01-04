/*
 * @(#)printhex.c 2.3 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

char chardigs[]="0123456789ABCDEF";

/*
 * printhex() prints rightmost <digs> hex digits of <val>
 */
printhex(val,digs)
	register int val;
	register int digs;
{

	digs = ((digs-1)&7)<<2;		/* digs == 0 => print 8 digits */
	
	for (; digs >= 0; digs-=4)
		putchar(chardigs[(val>>digs)&0xF]);
}
