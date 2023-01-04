/*	versys.c	1.1	86/09/25	*/
	/*  versys 3.1  10/26/79  11:48:11  */

#include "uucp.h"


#define SNAMESIZE 7

/*******
 *	versys(name)	verify system names n1 and n2
 *	char *name;
 *
 *	return codes:  0  |  FAIL
 */

versys(name)
char *name;
{
	FILE *fp;
	char line[300];
	char s1[SNAMESIZE + 1];
	char myname[SNAMESIZE + 1];

	sprintf(myname, "%.7s", Myname);
	sprintf(s1, "%.7s", name);
	if (strcmp(s1, myname) == 0)
		return(0);
	fp = fopen(SYSFILE, "r");
	if (fp == NULL)
		return(FAIL);
	
	while (cfgets(line, 300, fp) != NULL) {
		char *targs[20];

		getargs(line, targs);
		targs[0][7] = '\0';
		if (strcmp(s1, targs[0]) == SAME) {
			fclose(fp);
			return(0);
		}
	}
	fclose(fp);
	return(FAIL);
}
