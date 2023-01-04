/*	@(#)usercore.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef TRUE
#define TRUE		1
#endif
#ifndef FALSE
#define FALSE		0
#endif

#define STRING		0
#define CHARACTER	1
#define MAXVSURF	5	/* view surfaces; maximum number of */
#define PARALLEL	0	/* transform constants */
#define PERSPECTIVE	1
#define NONE		1	/* segment types */
#define XLATE2		2
#define XFORM2		3
#define XLATE3		2
#define XFORM3		3
#define SOLID		0	/* line styles */
#define DOTTED		1
#define DASHED		2
#define DOTDASHED	3
#define CONSTANT	0	/* polygon shading modes */
#define GOURAUD		1
#define PHONG		2
#define PICK		0	/* input device constants */
#define KEYBOARD	1
#define BUTTON		2
#define LOCATOR		3
#define VALUATOR	4
#define STROKE		5
#define ROMAN		0	/* vector font select constants */
#define GREEK		1
#define SCRIPT		2
#define OLDENGLISH	3
#define STICK		4
#define SYMBOLS		5
#define GALLANT		0	/* raster font constants */
#define GACHA		1
#define SAIL		2
#define GACHABOLD	3
#define CMR		4
#define CMRBOLD		5
#define OFF		0	/* char justify constants */
#define LEFT		1
#define CENTER		2
#define RIGHT		3
#define NORMAL		0	/* rasterop selection */
#define XORROP		1
#define ORROP		2
#define PLAIN		0	/* polygon interior style */
#define SHADED		1
#define BASIC		0	/* Core output levels */
#define BUFFERED	1
#define DYNAMICA	2
#define DYNAMICB	3
#define DYNAMICC	4
#define NOINPUT		0	/* Core input levels */
#define SYNCHRONOUS	1
#define COMPLETE	2
#define TWOD		0	/* Core dimensions */
#define THREED		1

#ifndef lint
static struct {			/* default primitive attributes */
	int lineindx;
	int fillindx;
	int textindx;
	int linestyl;
	int polyintstyl;
	int polyedgstyl;
	float linwidth;
	int pen;
	int font;
	float chwidth,chheight;
	float chup[4], chpath[4], chspace[4];
	int chjust;
	int chqualty;
	int marker;
	int pickid;
	int rasterop;
	} PRIMATTS = {1,1,1,SOLID,PLAIN,SOLID,0.0,0,STICK,11.,11.,
		{0.,1.,0.,1.},{1.,0.,0.,1.}, {0.,0.,0.,1.},
		OFF,STRING,42,0,NORMAL};
#endif

#define DEVNAMESIZE 20

struct vwsurf	{
		char screenname[DEVNAMESIZE];
		char windowname[DEVNAMESIZE];
		int windowfd;
		int (*dd)();
		int instance;
		int cmapsize;
		char cmapname[DEVNAMESIZE];
		int flags;
		char **ptr;
		};

struct suncore_raster {
		int width;
		int height;
		int depth;
		short *bits;
		};

#define NULL_VWSURF {"", "", 0, 0, 0, 0, "", 0, 0}
#define DEFAULT_VWSURF(ddname) {"", "", 0, ddname, 0, 0, "", 0, 0}
#define VWSURF_NEWFLG	1
