/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */

/*	@(#)mp.h 1.1 86/09/24 SMI; from UCB 5.1 5/30/85	*/

struct mint {
	int len;
	short *val;
};
typedef struct mint MINT;

extern MINT *itom();
extern MINT *xtom();
extern char *mtox();
extern short *xalloc();
extern void mfree();

#define	FREE(x)	xfree(&(x))		/* Compatibility */
