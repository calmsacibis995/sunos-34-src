#ifndef lint
static	char sccsid[] = "@(#)idprom.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/* Stolen from the ROM Monitor, version:
 * @(#)idprom.c 1.2 84/01/06 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <mon/idprom.h>

/*
 * Read the ID prom and check it.
 * Arguments are a format number and an address to store prom contents at.
 *
 * Result is format number if prom has the right format and good checksum.
 * Result is -1		   if prom has the right format and bad checksum.
 * Result is prom's format if prom has the wrong format.
 *
 * If the PROM is in the wrong format, the addressed area is not changed.
 *
 * This routine must know the size, and checksum algorithm, of each format.
 * (Currently there's only one.)
 */

int
idprom(format, idp)
	unsigned char format;
	register struct idprom *idp;
{
	unsigned char *cp, sum=0, promform;
	short i;

	Getidprom(&promform, 1);		/* Get format byte */
	if (format != promform)
		return promform;
	Getidprom((unsigned char *)idp, sizeof(*idp));	/* The whole thing */
	cp = (unsigned char *)idp;
	for (i=0; i<16; i++)
		sum ^= *cp++;
	if (sum != 0)
		return -1;
	return promform;
}
