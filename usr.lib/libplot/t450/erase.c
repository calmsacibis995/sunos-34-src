#ifndef lint
static	char sccsid[] = "@(#)erase.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include "con.h"
erase(){
	int i;
		for(i=0; i<11*(VERTRESP/VERTRES); i++)
			spew(DOWN);
		return;
}
