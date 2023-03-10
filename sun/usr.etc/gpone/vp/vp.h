| "@(#)vp.h 1.6 87/01/08"

| Copyright 1986 by Sun Microsystems, Inc.


#ifdef SCREEN1152
#define GP1_CODE_VER	0x0100
#else
#define GP1_CODE_VER	0x0101
#endif

| *** IMPORTANT:  THE VERSION INFORMATION MUST BE CHANGED WITH EVERY NEW VERSION OF MICROCODE RELEASED TO *SOMEONE*
|
|	The SW_RELEASE is defined down as follows:
|		upper byte 	major release number
|		lower byte	minor release number

|	The SERIAL_NUM is defined as follows:
|		upper byte	code version within minor release (starts at 1)
|		lower byte	flags		bit 0 - released to customers (if customer can call us, set bit)
|						bit 1 - release engineering (version handled through release engineering)
|						bit 2 - FCS
|						bit 3 - requires gp+

#define SW_RELEASE 0x0304	/* installed on buda/pest 1/8/87 */
#define SERIAL_NUM 0x0103

| Origin of PP microcode
#define PP_ORG	0x1123

| Default board addresses  (top 8 bits of 24-bit VME address)
#define GP1_DEFAULT_ADDR	0x21
#define GP1_NOTINSTALLED	0xFFFF
#define CG2_DEFAULT_ADDR	0x40
#define CG2_NOTINSTALLED	0xFFFF

| Graphics Buffer inquiry routine constants:
| 0xFFFF if no gb, otherwise index (0-3) of corresponding cg2 board
#define GB_CG2_0	0
#define GB_CG2_1	1
#define GB_CG2_2	2
#define GB_CG2_3	3
#define GB_NOTINSTALLED	0xFFFF

| Naming convention:  TEX alone refers to 1-dimensional linear textures.
| TEX2 refers to 2-dimensional textures, either 1-bit or 8-bit.
#define TEX2_SXSY	0
#define TEX2_2D		1
#define TEX2_3D		2

| Global static block offsets (block 0)
#define VERSIONFLG	2
#define VERSIONLOC	508
#define TEXPPFRMPTR	510
#define TEX2DPPFRM_GL	511
    /* Ptr or block number of 2d texture block stored on pp. */

| Static frame parameter offsets
#define FBINDX_FRMOFF		0
#define PIXPLNS_FRMOFF		1
#define ROP_FRMOFF		2
#define COLOR_FRMOFF		3
#define CLPPLNS_FRMOFF		4
#define MATRIXPTR_FRMOFF	5
#define HIDDENSURF_FRMOFF	6
#define TEX2DEPTH_FRMOFF	7
#define TEX2WIDTH_FRMOFF	8
#define TEX2HEIGHT_FRMOFF	9
#define TEX2FRAME_FRMOFF	10
#define TEX2COL0_FRMOFF		11
#define TEX2SX_FRMOFF		12
#define TEX2SY_FRMOFF		13
#define TEX2OFFX2_FRMOFF	14
#define TEX2OFFY2_FRMOFF	16
#define TEX2OFFX3_FRMOFF	18
#define TEX2OFFY3_FRMOFF	20
#define TEX2OFFZ3_FRMOFF	22
#define TEX2OFFKIND_FRMOFF	24
#define STOFF_FRMOFF		26
#define OPTIONS_FRMOFF		27
#define TEX_FRMOFF		28
#define WID_FRMOFF		44
#define VWPXSCL_FRMOFF		52
#define VWPXOFF_FRMOFF		54
#define VWPYSCL_FRMOFF		56
#define VWPYOFF_FRMOFF		58
#define VWPZSCL_FRMOFF		60
#define VWPZOFF_FRMOFF		62
#define XFRM_FRMOFF		64
#define RECTS_FRMOFF		256

#define TempFPAddr	230
    /* Larger of DUMMYEDGE constants from xfpolygon{2,3}.vp.u */

| PP Commands
#define PPACK		1
#define PPROPTEX1	2
#define PPINIT		3
#define PPROPNULL	4
#define PPROPFB		5

#define PPFBADDR	6
#define PPFLASHY	7
#define PPMASK		8
#define PPVEC		9
#define PPSTCLIP	10

#define PPSTCOLOR	11
#define PPVECXFINIT	12
#define PPVECXF		13
#define PPCOLMAPO	14
#define PPSCRBUF	15

#define PPSETBUF	16
#define PPBUFSCR	17
#define PPPLGINIT	18
#define PPPLHINIT	19
#define PPFILLINIT	20

#define PPSHINIT	21
#define PPPLGFILL	22
#define PPPLHFILL	23
#define PPPLGSH		24
#define PPPLHSH		25

#define PPPRLINE	26
#define PPLDTEX		27
#define PPPLGTEX1	28
#define PPPLGTEX8	29
#define PPPRPOLYLINE	30

#define PPXFLINEINIT	31
#define PPXFLINE	32
#define PPSETLINETEX	33
#define PPSETLINEWID	34
#define PPROPTEX8	35


#define IMM
#define pa adda,la	/* Weitek pipeline advance operation. */
#define noflop adda	/* Weitek nop. */
