/*
 *	@(#)testexp.c 2.3 83/09/16 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/* 
 *	This program tests the expand and compress programs.
 */

#include "../h/dpy.h"
#include "./gallant.c"		/* The original font */
#include "./gallmash.c"		/* The mashed font */

unsigned short expanded[96][21];	/* The expanded mashed font */

main ()
{
	register short i, j, first;

	printf ("testexp: started\n");

	fexpand (expanded[0], f_bitmap, FBITMAPSIZE, f_index, f_data_hi,
		f_data_lo);

	for (i=0; i<96; i++) {
		first = 1;
		for (j=0; j<20; j++) {
			if ((expanded[i][j+1] & 0xFFF0) != font_gallant[i][j]) {
				if (first) printf ("For %c:", i+32);
				first = 0;
				printf ("\t%d wrong: %04x != %04x\n",
					j, font_gallant[i][j], expanded[i][j] );
			}
		}
	}

	printf ("That's all.\n");
}
