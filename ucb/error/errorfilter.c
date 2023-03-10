#ifndef lint
static	char sccsid[] = "@(#)errorfilter.c 1.1 86/09/25 SMI"; /* from UCB 5.1 5/31/85 */
#endif
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include "error.h"

char	*lint_libs[] = {
	IG_FILE1,
	IG_FILE2,
	IG_FILE3,
	IG_FILE4,
	0
};
extern	char*	processname;
int	lexsort();
/*
 *	Read the file ERRORNAME of the names of functions in lint
 *	to ignore complaints about.
 */
getignored(auxname)
	char	*auxname;
{
	reg	int	i;
		FILE	*fyle;
		char	inbuffer[256];
		int	uid;
		char	filename[128];
		char	*username;
		struct	passwd *passwdentry;

	nignored = 0;
	if (auxname == 0){	/* use the default */
		if ( (username = (char *)getlogin()) == NULL){
			username = "Unknown";
			uid = getuid();
			if ( (passwdentry = (struct passwd *)getpwuid(uid)) == NULL){
				return;
			}
		} else {
			if ( (passwdentry = (struct passwd *)getpwnam(username)) == NULL)
				return;
		}
		strcpy(filename, passwdentry->pw_dir);
		(void)strcat(filename, ERRORNAME);
	} else
		(void)strcpy(filename, auxname);
#ifdef FULLDEBUG
	printf("Opening file \"%s\" to read names to ignore.\n",
		filename);
#endif
	if ( (fyle = fopen(filename, "r")) == NULL){
#ifdef FULLDEBUG
		fprintf(stderr, "%s: Can't open file \"%s\"\n",
			processname, filename);
#endif
		return;
	}
	/*
	 *	Make the first pass through the file, counting lines
	 */
	for (nignored = 0; fgets(inbuffer, 255, fyle) != NULL; nignored++)
		continue;
	names_ignored = (char **)Calloc(nignored+1, sizeof (char *));
	fclose(fyle);
	if (freopen(filename, "r", fyle) == NULL){
#ifdef FULLDEBUG
		fprintf(stderr, "%s: Failure to open \"%s\" for second read.\n",
			processname, filename);
#endif
		nignored = 0;
		return;
	}
	for (i=0; i < nignored && (fgets (inbuffer, 255, fyle) != NULL); i++){
		names_ignored[i] = strsave(inbuffer);
		(void)substitute(names_ignored[i], '\n', '\0');
	}
	qsort(names_ignored, nignored, sizeof *names_ignored, lexsort);
#ifdef FULLDEBUG
	printf("Names to ignore follow.\n");
	for (i=0; i < nignored; i++){
		printf("\tIgnore: %s\n", names_ignored[i]);
	}
#endif
}

int lexsort(cpp1, cpp2)
	char	**cpp1, **cpp2;
{
	return(strcmp(*cpp1, *cpp2));
}

int search_ignore(key)
	char	*key;
{
	reg	int	ub, lb;
	reg	int	halfway;
		int	order;

	if (nignored == 0)
		return(-1);
	for(lb = 0, ub = nignored - 1; ub >= lb; ){
		halfway = (ub + lb)/2;
		if ( (order = strcmp(key, names_ignored[halfway])) == 0)
			return(halfway);
		if (order < 0)	/*key is less than probe, throw away above*/
			ub = halfway - 1;
		 else
			lb = halfway + 1;
	}
	return(-1);
}

/*
 *	Tell if the error text is to be ignored.
 *	The error must have been canonicalized, with
 *	the file name the zeroth entry in the errorv,
 *	and the linenumber the second.
 *	Return the new categorization of the error class.
 */
Errorclass discardit(errorp)
	reg	Eptr	errorp;
{
		int	language;
	reg	int	i;
	Errorclass	errorclass = errorp->error_e_class;

	switch(errorclass){
		case	C_SYNC:
		case	C_NONSPEC:
		case	C_UNKNOWN:	return(errorclass);
		default:	;
	}
	if(errorp->error_lgtext < 2){
		return(C_NONSPEC);
	}
	language = errorp->error_language;
	if(language == INLINT){
		if (errorclass != C_NONSPEC){	/* no file */
			for(i=0; lint_libs[i] != 0; i++){
				if (strcmp(errorp->error_text[0], lint_libs[i]) == 0){
					return(C_DISCARD);
				}
			}
		}
		/* check if the argument to the error message is to be ignored*/
		if (ispunct(lastchar(errorp->error_text[2])))
			clob_last(errorp->error_text[2], '\0');
		if (search_ignore(errorp->error_text[errorclass == C_NONSPEC ? 0 : 2]) >= 0){
			return(C_NULLED);
		}
	}
	return(errorclass);
}
