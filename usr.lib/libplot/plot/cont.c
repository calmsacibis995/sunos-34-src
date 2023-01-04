#ifndef lint
static	char sccsid[] = "@(#)cont.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include <stdio.h>
cont(xi,yi){
	putc('n',stdout);
	putsi(xi);
	putsi(yi);
}
