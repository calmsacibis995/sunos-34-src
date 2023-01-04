#ifndef lint
static	char sccsid[] = "@(#)point.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include "con.h"
point(xi,yi){
		move(xi,yi);
		label(".");
		return;
}
