#ifndef lint
static	char sccsid[] = "@(#)space.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include <stdio.h>
space(x0,y0,x1,y1){
	putc('s',stdout);
	putsi(x0);
	putsi(y0);
	putsi(x1);
	putsi(y1);
}
