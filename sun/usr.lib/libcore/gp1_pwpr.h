/*	@(#)gp1_pwpr.h 1.1 86/09/25 Copyr 1985 Sun Micro */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#define XFORMTYPE_CHANGE	0
#define XFORMTYPE_IMAGE		1
#define XFORMTYPE_WLDSCR	2
#define XFORMTYPE_WLDNDC	3

typedef float matrix_3D[4][4];

struct gp1_attr
	{
	short attrchg, clplstchg, needreset;
	int resetcnt;
	int clipid;
	int org_x, org_y;
	int statblkindx;
	short *gp1_addr, *shmptr;
	int offset, count;
	unsigned int bitvec;
	matrix_3D mtxlist[6];
	int xfrmtype;
	float xscale, xoffset, yscale, yoffset, zscale, zoffset;
	float ndcxscale, ndcxoffset, ndcyscale, ndcyoffset, ndczscale, ndczoffset;
	short fbindx, pixplanes, op, color, hiddensurfflag;
	short wldclipplanes, outclipplanes, curclipplanes;
	short hwzbuf, forcerepaint;
	};

/*
  attrchg: indicates whether attributes have changed since last sent to GP
  vwpchg: indicates whether viewport params have changed since last sent to GP
  clplstchg: indicates whether clipping list has changed since last sent to GP
  needreset: indicates whether we have reset GP since most recent SIGXCPU signal
  resetcnt: value of reset counter for GP after most recent SIGXCPU signal
  clipid: value of clipid when clipping list last sent to GP
	  (pixwin viewsurfaces only)
  org_x, org_y: window origin in screen coordinates (useful for pixwin
		viewsurfaces only; set to 0 for pixrect viewsurfaces)
  statblkindx: obtained from the kernel, one per viewsurface; indicates where
	       static data and clipping list is stored for the viewsurface
  gp1_addr, shmptr, offset, count, bitvec: enable doing one allocate and post
		sequence distributed across several subroutines
  mtxlist: cached copies of all 6 GP matrices, if necessary for reset routine
  xfrmtype: CHANGE, IMAGE, 32K, or 1
  xscale, xoffset, yscale, yoffset, zscale, zoffset: NDC to screen
		coordinate transform
  (ndc* are viewport scaling into NDC equivalents of above)
  fbindx: index of cg2 color board for this viewsurface as known to the
	  GP microcode
  pixplanes: per plane mask to be used for primitives (needs to be set from
	     pr or pw attributes
  op: rasterop to be used for primitives
  color: color to be used for primitives
  clipplanes: 
	wld clip planes, output clip planes, wld || output clip planes;
	bit representation of planes to clip against L|R|B|T|H|Y is 5|4|3|2|1|0
  hiddensurfflag: 0 => no hidden surface removal;  1 => use z-buffer
*/
