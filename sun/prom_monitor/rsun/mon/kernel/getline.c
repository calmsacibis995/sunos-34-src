/*
 * @(#)getline.c 2.6 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * getline.c
 *
 * read input line for Sun ROM Monitor
 */

#include "../h/sunmon.h"
#include "../h/globram.h"

unsigned char getchar();

/*
 * read the next input line into a global buffer
 */
getline(echop)
	int echop;
{
	register unsigned char *p = gp->g_linebuf;
	register int linelength = 0;
	register int erase;	/* number of characters to erase */

	gp->g_lineptr = p;	/* reset place-in-line pointer */
	gp->g_tracecmds= 0;	/* Reset trace ptr, which shares buf w/us */

	for (;;) {
		while ( (erase=mayget()) <= 0) ;	/* Skip nulls too */
		*p = (unsigned char) erase;

		     if (*p == CERASE1 || *p == CERASE2)
			erase = 1;	/* erase one character */
		else if (*p == CKILL1)
			erase = linelength;	/* erase all chars on line */
		else {
			if (echop) putchar (erase);

			if (*p == '\r') break;

			if (linelength < BUFSIZE) { /* line not full */
			    p++;
			    linelength++;
			/* here we could ding for line-too-long */
			}
			erase = 0;
		}

		while(linelength && erase--) {
			p--;
			linelength--;
			printf("\b \b");
		    }
	}

	if (echop) putchar('\n'); /* echo a linefeed to follow the return */
	*(++p)= '\0';	/* the ++ protects the \r to make getone() work */
	gp->g_linesize = linelength;
}

/*
 * gets one character from the input buffer; returns are
 * converted to nulls
 */
unsigned char
getone()
{

	return ((*(gp->g_lineptr)=='\r')?
		'\0':
		 *((gp->g_lineptr)++));
}

/*
 * indicates the next character that getone() would return;
 * does NOT convert \r to \0.
 */
unsigned char
peekchar()
{

	return(*(gp->g_lineptr));
}

