/*	@(#)cursor_impl.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#define	DONT_SHOW_CURSOR	0x00000001
#define	SHOW_CROSSHAIRS		0x00000002
#define	SHOW_HORIZ_HAIR		0x00000004
#define	SHOW_VERT_HAIR		0x00000008
#define	FULLSCREEN		0x00000010
#define	HORIZ_BORDER_GRAVITY	0x00000020
#define	VERT_BORDER_GRAVITY	0x00000040
#define	FREE_SHAPE		0x00000080

#define	show_cursor(cursor)		(!((cursor)->flags & DONT_SHOW_CURSOR))
#define	show_crosshairs(cursor)		((cursor)->flags & SHOW_CROSSHAIRS)
#define	show_horiz_hair(cursor)		((cursor)->flags & SHOW_HORIZ_HAIR)
#define	show_vert_hair(cursor)		((cursor)->flags & SHOW_VERT_HAIR)
#define	fullscreen(cursor)		((cursor)->flags & FULLSCREEN)
#define	horiz_border_gravity(cursor)	((cursor)->flags & HORIZ_BORDER_GRAVITY)
#define	vert_border_gravity(cursor)	((cursor)->flags & VERT_BORDER_GRAVITY)
#define	free_shape(cursor)		((cursor)->flags & FREE_SHAPE)
