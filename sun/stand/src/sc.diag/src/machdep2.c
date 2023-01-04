/*
 * Use #define SUN1  if you want this to run on both Sun-1 and Sun-2.
 * This will access the romvec at location 0x200000, so don't remap it...
 */
static char     sccsid[] = "@(#)machdep2.c 1.1 9/25/86 Copyright Sun Micro";

#include <mon/sunromvec.h>

getchar()
{
	return((*RomVecPtr->v_getchar)());
}

putchar(c)
{
	(*RomVecPtr->v_putchar)(c);
}
