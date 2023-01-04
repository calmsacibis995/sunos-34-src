#ifndef lint
static	char sccsid[] = "@(#)probe.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Common routines for probing memory.
 * Typically called by drivers during initialization to
 * see if a device is present on the Mainbus.
 */
#include "../h/types.h"
#include "../sun/fault.h"

static label_t *saved_jb;
static label_t jb;

/*
 * Probe a location by attempting to read a word
 */
peek(a)
	short *a;
{
	int val;

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		val = *a;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (val & 0xFFFF);
	}
	/* A fault occured */
	nofault = saved_jb;
	return (-1);
}

/*
 * Probe a location by attempting to read a byte
 */
peekc(a)
	char *a;
{
	int val;

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		val = *a;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (val & 0xFF);
	}
	/* A fault occured */
	nofault = saved_jb;
	return (-1);
}

/*
 * Probe a location by attempting to read a long
 */
peekl(a, val)
	long *a;
	long *val;
{

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		*val = *a;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (0);
	}
	/* A fault occured */
	nofault = saved_jb;
	return (-1);
}

/*
 * Probe a location by attempting to write a long word
 */
pokel(a, val)
	long *a;
	long val;
{

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		*a = val;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (0);
	}
	/* A fault occured */
	nofault = saved_jb;
	return (1);
}

/*
 * Probe a location by attempting to write a word
 */
poke(a, val)
	short *a;
	short val;
{

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		*a = val;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (0);
	}
	/* A fault occured */
	nofault = saved_jb;
	return (1);
}

/*
 * Probe a location by attempting to write a byte
 */
pokec(a, val)
	char *a;
	char val;
{

	saved_jb = nofault;
	nofault = &jb;
	if (!setjmp(&jb)) {
		*a = val;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (0);
	}
	/* A fault occured */
	nofault = saved_jb;
	return (1);
}
