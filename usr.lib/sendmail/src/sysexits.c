# include <sysexits.h>
# include "useful.h"

SCCSID(@(#)sysexits.c 1.1 86/09/25 SMI); /* from UCB 4.2 7/31/83 */

/*
**  SYSEXITS.C -- error messages corresponding to sysexits.h
*/

char	*SysExMsg[] =
{
	/* 64 USAGE */		"500 Bad usage",
	/* 65 DATAERR */	"501 Data format error",
	/* 66 NOINPUT */	"550 Cannot open input",
	/* 67 NOUSER */		"550 User unknown",
	/* 68 NOHOST */		"550 Host unknown",
	/* 69 UNAVAILABLE */	"554 Service unavailable",
	/* 70 SOFTWARE */	"554 Internal error",
	/* 71 OSERR */		"451 Operating system error",
	/* 72 OSFILE */		"554 System file missing",
	/* 73 CANTCREAT */	"550 Can't create output",
	/* 74 IOERR */		"451 I/O error",
	/* 75 TEMPFAIL */	"250 Deferred",
	/* 76 PROTOCOL */	"554 Remote protocol error",
	/* 77 NOPERM */		"550 Insufficient permission",
};

int	N_SysEx = sizeof SysExMsg / sizeof SysExMsg[0];
