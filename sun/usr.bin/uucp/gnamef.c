/*	gnamef.c	1.1	86/09/25	*/
	/*  gnamef 3.1  10/26/79  11:28:55  */
#include "uucp.h"
#include <sys/types.h>
#include <sys/dir.h>



/*******
 *	gnamef(p, filename)	get next file name from directory
 *	DIR *p;
 *	char *filename;
 *
 *	return codes:
 *		0  -  end of directory read
 *		1  -  returned name
 */


gnamef(p, filename)
DIR *p;
char *filename;
{
	register struct direct *dp;
	int i;
	char *s;

	while (1) {
		if ((dp = readdir(p)) == NULL)
			return(0);
		if (dp->d_ino != 0)
			break;
	}

	for (i = 0, s = dp->d_name; i < NAMESIZE-1; i++)
		if ((filename[i] = *s++) == '\0')
			break;
	filename[NAMESIZE-1] = '\0';
	return(1);
}
