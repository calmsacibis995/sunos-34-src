#ifndef lint
static	char sccsid[] = "@(#)label.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include "diag.h"
#include <sun/dklabel.h>

struct dkmaplist {
	char dkm_name[64];		/* partition table name */
	struct dk_map dkm_map[NDKMAP];	/* logical unit disk map */
};

struct dkmaplist dkmap0[] = {
	{
		"Fujitsu-M2312K",
#undef	CYL
#define	CYL	(7*34)
#undef	NCYL
#define	NCYL	587	/* leaves 2 for alternates */
		{ 0,	15884 },		/* a - root */
		{ 67,	33440 },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 208,	(NCYL-208)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Fujitsu-M2284/M2322",
#undef	CYL
#define	CYL	(10*34)
#undef	NCYL
#define	NCYL	821	/* leaves 2 for alternates */
		{ 0,	15884 },		/* a - root */
		{ 47,	33440 },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 146,	(NCYL-146)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Other",
		{ 0,	0 },			/* filled in by user */
	},
	{
		"",				/* Termination of dkmap */
	},
};

struct dkmaplist dkmap1[] = {
	{
		"Fujitsu-M2312K",
#undef	CYL
#define	CYL	(7*32)
#undef	NCYL
#define	NCYL	587	/* leaves 2 for alternates */
		{ 0,	15884 },		/* a - root */
		{ 71,	33440 },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 221,	(NCYL-221)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Fujitsu-M2284/M2322",
#undef	CYL
#define	CYL	(10*32)
#undef	NCYL
#define	NCYL	821	/* leaves 2 for alternates */
		{ 0,	15884 },		/* a - root */
		{ 50,	33440 },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 155,	(NCYL-155)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Other",
		{ 0,	0 },			/* filled in by user */
	},
	{
		"",				/* Termination of dkmap */
	},
};

struct dkmaplist dkmap2[] = {
	{
		"Fujitsu-M2312K",
#undef	CYL
#define	CYL	(7*32)
#undef	NCYL
#define	NCYL	587	/* leaves 2 for alternates */
		{ 0,	71*CYL },		/* a - root */
		{ 71,	150*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 221,	(NCYL-221)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Fujitsu-M2312K Old Type",
#undef	CYL
#define	CYL	(7*32)
#undef	NCYL
#define	NCYL	587	/* leaves 2 for alternates */
		{ 0,	15884 },		/* a - root */
		{ 71,	33440 },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 221,	(NCYL-221)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Fujitsu-M2284/M2322",
#undef	CYL
#define	CYL	(10*32)
#undef	NCYL
#define	NCYL	821	/* leaves 2 for alternates */
		{ 0,	50*CYL },		/* a - root */
		{ 50,	105*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 155,	(NCYL-155)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Fujitsu-M2284/M2322 Old Type",
#undef	CYL
#define	CYL	(10*32)
#undef	NCYL
#define	NCYL	821	/* leaves 2 for alternates */
		{ 0,	15884 },		/* a - root */
		{ 50,	33440 },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 155,	(NCYL-155)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Fujitsu-M2351 Eagle",
#undef	CYL
#define	CYL	(20*46)
#undef	NCYL
#define	NCYL	840	/* leaves 2 for alternates */
		{ 0,	18*CYL },		/* a - root */
		{ 18,	37*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 55,	(NCYL-55)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Fujitsu-M2351 Eagle Old Type",
#undef	CYL
#define	CYL	(20*46)
#undef	NCYL
#define	NCYL	840	/* leaves 2 for alternates */
		{ 0,	15884 },		/* a - root */
		{ 18,	33440 },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 55,	(NCYL-55)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Fujitsu-M2333",
#undef	CYL
#define	CYL	(10*67)
#undef	NCYL
#define	NCYL	821	/* leaves 2 for alternates */
		{ 0,	24*CYL },		/* a - root */
		{ 24,	50*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 74,	(NCYL-74)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Fujitsu-M2361 Eagle",
#undef	CYL
#define	CYL	(20*67)
#undef	NCYL
#define	NCYL	840	/* leaves 2 for alternates */
		{ 0,	12*CYL },		/* a - root */
		{ 12,	25*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 37,	(NCYL-37)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"CDC EMD 9720",
#undef	CYL
#define	CYL	(10*48)
#undef	NCYL
#define	NCYL	1147	/* leaves 2 for alternates */
		{ 0,	34*CYL },		/* a - root */
		{ 34,	70*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 104,	(NCYL-104)*CYL },	/* g - /usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Other",
		{ 0,	0 },			/* filled in by user */
	},
	{
		"",				/* Termination of dkmap */
	},
};

struct dkmaplist dkmap3[] = {
	{
		"Micropolis 1304",
#undef	CYL
#define	CYL	(6*17)
#undef	NCYL
#define	NCYL	825	/* leaves 5 for alternates */
		{ 0,	156*CYL }, 		/* a - root */
		{ 156,	161*CYL },		/* b - swap (small) */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 317,	(NCYL-317)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Micropolis 1304 Old Type",
#undef	CYL
#define	CYL	(6*17)
#undef	NCYL
#define	NCYL	825	/* leaves 5 for alternates */
		{ 0,	15884 }, 		/* a - root */
		{ 156,	161*CYL },		/* b - swap (small) */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 317,	(NCYL-317)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{ 
		"Micropolis 1325",
#undef  CYL
#define CYL	(8*17)
#undef	NCYL
#define	NCYL	(1022-4)/* leaves 2 for alternates (-4 size kludge) */
		{ 0,	117*CYL },		/* a - root */
		{ 117,	246*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 363,	(NCYL-363)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{ 
		"Micropolis 1325 Old Type",
#undef  CYL
#define CYL	(8*17)
#undef	NCYL
#define	NCYL	(1022-4)/* leaves 2 for alternates (-4 size kludge) */
		{ 0,	15884 },		/* a - root */
		{ 117,	33440 },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 363,	(NCYL-363)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Maxtor XT-1050",
#undef	CYL
#define	CYL	(5*17)
#undef	NCYL
#define NCYL	1020	/* leaves 4 for alternates */
		{ 0,	187*CYL }, 		/* a - root */
		{ 187,	193*CYL }, 		/* b - swap (small) */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 380,	(NCYL-380)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Maxtor XT-1050 Old Type",
#undef	CYL
#define	CYL	(5*17)
#undef	NCYL
#define NCYL	1020	/* leaves 4 for alternates */
		{ 0,	15884 }, 		/* a - root */
		{ 187,	193*CYL }, 		/* b - swap (small) */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 380,	(NCYL-380)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{ 
		"Fujitsu M2243AS",
#undef	CYL
#define	CYL	(11*17)
#undef	NCYL
#define	NCYL	(752-12)/* leaves 2 for alternates (-12 size kludge) */
		{ 0,	85*CYL },		/* a - root */
		{ 85,	179*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 264,	(NCYL-264)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{ 
		"Fujitsu M2243AS Old Type",
#undef	CYL
#define	CYL	(11*17)
#undef	NCYL
#define	NCYL	(752-12)/* leaves 2 for alternates (-12 size kludge) */
		{ 0,	15884 },		/* a - root */
		{ 85,	33440 },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 264,	(NCYL-264)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{ 
		"Vertex V185",
#undef	CYL
#define	CYL	(7*17)
#undef	NCYL
#define	NCYL	1163	/* leaves 3 for alternates */
		{ 0,	134*CYL },		/* a - root */
		{ 134,	282*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 416,	(NCYL-416)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{ 
		"Vertex V185 Old Type",
#undef	CYL
#define	CYL	(7*17)
#undef	NCYL
#define	NCYL	1163	/* leaves 3 for alternates */
		{ 0,	15884 },		/* a - root */
		{ 134,	33440 },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 416,	(NCYL-416)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
#ifdef	notdef
	{
		"Vertex V150",
#undef	CYL
#define	CYL	(5*17)
#undef	NCYL
#define	NCYL	984	/* leaves 3 for alternates */
		{ 0,	187*CYL },		/* a - root */
		{ 187,	193*CYL },		/* b - swap (small) */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 380,	(NCYL-380)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Vertex V150 Old Type",
#undef	CYL
#define	CYL	(5*17)
#undef	NCYL
#define	NCYL	984	/* leaves 3 for alternates */
		{ 0,	15884 },		/* a - root */
		{ 187,	193*CYL },		/* b - swap (small) */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 380,	(NCYL-380)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Tandon TM 503",
#undef	CYL
#define	CYL	(5*17)
#undef	NCYL
#define	NCYL	300	/* leaves 6 for alternates */
		{ 0,	0 },			/* a - unused */
		{ 0,	0 },		 	/* b - unused */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 0,	0 },			/* g - unused */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Atasi 3046",
#undef	CYL
#define	CYL	(5*17)
#undef	NCYL
#define	NCYL	640	/* leaves 5 for alternates */
		{ 0,	199*CYL },		/* a - root */
		{ 199,	128*CYL }, 		/* b - swap (small) */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 327,	(NCYL-327)*CYL, },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Atasi 3046 Old Type",
#undef	CYL
#define	CYL	(5*17)
#undef	NCYL
#define	NCYL	640	/* leaves 5 for alternates */
		{ 0,	15884 },		/* a - root */
		{ 199,	128*CYL }, 		/* b - swap (small) */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 327,	(NCYL-327)*CYL, },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Maxtor XT-1140",
#undef	CYL
#define	CYL	(15*17)
#undef	NCYL
#define	NCYL	914	/* leaves 4 for alternates */
		{ 0,	63*CYL }, 		/* a - root */
		{ 63,	132*CYL }, 		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 195,	(NCYL-195)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Maxtor XT-1140 Old Type",
#undef	CYL
#define	CYL	(15*17)
#undef	NCYL
#define	NCYL	914	/* leaves 4 for alternates */
		{ 0,	15884 }, 		/* a - root */
		{ 63,	33440 }, 		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 195,	(NCYL-195)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
#endif
	{
		"Other",
		{ 0,	0 },			/* filled in by user */
	},
	{
		"",				/* Termination of dkmap */
	},
};

struct dkmaplist dkmap4[] = {
	{
		"Micropolis 1355",
#undef  CYL
#define CYL	(8*34)
#undef	NCYL
#define	NCYL	(1018)	/* leaves 2 for alternates */
		{ 0,	59*CYL },		/* a - root */
		{ 59,	123*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 182,	(NCYL-182)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Toshiba MK 156F",
#undef  CYL
#define CYL	(10*34)
#undef	NCYL
#define	NCYL	(815)	/* leaves 2 for alternates */
		{ 0,	47*CYL },		/* a - root */
		{ 47,	99*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 146,	(NCYL-146)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
#ifdef notdef
	{
		"CDC 94166-182",
#undef  CYL
#define CYL	(9*34)
#undef	NCYL
#define	NCYL	(905)	/* leaves 2 for alternates */
		{ 0,	52*CYL },		/* a - root */
		{ 52,	110*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 162,	(NCYL-162)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Fujitsu M2246E",
#undef  CYL
#define CYL	(10*34)
#undef	NCYL
#define	NCYL	(815)	/* leaves 2 for alternates */
		{ 0,	47*CYL },		/* a - root */
		{ 47,	99*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 146,	(NCYL-146)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"Hitachi DK512-17",
#undef  CYL
#define CYL	(10*34)
#undef	NCYL
#define	NCYL	(815)	/* leaves 2 for alternates */
		{ 0,	47*CYL },		/* a - root */
		{ 47,	99*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 146,	(NCYL-146)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
	{
		"NEC D5652",
#undef  CYL
#define CYL	(10*34)
#undef	NCYL
#define	NCYL	(815)	/* leaves 2 for alternates */
		{ 0,	47*CYL },		/* a - root */
		{ 47,	99*CYL },		/* b - swap */
		{ 0,	NCYL*CYL },		/* c - entire disk */
		{ 0,	0 },			/* d - unused */
		{ 0,	0 },			/* e - unused */
		{ 0,	0 },			/* f - unused */
		{ 146,	(NCYL-146)*CYL },	/* g - usr */
		{ 0,	0 },			/* h - unused */
	},
#endif notdef
	{
		"Other",
		{ 0,	0 },			/* filled in by user */
	},
	{
		"",				/* Termination of dkmap */
	},
};

struct dkmaplist *dkmap[] = {
	dkmap0,		/* 2180 */
	dkmap1,		/* XY440 */
	dkmap2,		/* XY450 */
	dkmap3,		/* Adaptec ACB 4000 - SCSI/ST506 */
	dkmap4,		/* CCS_EMULEX - SCSI/ESDI */
	dkmap2,		/* XY751 */
};

struct dkmaplist *currmap = NULL;
struct dk_map currparts[NDKMAP];

islabel(l)
	struct dk_label *l;
{

	if (!ck_cksum(l)) {
		printf("CORRUPT LABEL!!\n");
		return 0;
	}
	if (l->dkl_bhead != basehead) {
		printf("MISPLACED LABEL!!\n");
		return 0;
	}
	return 1;
}

C_verify()
{
	struct dk_label *l = (struct dk_label *)DBUF_VA;
	int found, i, backup = 0;

	printf("verify label\n");
	l->dkl_asciilabel[0] = 0;
	l->dkl_magic = 0;
	(void) devcmd(READ, 0, 0, 0, 1, DBUF_PA);
	if ((l->dkl_magic != DKL_MAGIC) || !islabel(l)) {
		if (l->dkl_magic != DKL_MAGIC)
			printf("NO LABEL!!\n");
		printf("Do you wish to search for backup labels? ");
		if (!confirm())
			return 0;
		for (i = 1; i < 11; i += 2) {
			l->dkl_asciilabel[0] = 0;
			l->dkl_magic = 0;
			if (controller == C_ADAPTEC)
				(void) devcmd(READ, ncyl+acyl-1, 2, i, 1,
					DBUF_PA);
			else
				(void) devcmd(READ, ncyl+acyl-1, nhead-1, i, 1,
					DBUF_PA);
			if ((l->dkl_magic == DKL_MAGIC) && islabel(l))
				break;
		}
		/* this is for old drives - they have labels where the new
		   defect list goes */
		if (controller == C_ADAPTEC && i >= 11) {
			for (i = 1; i < 11; i += 2) {
				l->dkl_asciilabel[0] = 0;
				l->dkl_magic = 0;
				(void) devcmd(READ, ncyl+acyl-1, 1, i, 1,
					DBUF_PA);
				if ((l->dkl_magic == DKL_MAGIC) && islabel(l))
					break;
			}
		}
		if (i >= 11) {
			printf("No backup label found.\n");
			return 0;
		}
		printf("Backup label found.\n");
		backup = 1;
	}
	printf("id: <%s>\n", l->dkl_asciilabel);
	if (scsi == 0 && (basehead != 0 || l->dkl_ppart != 0))
		printf("\tPhysical partition #%d\n", l->dkl_ppart);

	found = 0;
	for (i = 0; i < NDKMAP; i++) {
		if (l->dkl_map[i].dkl_nblk != 0) {
			printf("\tPartition %c: ", 'a'+i);
			printf("starting cyl=%d, # blocks=%d\n",
			    l->dkl_map[i].dkl_cylno, l->dkl_map[i].dkl_nblk);
			found = 1;
		}
		currparts[i] = l->dkl_map[i];
	}
	if (!found) {
		printf("\tNo logical partitions!!\n");
		return 0;
	}
	if (backup) {
		printf("Do you wish to restore the primary label? ");
		if (!confirm())
			return 0;
		(void) devcmd(WRITE, 0, 0, 0, 1, DBUF_PA);
		for (i = 1; i < 11; i += 2)
			if (scsi)
				(void) devcmd(WRITE, ncyl+acyl-1, 1, i, 1,
					DBUF_PA);
			else
				(void) devcmd(WRITE, ncyl+acyl-1, nhead-1, i, 1,
					DBUF_PA);
	}
	return 1;
}

/*
 * itoa - integer to string conversion
 */
char *
itoa(i)
	register int i;
{
	static char b[10] = "########";
	register char *p;

	p = &b[8];
	do
		*p-- = i%10 + '0';
	while (i /= 10);
	return(++p);
}

C_label()
{
	struct dk_label *l = (struct dk_label *)DBUF_VA;
	struct dkmaplist *m;
	int i;
	char *zp;

	printf("label this disk...\n");
	if (currmap == NULL) {
		for (m = dkmap[controller]; m->dkm_name[0] != '\0'; m++) {
			if (strcmp(ascii_id, m->dkm_name) == 0) {
				currmap = m;
				break;
			}
		}
	}
	if (currmap != NULL) {
		printf("OK to use logical partition map '%s'? ",
			currmap->dkm_name);
		if (!confirm())
			currmap = NULL;
	}
	if (currmap == NULL) {
		printf("Use partition command to define logical partitions,\n");
		printf("then re-issue label command.\n");
		return;
	}
	printf("Are you sure you want to write? ");
	if (!confirm())
		return;
	zp = (char *)l;
	for (i = 0; i < sizeof (struct dk_label); i++)
		*zp++ = 0;

	(void) strcpy(l->dkl_asciilabel, ascii_id);
	(void) strcat(l->dkl_asciilabel, " cyl ");
	(void) strcat(l->dkl_asciilabel, itoa(ncyl));
	(void) strcat(l->dkl_asciilabel, " alt ");
	(void) strcat(l->dkl_asciilabel, itoa(acyl));
	(void) strcat(l->dkl_asciilabel, " hd ");
	(void) strcat(l->dkl_asciilabel, itoa(nhead));
	(void) strcat(l->dkl_asciilabel, " sec ");
	(void) strcat(l->dkl_asciilabel, itoa(nsect));

	l->dkl_ncyl = ncyl;
	l->dkl_acyl = acyl;
	l->dkl_nhead = nhead;
	l->dkl_nsect = nsect;
	l->dkl_apc = nspare;
	l->dkl_bhead = basehead;
	l->dkl_ppart = physpart;
	l->dkl_gap1 = gap1;
	l->dkl_gap2 = gap2;
	l->dkl_intrlv = interleave;
	for (i = 0; i < NDKMAP; i++)
		l->dkl_map[i] = currmap->dkm_map[i];
	l->dkl_magic = DKL_MAGIC;
	mk_cksum(l);
	(void) devcmd(WRITE, 0, 0, 0, 1, DBUF_PA);
	for (i = 1; i < 11; i += 2)
		if (controller == C_ADAPTEC)
			(void) devcmd(WRITE, ncyl+acyl-1, 2, i, 1, DBUF_PA);
		else
			(void) devcmd(WRITE, ncyl+acyl-1, nhead-1, i, 1,
				DBUF_PA);
	(void) C_verify();
}

C_partition()
{
	int i, j, change;
	struct dkmaplist *m;

again:
	printf("Select partition table:\n");
	for (i = 0; dkmap[controller][i].dkm_name[0] != '\0'; i++) {
		printf("\t%d - %s\n", i, dkmap[controller][i].dkm_name);
		if (strcmp(dkmap[controller][i].dkm_name, "Other") == 0)
			break;
	}
	j = pgetn("Which one? ");
	if (j < 0 || j > i)
		goto again; 
	m = &dkmap[controller][j];

	if (strcmp(m->dkm_name, "Other") == 0) {
		printf("Name this partition table: ");
		gets(m->dkm_name);
		change = 1;
	} else {
		printf("Do you wish to modify this table? ");
		change = confirm();
	}
	if (change) {
		for (i = 0; i < NDKMAP; i++) {
			printf("Partition %c: ", 'a'+i);
			printf("starting cyl=%d, ",
				m->dkm_map[i].dkl_cylno);
			printf("# of blocks=%d\n",
				m->dkm_map[i].dkl_nblk);
			printf("Change this partition? ");
			if (!confirm())
				continue;
			m->dkm_map[i].dkl_cylno = pgetbn("starting cylinder? ");
			m->dkm_map[i].dkl_nblk = pgetbn("# of blocks? ");
		}
	}

	printf("Verify partition table '%s':\n", m->dkm_name);
	for (i = 0; i < NDKMAP; i++)
		if (m->dkm_map[i].dkl_nblk != 0) {
			printf("\tPartition %c: ", 'a'+i);
			printf("starting cyl=%d, # blocks=%d\n",
				m->dkm_map[i].dkl_cylno,
				m->dkm_map[i].dkl_nblk);
		}
	printf("OK to use this partition table? ");
	if (!confirm())
		goto again;
	currmap = m;
	printf("Use the label command to write out the partition table.\n");
}

ck_cksum(l)
struct dk_label *l;
{
	short *sp, sum = 0;
	short count = sizeof(struct dk_label)/sizeof(short);

	sp = (short *)l;
	while (count--) 
		sum ^= *sp++;
	return (sum ? 0 : 1);
}

mk_cksum(l)
struct dk_label *l;
{
	short *sp, sum = 0;
	short count = sizeof(struct dk_label)/sizeof(short) - 1;

	sp = (short *)l;
	while (count--) 
		sum ^= *sp++;
	l->dkl_cksum = sum;
}
