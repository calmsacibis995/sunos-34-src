#ifndef lint
static	char sccsid[] = "@(#)point.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include <stdio.h>
point(xi,yi){
	putc('p',stdout);
	putsi(xi);
	putsi(yi);
}
