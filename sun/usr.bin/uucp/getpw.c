/*	getpw.c	1.1	86/09/25	*/
	/*  getpw 3.1  10/26/79  11:27:15  */

#include "stdio.h"

getpw(uid, buf)
int uid;
char buf[];
{
	static FILE *pwf;
	register n, c;
	register char *bp;

	if(pwf == 0)
		pwf = fopen("/etc/passwd", "r");
	if(pwf == NULL)
		return(1);
	rewind(pwf);
	uid &= 0377;

	for (;;) {
		bp = buf;
		while((c=getc(pwf)) != '\n') {
			if(c <= 0)
				return(1);
			*bp++ = c;
		}
		*bp++ = '\0';
		bp = buf;
		n = 3;
		while(--n)
		while((c = *bp++) != ':')
			if(c == '\n')
				return(1);
		while((c = *bp++) != ':') {
			if(c<'0' || c>'9')
				continue;
			n = n*10+c-'0';
		}
		if(n == uid)
			return(0);
	}
}
