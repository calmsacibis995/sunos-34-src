#ifndef lint
static	char sccsid[] = "@(#)close.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include <stdio.h>
closevt(){
	putch(037);
	fflush(stdout);
}
closepl(){
	putch(037);
	fflush(stdout);
}
