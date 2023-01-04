/*
 *	@(#)makefont.c 2.3 83/09/16 Copyright (c) 1983 by Sun Microsystems, Inc.
 *
 *	Also see copyright notice further below, which is inserted into
 *	the resulting program fragment.
 */

#include <stdio.h>
#include "../h/dpy.h"

#define MAXSIZE ((CHRHEIGHT-2)*96)
#define BMSIZE  ((CHRHEIGHT-2)*96/8)

unsigned short f_data[256];
unsigned char f_bitmap[BMSIZE];
unsigned char f_index[MAXSIZE];
short max_i_data = 0;

#include "./gallant.c"

unsigned char findindex();

main(argc,argv)
	int argc;
	char *argv[];
{
	FILE *g;
	register unsigned short *ptable = font_gallant[0];
	register unsigned char *pindex = f_index;
	register unsigned char *pbitmap = f_bitmap;
	register short i, j, curpixels, pixels;
	register unsigned char bits;
	int bytes;
	int indexsize;

	g = stdout;
	if (argc == 1) goto skipstuff;

	if (argc != 2) {
		fprintf(stderr,"Usage: makefont outfile\n");
		exit(1);
	}
	g = fopen(argv[1],"w");
	if (g <= 0) {
		fprintf(stderr,"makefont: can't open %s\n", argv[1]);
		exit(1);
	}

skipstuff:

/* NOW DO SOME REAL WORK */

	curpixels = 1 + *ptable;	/* ensure 1st entry is diff'rent */

	for (i = BMSIZE - 1; i != -1; i--) {
		for (j = 7; j != -1; j--) {
			pixels = *ptable++;
			if (pixels == curpixels) {
				bits <<= 1;
			} else {
				bits = (bits << 1) | 1;
				curpixels = pixels;
				*pindex++ = findindex(pixels);
			}
		}
		*pbitmap++ = bits;
	}

/* THE TABLES ARE FILLED.  NOW PRINT THEM AS A C PROGRAM FRAGMENT. */

	fprintf (stderr, "makefont: %d changes, %d unique rows\n",
		 pindex - f_index, max_i_data);
	bytes = ( 
		 (BMSIZE)			/* bit map */
		+(indexsize = pindex - f_index)		/* index table */
		+((max_i_data*12)/8)		/* Data itself */
		);
	fprintf (stderr, "makefont: %d (0x%x) bytes for font tables\n",
		 bytes, bytes);

	pbitmap = f_bitmap;

	/* First print SCCS header line and copyright notice */
	fprintf (g,"%s%s%s%s%s","/*\n *	%Z", "%%M","% %I","% %E",
		"% Copyright (c) 1983 by Sun Microsystems, Inc.\n */\n\n");

	fprintf (g,"unsigned char f_bitmap[%d] = {\n\t", BMSIZE);
	for (i = 0; i < BMSIZE; i++) {
		fprintf (g, "0x%02x, ", *pbitmap++);
		if (7 == (i&7)) fprintf (g, "\n\t");
	}
	fprintf (g, "};\n\n");

	pindex = f_index;
	fprintf (g, "unsigned char f_index[%d] = {\n\t", indexsize);
	for (i = 0; i < indexsize; i++) {
		fprintf (g, "%d, ", *pindex++);
		if (7 == (i&7)) fprintf (g, "\n\t");
	}
	fprintf (g, "};\n\n");

	fprintf (g, "unsigned char f_data_hi[%d] = {\n\t", max_i_data);
	for (i = 0; i < max_i_data; i++) {
		fprintf (g, "0x%02x, ", f_data[i]>>8);
		if (7 == (i&7)) fprintf (g, "\n\t");
	}
	fprintf (g, "};\n\n");
	
	fprintf (g, "unsigned char f_data_lo[%d] = {\n\t", (max_i_data+1)/2);
	for (i = 0; i < max_i_data; i+=2) {
		fprintf (g, "0x%02x, ", 
			(0x00F0 & f_data[i])|(0x000F & (f_data[i+1]>>4)));
		if (14 == (i&15)) fprintf (g, "\n\t");
	}
	fprintf (g, "};\n\n");

	fclose (g);

}


unsigned char
findindex(pix)
	register short pix;
{
	short i;
	
	for (i = max_i_data-1; i != -1; i--) {
		if (pix == f_data[i]) return i;
	}
	if (max_i_data == 256) {
		fprintf (stderr,
		"makefont: More than 256 distinct pixel lines in font.\n");
		exit(2);
	}
	f_data[max_i_data] = pix;
	return max_i_data++;
}
