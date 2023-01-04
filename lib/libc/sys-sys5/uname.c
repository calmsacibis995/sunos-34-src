#ifndef lint
static	char sccsid[] = "@(#)uname.c 1.1 86/09/24 SMI";
#endif

/*
	uname -- system call emulation for 4.2BSD

	last edit:	05-Mar-1984	D A Gwyn
*/

#include	<errno.h>
#include	<string.h>
#include	<sys/utsname.h>

#define NULL	0

extern int	gethostname();

int
uname( name )
	register struct utsname *name;	/* where to put results */
	{
	register char		*np;	/* -> array being cleared */

	if ( name == NULL )
		{
		errno = EFAULT;
		return -1;
		}

	for ( np = name->nodename;
	      np < &name->nodename[sizeof name->nodename];
	      ++np
	    )
		*np = '\0';		/* for cleanliness */
	if ( gethostname( name->nodename, sizeof name->nodename ) != 0
	   )
		(void)strcpy( name->nodename, "unknown" );

	(void)strncpy( name->sysname, name->nodename,
		       sizeof name->sysname
		     );

	(void)strncpy( name->release, "4.2BSD", sizeof name->release );

	(void)strncpy( name->version, "vm", sizeof name->version );

	(void)strncpy( name->machine,
#ifdef	interdata
		       "interdata",
#else
#ifdef	pdp11
		       "pdp11",
#else
#ifdef	sun
		       "sun",
#else
#ifdef	u370
		       "u370",
#else
#ifdef	u3b
		       "u3b",
#else
#ifdef	vax
		       "vax",
#else
		       "unknown",
#endif
#endif
#endif
#endif
#endif
#endif
		       sizeof name->machine
		     );

	return 0;
	}
