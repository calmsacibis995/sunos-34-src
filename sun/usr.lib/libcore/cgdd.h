/*	@(#)cgdd.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/cms.h>
/*
 * Instance variables for view surfaces
 */
 struct cgddvars {
	short ddcp[2];			/* device coords current point */
	short fillop, fillrastop;	/* rop for region fill, raster */
	short cursorrop;		/* rop for cursor raster */
	short lineop;			/* rop for lines */
	short rtextop;			/* rop for raster text */
	short vtextop;			/* rop for vector text */
	short RAS8func;			/* rop for depth 8 raster */
	char cmsname[CMS_NAMESIZE];
	u_char red[256], green[256], blue[256];	/* colormap */
	short lineindex, textindex, fillindex;	/* color attributes */
	int linewidth, linestyle, polyintstyle;	/* line, poly attrib */
	int ddfont;			/* text attributes */
	int openfont;
	ipt_type ddup, ddpath, ddspace;
	PIXFONT *pf;
	struct pixrect *screen;		/* viewsurf and viewport pixrect */
	short openzbuffer;		/* zbuffer control */
        int _core_hidden;		/* True if doing hidden surfaces */
	struct pixrect *zbuffer, *cutaway;
	float *xarr, *zarr;             /* NDC cutaway data */
	int cutarraysize;               /* no. of cutaway points */
	int xoff, yoff, scale;		/* NDC to device */
	int maxy, maxz, fullx, fully;
	struct windowstruct wind;
	int curstrk;
	int lockcount;
};
