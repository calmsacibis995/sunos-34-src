/*	@(#)gp1dd.h 1.1 86/09/25 Copyr 1985 Sun Micro */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef CMS_H
#define CMS_H
#include <sunwindow/cms.h>
#endif

/*
 * Instance variables for view surfaces
 */
 struct gp1ddvars {
	ipt_type ddcp;			/* device coords current point */
	short fillop, fillrastop;	/* rop for region fill, raster */
	short cursorrop;		/* rop for cursor raster */
	short lineop;			/* rop for lines */
	short rtextop;			/* rop for raster text */
	short vtextop;			/* rop for vector text */
	short RAS8func;			/* rop for depth 8 raster */
	char cmsname[CMS_NAMESIZE];
	u_char red[256], green[256], blue[256];	/* colormap */
	short lineindex, textindex, fillindex;	/* color attributes */
	short opcolorset;
	int linewidth, linestyle, polyintstyle;	/* line, poly attrib */
	int ddfont, openfont;			/* text attributes */
	PIXFONT *pf;
	ipt_type ddup, ddpath, ddspace;
	struct pixrect *screen;		/* viewsurf and viewport pixrect */
	short openzbuffer;              /* zbuffer control:
                                           NOZB => no z-buffer
                                           SWZB => software z-buffer
                                           HWZB => hardware z-buffer (GB)
                                        */
	struct pixrect *zbuffer, *cutaway;	/* SWZB only */
	float *xarr, *zarr;             /* NDC cutaway data -- SWZB only*/
	int cutarraysize;               /* no. of cutaway points -- SWZB only */
	int xoff, yoff, scale;		/* NDC to device */
	int maxy, maxz, fullx, fully;
	struct windowstruct wind;
	porttype port;                  /* NDC viewport info */
	int curstrk;
	int lockcount;
	struct gp1_attr *gp1attr;	/* ptr to gp attr list */
};


#define RAWDEV 1


#ifndef NOZB        
                    
/* z-buffer constants */
#define NOZB 0      
#define SWZB 1      
#define HWZB 2      
                    
                    
/* opcolorset constants */
#define LINE_OPCOLOR 0
#define REGION_OPCOLOR 1
#define RASTER_OPCOLOR 2
#define RASTER8_OPCOLOR 3
#define RASTERTEXT_OPCOLOR 4
#define VECTORTEXT_OPCOLOR 5
#define CURSOR_OPCOLOR 6
 
#endif
