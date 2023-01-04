#ifndef lint
static	char sccsid[] = "@(#)cim_change.c 1.5 87/01/07 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Character image manipulation (except size change) routines.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include "ttyansi.h"
#include "charimage.h"
#include "charscreen.h"

int	boldify;

extern	char *  strcpy();

#define JF

vpos(row, col)
	int row, col;
{
	register char *line = image[row];

	while (length(line) <= col)
		line[length(line)++] = ' ';
	setlinelength(line, (length(line)));
}

bold()
{
	boldify = 1;
}

nobold()
{
	boldify = 0;
}

writePartialLine(s, curscolStart)
	char *s;
	register int curscolStart;
{
	register char *sTmp, *line = image[cursrow];
	register int curscolTmp = curscolStart;

	/*
	 * Fix line length if start is past end of line length.
	 * This shouldn't happen but does.
	 */
	if (length(line) < curscolStart )
		(void)vpos(cursrow, curscolStart);
	/*
	 * Stick characters in line.
	 */
	for (sTmp=s;*sTmp!='\0';sTmp++) {
		line[curscolTmp] = *sTmp;
		if (boldify) {
			line[curscolTmp] |= BOLDBIT;
			*sTmp |= BOLDBIT; /* Modified cause used below */
		}
		curscolTmp++;
	}
	/*
	 * Set new line length.
	 */
	if (length(line) < curscolTmp )
		setlinelength(line, curscolTmp);
/*
if (sTmp>(s+3)) printf("%d\n",sTmp-s);
*/
	/* Note: curscolTmp should equal curscol here */
/*
if (curscolTmp!=curscol)
	printf("csurscolTmp=%d, curscol=%d\n", curscolTmp,curscol);
*/
	(void)pstring(s, curscolStart, cursrow, PIX_SRC);
}

#ifdef	USE_WRITE_CHAR
writechar(c)
	char c;
{
	register char *line = image[cursrow];
	char unitstring[2];

	unitstring[0] = line[curscol] = c;
	unitstring[1] = 0;
	if (length(line) <= curscol )
		setlinelength(line, curscol+1);
	/* Note: if revive this proc then null terminate line */
	(void)pstring(unitstring, curscol, cursrow, PIX_SRC);
}
#endif	USE_WRITE_CHAR

#ifdef JF
cim_scroll(n)
register int n;
{	register int	new;

#ifdef DEBUG_LINES
printf(" cim_scroll(%d)	\n", n);
#endif
	if (n>0) {		/*	text moves UP screen	*/
		(void)delete_lines(top, n);
	} else {	/* (n<0)	text moves DOWN	screen	*/
		new = bottom + n;
		(void)roll(top, new, bottom);
		(void)pcopyscreen(top,top - n, new);
		(void)cim_clear(top, top - n);
	}
}
#else
cim_scroll(toy, fromy)
	int fromy, toy;
{

	if (toy < fromy) /* scrolling up */
		(void)roll(toy, bottom, fromy);
	else
		swapregions(fromy, toy, bottom-toy);
	if (fromy > toy) {
		(void)pcopyscreen(fromy, toy, bottom - fromy);
		(void)cim_clear(bottom-(fromy-toy), bottom);	/* move text up */
	} else {
		(void)pcopyscreen(fromy, toy, bottom - toy);
		(void)cim_clear(fromy, bottom-(toy-fromy));	/* down */
	}
}
#endif

insert_lines(where, n)
register int where, n;
{	register int new = where + n;

#ifdef DEBUG_LINES
printf(" insert_lines(%d,%d) bottom=%d	\n", where, n, bottom);
#endif
	if (new > bottom)
		new = bottom;
	(void)roll(where, new, bottom);
	(void)pcopyscreen(where, new, bottom - new);
	(void)cim_clear(where, new);
}

delete_lines(where, n)
register int where, n;
{	register int new = where + n;

#ifdef DEBUG_LINES
printf(" delete_lines(%d,%d)	\n", where, n);
#endif
	if (new > bottom) {
		n -= new - bottom;
		new = bottom;
	}
	(void)roll(where, bottom - n, bottom);
	(void)pcopyscreen(new, where, bottom - new);
	(void)cim_clear(bottom-n, bottom);
}

roll(first, mid, last)
	int first, last, mid;
{

/* printf("first=%d, mid=%d, last=%d\n", first, mid, last); */
	reverse(first, last);
	reverse(first, mid);
	reverse(mid, last);
}

static
reverse(a, b)
	int a, b;
{

	b--;
	while (a < b)
		(void)swap(a++, b--);
}

swapregions(a, b, n)
	int a, b, n;
{

	while (n--)
		(void)swap(a++, b++);
}

swap(a, b)
	int a, b;
{
	char *tmpline = image[a];

	image[a] = image[b];
	image[b] = tmpline;
}

cim_clear(a, b)
	int a, b;
{
	register int i;

	for (i = a; i < b; i++)
		setlinelength(image[i], 0);
	(void)pclearscreen(a, b);
	if (a == top && b == bottom) {
		if (delaypainting)
			(void)pdisplayscreen(1);
		else
			delaypainting = 1;
	}
}

deleteChar(fromcol, tocol, row)
	int fromcol, tocol, row;
{
	register char *line = image[row];
	int len = length(line);

	if (fromcol >= tocol)
		return;
	if (tocol < len) {
		/*
		 * There's a fragment left at the end
		 */
		register int	gap = tocol - fromcol;
		(void)strcpy(line+fromcol, line+tocol);
		setlinelength(line, len-gap);
		(void)pcopyline(fromcol, tocol, len - tocol, row);
		(void)pclearline(len - gap, len, row);
	} else if (fromcol < len) {
		setlinelength(line, fromcol);
		(void)pclearline(fromcol, len, row);
	}
}

insertChar(fromcol, tocol, row)
	int fromcol;
	register int tocol;
	int row;
{
	register char *line = image[row];
	int len = length(line);
	register int i;
	int delta, newlen, slug, rightextent;;

	if (fromcol >= tocol || fromcol >= len)
		return;
	delta = tocol - fromcol;
	newlen = len+delta;
	if (newlen > right)
		newlen = right;
	for (i = newlen; i >= tocol; i--)
		line[i] = line[i-delta];
	for (i = fromcol; i < tocol; i++)
		line[i] = ' ';
	setlinelength(line, newlen);
	rightextent = len+(tocol-fromcol);
	slug = len - fromcol;
	if (rightextent > right)
		slug -= rightextent - right;
	(void)pcopyline(tocol, fromcol, slug, row);
	(void)pclearline(fromcol, tocol, row);
}

