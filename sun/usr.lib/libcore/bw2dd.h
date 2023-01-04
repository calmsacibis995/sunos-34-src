
/*	@(#)bw2dd.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Instance variables for view surfaces
 */
 struct bw2ddvars {
	short ddcp[2];		/* device coords current point */
	short fillop, fillrastop;/* rop for region fill, raster */
	short cursorrop;		/* rop for cursor raster */
	short lineop;		/* rop for lines */
	short rtextop;		/* rop for raster text */
	short vtextop;		/* rop for vector text */
	int texture; 		/* set the region fill texture */
	struct pixrect prmask;
	struct mpr_data prmaskdata;
	short msklist[16];
	u_char red[256], green[256], blue[256];
	int linewidth, linestyle, polyintstyle;	/* line attributes */
	int ddfont;		/* text attributes */
	int openfont;
	ipt_type ddup, ddpath, ddspace;
	PIXFONT *pf;
	int maxz;
	int fullx, fully, xoff, yoff, scale;
	struct pixrect *screen;
	struct windowstruct wind;
	int curstrk;
	int lockcount;
};
