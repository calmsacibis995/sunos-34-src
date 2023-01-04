/*	anyread.c	1.1	86/09/25	*/
	/*  anyread 3.1  10/26/79  11:21:09  */
#include "uucp.h"
#include <sys/types.h>
#include <sys/stat.h>


/*******
 *	anyread		check if anybody can read
 *	return 0 ok: FAIL not ok
 */

anyread(file)
char *file;
{
	struct stat s;

	if (stat(file, &s) != 0)
		/* for security check a non existant file is readable */
		return(0);
	if (!(s.st_mode & ANYREAD))
		return(FAIL);
	return(0);
}
