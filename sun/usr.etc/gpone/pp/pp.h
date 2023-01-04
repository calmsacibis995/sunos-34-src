| @(#)pp.h 1.1 86/09/25 SMI

| Copyright (c) 1985 by Sun Microsystems, Inc.

/* pp.h
   Standard definitions for paint processor.
*/

#include "macros.pp.h"

#ifndef SWIDTH
#include "s1152.pp.h"
#else
#if SWIDTH == 1152
#include "s1152.pp.h"
#else
#if SWIDTH == 1024
#include "s1024.pp.h"
#endif
#endif
#endif


/* Device Constants */
#define GPVWord	0x73	/* contains VME Address modifier bits: 111x011.  Last bit for word. */
	/* last bit is 1 for word mode. */
#define VByte	0x60	/* contains VME Address modifier bits: 11xx0x0. */
	/* last bit is 0 for byte mode. */
#define VWord	0x61	/* contains VME Address modifier bits: 11xx0x1. */
	/* last bit is 1 for word mode. */
#define USMul	0x1e	/* 11110 = Read low product first, 2's complement, format adjust = 32 bit product. */
#define RopFB	0x20	/* Frame buffer offset from board base address for rop mode. */


/* Standard definitions for paint processor to duplicate pixrect
    defines.
*/

/* From cg2reg.h */
#define PRWWRD	0
#define SRWPIX	1
#define PWWWRD	2
#define SWWPIX	3
#define PRRWRD	4
#define PRWPIX	5
#define PWRWRD	6
#define PWWPIX	7

/* Low addresses */
#define FBStatus	0x9000
#define FBPPMask	0xa000

#define FBAllRopRegSrc1		0x8002
#define FBAllRopRegSrc2		0x8004
#define FBAllRopRegPatt		0x8006
#define FBAllRopRegMask1	0x8008
#define FBAllRopRegMask2	0x800a
#define FBAllRopRegShift	0x800c
#define FBAllRopRegOp		0x800e
#define FBAllRopRegWidth	0x8010
#define FBAllRopRegOpCnt	0x8012

#define FBAllRopPrimeSrc1	0x8802
#define FBAllRopPrimeSrc2	0x8804


/* Scratch Memory addresses.  Starting at zero is true scratch.
    It should be used from zero on up
    lest more locations be allocated from SavePts down.
*/

/* Attributes for textured vectors */
#define NumsegsAddr	0x185
#define PatlnAddr	0x186
#define OffsetAddr	0x187
#define OptionsAddr	0x188
#define WidthAddr	0x189
#define WOptionsAddr	0x18a
#define	TexWidth	0x18b
#define TexHeight	0x18c
#define GPAddress	0x18d
#define DebugLevel	0x18e
#define LastCmd		0x18f

#define SavePts		0x190
    /* At least ten locations.  Used for restoring values for clipping rectangles. */

#define LMaskTable	0x1a0
#define RMaskTable	0x1b0
#define RRValues	0x1c0
#define RRValPatt	0x1c1
#define RRValm2		0x1c2
#define RRValm1		0x1c3
#define RRValWid	0x1c4
    /* Raster-op values reserved from 0x1c0 thru 0x1c7. */

/* Color maps */
#define RMap		0x1d0
#define GMap 		0x1e0
#define BMap		0x1f0
#define RedMap		0x200
#define GreenMap	0x300
#define BlueMap		0x400

#define ClipList	0x500
    /* 0x500 to 0x5ff reserved for clip list (about 60 rectangles). */

#define PatternAddr	0x600
    /* 0x100 values. */
#define PatCorrAddr	0x700
    /* 0x100 values. */

/* Two D texture. */
#define Texture		0x800
    /* Reserved to 0xfff.  Will hold 64 x 32 8-bit pixels. */
