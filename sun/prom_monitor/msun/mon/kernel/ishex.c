/*
 * @(#)ishex.c 2.3 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * ishex.c
 *
 * find hex value of character
 */

#define isnum(c) ((c>='0')&&(c<='9'))

/*
 * Returns the hex value of a char or -1 if the char is not hex
 */
int
ishex(c)
	register unsigned char c;
{

	if (isnum(c)) 		return(c-'0');
	if (c>='a'&&c<='f') 	return(c-'a'+10);
	if (c>='A'&&c<='F') 	return(c-'A'+10);
	return(-1);
}
