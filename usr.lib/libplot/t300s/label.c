#ifndef lint
static	char sccsid[] = "@(#)label.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include "con.h"
label(s)
char *s;
{
	int i,c;
		while((c = *s++) != '\0'){
			xnow += HORZRES;
			spew(c);
		}
		return;
}
