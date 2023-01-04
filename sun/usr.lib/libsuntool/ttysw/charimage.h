/*	@(#)charimage.h 1.5 87/01/07 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Definitions relating to maintenance of virtual screen image.
 */

/*
 * Screen is maintained as an array of characters.
 * Screen is bottom lines and right columns.
 * Each line has length and array of characters.
 * Characters past length position are undefined.
 * Line is otherwise null terminated.
 */
char	**image;
int	top, bottom, left, right;
int	cursrow, curscol;

#define length(line)	((unsigned char)((line)[-1]))
#define BOLDBIT		0X80

#define	setlinelength(line, column) \
	{ int _col = ((column)>right)?right:(column); \
	  (line)[(_col)] = '\0'; \
	  length((line)) = (_col);}
