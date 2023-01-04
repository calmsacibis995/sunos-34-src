/*	@(#)consdev.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

dev_t	consdev;	/* user retargettable console */
dev_t	rconsdev;	/* real console - 0 if monitor */
dev_t	mousedev;	/* default mouse device */
dev_t	kbddev;		/* default keyboard device */
int	kbddevopen;	/* keyboard open flag */
dev_t	fbdev;		/* default framebuffer device */

