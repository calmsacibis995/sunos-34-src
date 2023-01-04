#ifndef lint
static	char sccsid[] = "@(#)pwd.c 1.1 86/09/24 SMI"; /* from S5R2 1.3 */
#endif

/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#include		"mac.h"
#include		<sys/param.h>

#define	DOT		'.'
#define	NULL	0
#define	SLASH	'/'

static char cwdname[MAXPATHLEN];
static int 	didpwd = FALSE;

cwd(dir)
	register char *dir;
{
	register char *pcwd;
	register char *pdir;

	/* First remove extra /'s */

	rmslash(dir);

	/* Now remove any .'s */

	pdir = dir;
	while(*pdir) 			/* remove /./ by itself */
	{
		if((*pdir==DOT) && (*(pdir+1)==SLASH))
		{
			movstr(pdir+2, pdir);
			continue;
		}
		pdir++;
		while ((*pdir) && (*pdir != SLASH)) 
			pdir++;
		if (*pdir) 
			pdir++;
	}
	if(*(--pdir)==DOT && pdir>dir && *(--pdir)==SLASH)
		*pdir = NULL;
	

	/* Remove extra /'s */

	rmslash(dir);

	/* Now that the dir is canonicalized, process it */

	if(*dir==DOT && *(dir+1)==NULL)
	{
		return;
	}

	if(*dir==SLASH)
	{
		/* Absolute path */

		pcwd = cwdname;
		didpwd = TRUE;
	}
	else
	{
		/* Relative path */

		if (didpwd == FALSE) 
			return;
			
		pcwd = cwdname + length(cwdname) - 1;
		if(pcwd != cwdname+1)
		{
			*pcwd++ = SLASH;
		}
	}
	while(*dir)
	{
		if(*dir==DOT && 
		   *(dir+1)==DOT &&
		   (*(dir+2)==SLASH || *(dir+2)==NULL))
		{
			/* Parent directory, so backup one */

			if( pcwd > cwdname+2 )
				--pcwd;
			while(*(--pcwd) != SLASH)
				;
			pcwd++;
			dir += 2;
			if(*dir==SLASH)
			{
				dir++;
			}
			continue;
		}
		*pcwd++ = *dir++;
		while((*dir) && (*dir != SLASH))
			*pcwd++ = *dir++;
		if (*dir) 
			*pcwd++ = *dir++;
	}
	*pcwd = NULL;

	--pcwd;
	if(pcwd>cwdname && *pcwd==SLASH)
	{
		/* Remove trailing / */

		*pcwd = NULL;
	}
	return;
}

/*
 *	Print the current working directory.
 */

cwdprint()
{
	if (didpwd == FALSE)
		pwd();

	prs_buff(cwdname);
	prc_buff(NL);
	return;
}

/*
 *	This routine will remove repeated slashes from string.
 */

static
rmslash(string)
	char *string;
{
	register char *pstring;

	pstring = string;
	while(*pstring)
	{
		if(*pstring==SLASH && *(pstring+1)==SLASH)
		{
			/* Remove repeated SLASH's */

			movstr(pstring+1, pstring);
			continue;
		}
		pstring++;
	}

	--pstring;
	if(pstring>string && *pstring==SLASH)
	{
		/* Remove trailing / */

		*pstring = NULL;
	}
	return;
}

/*
 *	Find the current directory the hard way.
 */

extern char	*getwd();

static
pwd()
{
	if (getwd(cwdname) == NULL) {
		error(cwdname);
		cwdname[0] = '\0';
	}
}
