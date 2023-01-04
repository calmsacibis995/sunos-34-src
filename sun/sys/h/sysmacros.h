/*	@(#)sysmacros.h 1.1 86/09/25 SMI	*/

/*
 * Major/minor device constructing/busting macros.
 */
#ifndef _SYSMACROS_
#define	_SYSMACROS_

/* major part of a device */
#define	major(x)	((int)(((unsigned)(x)>>8)&0377))

/* minor part of a device */
#define	minor(x)	((int)((x)&0377))

/* make a device number */
#define	makedev(x,y)	((dev_t)(((x)<<8) | (y)))

#endif
