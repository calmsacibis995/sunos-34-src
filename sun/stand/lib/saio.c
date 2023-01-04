#include <sunromvec.h>

static char	sccsid[] = "@(#)saio.c 1.1 9/25/86 Copyright Sun Micro";

int
getchar()
{
	return((*romp->v_getchar)());
}

int
maygetchar()
{
	return((*romp->v_mayget)());
}

int
putchar(c)
{
	return((*romp->v_putchar)(c));
}

int
gets(buf)
register char		*buf;
{
	register	count = 0;
	register char	c, *to = buf;

	for(;;){
	while((c = maygetchar()) < 0);		/* wait for char */
	switch(c){				/* process it */
		case '\b':			/* erase char */
		case '#':
		case '\177':			/* delete */
			if (count){		/* get rid (if there) */
				printf("\b \b");/* scratch char */
				--to; --count;
			}
			break;
		case '@':			/* kill char */
		case '\025':			/* ^U */
			while(count--)		/* scratch line */
				printf("\b \b");
			to = buf;		/* reset pointer */
			count = 0;
			break;
		case '\r':			/* newline */
		case '\n':
			putchar('\n');		/* echo it */
			*to = 0;		/* terminate line */
			return(count);		/* tell 'em what we got */
			break;			/* for grins */
		default:
			++count;
			putchar(*to++ = c);	/* add char */
			break;
	}
	}
}
