#ifndef lint
static char sccsid[] = "@(#)gp1_attr.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/window_hs.h>
#include "coretypes.h"
#include "corevars.h"
#include "gp1_pwpr.h"
#include <pixrect/gp1cmds.h>
#include <sys/ioctl.h>
#include <sun/gpio.h>
#include <stdio.h>

struct gp1_attr *_core_gp1_attr_init(pw)
struct pixwin *pw;
	{
	register struct gp1_attr *attrptr;
	struct gp1pr *dmd;
	int statblk;

	dmd = gp1_d(pw->pw_pixrect);
	if ((statblk = _core_gp1_stblk_alloc( dmd->ioctl_fd)) < 0)
		return(0);
	if((attrptr = (struct gp1_attr *) malloc(sizeof(struct gp1_attr))) == 0)
		{
		_core_gp1_stblk_free(dmd->ioctl_fd, statblk);
		return(0);
		}
	attrptr->attrchg = TRUE;
	ioctl(dmd->ioctl_fd, GP1IO_GET_RESTART_COUNT, &attrptr->resetcnt);
	attrptr->attrchg = attrptr->clplstchg = TRUE;
	attrptr->xfrmtype = XFORMTYPE_CHANGE;
	attrptr->needreset = FALSE;
	attrptr->org_x = attrptr->org_y = 0;
	attrptr->statblkindx = statblk;
	attrptr->gp1_addr = (short *) dmd->gp_shmem;
	attrptr->xscale = 0.0;
	attrptr->xoffset = 0.0;
	attrptr->yscale = 0.0;
	attrptr->yoffset = 0.0;
	attrptr->zscale = 0.0;
	attrptr->zoffset = 0.0;
	attrptr->ndcxscale = 0.0;
	attrptr->ndcxoffset = 0.0;
	attrptr->ndcyscale = 0.0;
	attrptr->ndcyoffset = 0.0;
	attrptr->ndczscale = 0.0;
	attrptr->ndczoffset = 0.0;
	attrptr->fbindx = dmd->cg2_index;
	attrptr->pixplanes = 0;
	attrptr->op = 0;
	attrptr->color = 0;
	attrptr->wldclipplanes = _core_outpclip;
	attrptr->outclipplanes = _core_wclipplanes;
	attrptr->curclipplanes = 0;
	attrptr->hiddensurfflag = 0;
	attrptr->forcerepaint = 0;
	attrptr->hwzbuf = 0;

	return(attrptr);
	}


_core_gp1_attr_close(pw, attrptr)
struct pixwin *pw;
struct gp1_attr *attrptr;
	{
	struct gp1pr *dmd;

	dmd = gp1_d(pw->pw_pixrect);
	_core_gp1_stblk_free( dmd->ioctl_fd, attrptr->statblkindx);
	free(attrptr);
	}


_core_gp1_set_op(op, ptr)
int op;
struct gp1_attr *ptr;
	{
	if (op != ptr->op)
		{
		ptr->op = op;
		ptr->attrchg = TRUE;
		}
	}


_core_gp1_set_color(color, ptr)
int color;
struct gp1_attr *ptr;
	{
	if (color != ptr->color)
		{
		ptr->color = color;
		ptr->attrchg = TRUE;
		}
	}


_core_gp1_set_pixplanes(pixplanes, ptr)
int pixplanes;
struct gp1_attr *ptr;
	{
	if (pixplanes != ptr->pixplanes)
		{
		ptr->pixplanes = pixplanes;
		ptr->attrchg = TRUE;
		}
	}


_core_gp1_set_wldclipplanes(clipplanes, ptr)
int clipplanes;
struct gp1_attr *ptr;
	{
	static unsigned char clipplanetbl[8] = {0, 4, 2, 6, 1, 5, 3, 7};

	if (clipplanes != ptr->wldclipplanes) {
		/* 
		 * change order of planes (this should change later)
		 * suncore --- Y|H|T|B|R|L
		 * gp      --- L|R|B|T|H|Y
		 */
		ptr->wldclipplanes = (clipplanetbl[(clipplanes & 7)] << 3) |
			(clipplanetbl[(clipplanes >> 3) & 7]);

		ptr->xfrmtype = XFORMTYPE_CHANGE;
		}
	}


_core_gp1_set_outclipplanes(clipplanes, ptr)
int clipplanes;
struct gp1_attr *ptr;
	{
	if (clipplanes != ptr->outclipplanes) {
		ptr->outclipplanes = clipplanes;
		ptr->xfrmtype = XFORMTYPE_CHANGE;
		}
	}


_core_gp1_set_hiddensurfflag(hiddensurfflag, ptr)
int hiddensurfflag;
struct gp1_attr *ptr;
	{
	if (hiddensurfflag != ptr->hiddensurfflag)
		{
		ptr->hiddensurfflag = hiddensurfflag;
		ptr->attrchg = TRUE;
		}
	}


_core_gp1_set_hwzbuf(hwzbufflag, ptr)
int hwzbufflag;
struct gp1_attr *ptr;
	{
	if (hwzbufflag != ptr->hwzbuf)
		{
		ptr->hwzbuf = hwzbufflag;
		}
	}


_core_gp1_snd_attr(pw, attr)
struct pixwin *pw;
register struct gp1_attr *attr;
	{
	struct gp1pr *dmd;
	register short *shmptr;
	int offset;
	unsigned int bitvec;
	int cnt;

Restart:
	cnt = attr->resetcnt;
	dmd = gp1_d(pw->pw_pixrect);
	while ((offset = gp1_alloc(attr->gp1_addr, 1, &bitvec,
	    dmd->minordev, dmd->ioctl_fd)) == 0)
		;
	shmptr = &(attr->gp1_addr)[offset];
	*shmptr++ = GP1_USEFRAME | attr->statblkindx;
	*shmptr++ = GP1_SETFBINDX | attr->fbindx;
	*shmptr++ = GP1_SETROP;
	*shmptr++ = attr->op;
	*shmptr++ = GP1_SETCOLOR | attr->color;
	*shmptr++ = GP1_SETPIXPLANES | attr->pixplanes;
	*shmptr++ = GP1_SETHIDDENSURF | attr->hiddensurfflag;
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	*((unsigned int *)shmptr) = bitvec;
	if (gp1_post(attr->gp1_addr, offset, dmd->ioctl_fd)) {
		while (cnt == attr->resetcnt)
			;
		goto Restart;
	}
	attr->attrchg = FALSE;
	return(0);
	}


_core_gp1_snd_xfrm_attr(pw, attr, type)
struct pixwin *pw;
register struct gp1_attr *attr;
int type;
	{
	struct gp1pr *dmd;
	register short *shmptr;
	int offset;
	unsigned int bitvec;
	int cnt, matnum;

	if (type == XFORMTYPE_WLDSCR || type == XFORMTYPE_WLDNDC)
		matnum = 0;
	else /* XFORMTYPE_IMAGE */
		matnum = 2;

Restart:
	cnt = attr->resetcnt;
	dmd = gp1_d(pw->pw_pixrect);
	while ((offset = gp1_alloc(attr->gp1_addr, 1, &bitvec,
	    dmd->minordev, dmd->ioctl_fd)) == 0)
		;
	shmptr = &(attr->gp1_addr)[offset];
	*shmptr++ = GP1_USEFRAME | attr->statblkindx;
 	*shmptr++ = GP1_SELECTMATRIX | matnum;

	*shmptr++ = GP1_SETVWP_3D;
	if (type == XFORMTYPE_WLDNDC) {
		*((float *) shmptr)++ = attr->ndcxscale;
		*((float *) shmptr)++ = attr->ndcxoffset;
		*((float *) shmptr)++ = attr->ndcyscale;
		*((float *) shmptr)++ = attr->ndcyoffset;
		*((float *) shmptr)++ = attr->ndczscale;
		*((float *) shmptr)++ = attr->ndczoffset;
	} else { /* XFORMTYPE_IMAGE || XFORMTYPE_WLDSCR*/
		*((float *) shmptr)++ = attr->xscale;
		*((float *) shmptr)++ = attr->xoffset;
		*((float *) shmptr)++ = attr->yscale;
		*((float *) shmptr)++ = attr->yoffset;
		*((float *) shmptr)++ = attr->zscale;
		*((float *) shmptr)++ = attr->zoffset;
	}

	switch (type) {
		case XFORMTYPE_WLDSCR:
			attr->curclipplanes = attr->wldclipplanes |
				attr->outclipplanes;
			break;
		case XFORMTYPE_WLDNDC:
			attr->curclipplanes = attr->wldclipplanes;
			break;
		case XFORMTYPE_IMAGE:
			attr->curclipplanes = attr->outclipplanes;
			break;
		default:
			break;
	}
	*shmptr++ = GP1_SETCLIPPLANES | attr->curclipplanes;
	
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	*((unsigned int *)shmptr) = bitvec;
	if (gp1_post(attr->gp1_addr, offset, dmd->ioctl_fd)) {
		while (cnt == attr->resetcnt)
			;
		goto Restart;
	}
	attr->xfrmtype = type;
	return(0);
	}



_core_gp1_set_matrix_3d( pw, matno, matptr, attr)
struct pixwin *pw;
int matno;
register float *matptr;
struct gp1_attr *attr;
	{
	register short *shmptr;
	register float *mtxlistptr;
	unsigned int bitvec;
	int offset;
	struct gp1pr *dmd;
	int cnt;

Restart:
	cnt = attr->resetcnt;
	dmd = gp1_d( pw->pw_pixrect);
	mtxlistptr = (float *) attr->mtxlist[matno];
	while ((offset = gp1_alloc(attr->gp1_addr, 1, &bitvec,
		dmd->minordev, dmd->ioctl_fd)) == 0)
		;
	shmptr = &(attr->gp1_addr)[offset];
	*shmptr++ = GP1_USEFRAME | attr->statblkindx;
	*shmptr++ = GP1_SET_MATRIX_3D | matno;	/* select matrix number 0-5 */
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*((float *) shmptr)++ = *mtxlistptr++ = *matptr++;
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	*((unsigned int *)shmptr) = bitvec;
	if (gp1_post(attr->gp1_addr, offset, dmd->ioctl_fd))
		{
		while (cnt == attr->resetcnt)
			;
		goto Restart;
		}
	}


_core_gp1_xformpt_2d( pw, npts, pt1, pt2, attr)
struct pixwin *pw;
int npts;
float *pt1, *pt2;
struct gp1_attr *attr;
	{
	register short *shmptr, *ptlist;
	register float *ptr;
	unsigned int bitvec;
	register int i;
	struct gp1pr *dmd;
	int cnt, offset;

	if (npts > 56)
		{
		fprintf(stderr, "Too many points in _core_gp1_xformpt_2d: %d\n",
		    npts);
		return(-1);
		}
Restart:
	cnt = attr->resetcnt;
	dmd = gp1_d( pw->pw_pixrect);
	while ((offset = gp1_alloc(attr->gp1_addr, 1, &bitvec,
		dmd->minordev, dmd->ioctl_fd)) == 0)
		;
	shmptr = &(attr->gp1_addr)[offset];
	*shmptr++ = GP1_USEFRAME | attr->statblkindx;
	*shmptr++ = GP1_SELECTMATRIX | 1;
	*shmptr++ = GP1_XFORMPT_3D;
	*shmptr++ = i = npts;
	ptlist = shmptr;
	ptr = pt1;
	while (i--) {		/* load points to be transformed */
	    *((float *) shmptr)++ = *ptr++;	/* x (transformed in place) */
	    *((float *) shmptr)++ = *ptr++;	/* y (transformed in place) */
	    *((float *) shmptr)++ = 0.0;	/* z (transformed in place) */
	    *((float *) shmptr)++ = 1.0;	/* w (transformed in place) */
	    *shmptr++ = 1;			/* flag (transform not done) */
	}
	attr->xfrmtype = XFORMTYPE_CHANGE;
	*shmptr++ = GP1_EOCL;
	if (gp1_post(attr->gp1_addr, offset, dmd->ioctl_fd))
		{
		while (cnt == attr->resetcnt)
			;
		goto Restart;
		}
	i = npts;
	ptr = pt2;
	while ( i--) {		/* read transformed points */
				/* wait til transform done */
		if (_core_gp1_chkloc(&ptlist[8], dmd->ioctl_fd))
			{
			while (cnt == attr->resetcnt)
				;
			goto Restart;
			}
		*ptr++ = *((float *) ptlist)++;
		*ptr++ = *((float *) ptlist)++;
		ptlist += 5;
	}
	cnt = attr->resetcnt;
	shmptr = &(attr->gp1_addr)[offset];
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	*((unsigned int *)shmptr) = bitvec;
	if (gp1_post(attr->gp1_addr, offset, dmd->ioctl_fd))
		{
		while (cnt == attr->resetcnt)
			;
		}
	return(0);
	}



_core_gp1_xformpt_3d( pw, npts, pt1, pt2, attr)
struct pixwin *pw;
int npts;
float *pt1, *pt2;
struct gp1_attr *attr;
	{
	register short *shmptr, *ptlist;
	register float *ptr;
	unsigned int bitvec;
	register int i;
	struct gp1pr *dmd;
	int cnt, offset;

	if (npts > 56)
		{
		fprintf(stderr, "Too many points in _core_gp1_xformpt_3d: %d\n",
		    npts);
		return(-1);
		}
Restart:
	cnt = attr->resetcnt;
	dmd = gp1_d( pw->pw_pixrect);
	while ((offset = gp1_alloc(attr->gp1_addr, 1, &bitvec,
		dmd->minordev, dmd->ioctl_fd)) == 0)
		;
	shmptr = &(attr->gp1_addr)[offset];
	*shmptr++ = GP1_USEFRAME | attr->statblkindx;
	*shmptr++ = GP1_SELECTMATRIX | 1;
	*shmptr++ = GP1_XFORMPT_3D;
	*shmptr++ = i = npts;
	ptlist = shmptr;
	ptr = pt1;
	while (i--) {		/* load points to be transformed */
	    *((float *) shmptr)++ = *ptr++;	/* x (transformed in place) */
	    *((float *) shmptr)++ = *ptr++;	/* y (transformed in place) */
	    *((float *) shmptr)++ = *ptr++;	/* z (transformed in place) */
	    *((float *) shmptr)++ = *ptr++;	/* w (transformed in place) */
	    *shmptr++ = 1;			/* flag (transform not done) */
	}
	attr->xfrmtype = XFORMTYPE_CHANGE;
	*shmptr++ = GP1_EOCL;
	if (gp1_post(attr->gp1_addr, offset, dmd->ioctl_fd))
		{
		while (cnt == attr->resetcnt)
			;
		goto Restart;
		}
	i = npts;
	ptr = pt2;
	while ( i--) {		/* read transformed points */
				/* wait til transform done */
		if (_core_gp1_chkloc(&ptlist[8], dmd->ioctl_fd))
			{
			while (cnt == attr->resetcnt)
				;
			goto Restart;
			}
		*ptr++ = *((float *) ptlist)++;
		*ptr++ = *((float *) ptlist)++;
		*ptr++ = *((float *) ptlist)++;
		*ptr++ = *((float *) ptlist)++;
		ptlist++;
	}
	cnt = attr->resetcnt;
	shmptr = &(attr->gp1_addr)[offset];
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	*((unsigned int *)shmptr) = bitvec;
	if (gp1_post(attr->gp1_addr, offset, dmd->ioctl_fd))
		{
		while (cnt == attr->resetcnt)
			;
		}
	return(0);
	}



_core_gp1_matmul_3d( pw, m1, m2, m3, attr)
struct pixwin *pw;
float *m1, *m2, *m3;
struct gp1_attr *attr;
	{
	register short *shmptr, *result;
	register float *mptr;
	unsigned int bitvec;
	register int offset, i;
	struct gp1pr *dmd;
	int cnt;

Restart:
	cnt = attr->resetcnt;
	dmd = gp1_d( pw->pw_pixrect);
	while ((offset = gp1_alloc(attr->gp1_addr, 1, &bitvec,
		dmd->minordev, dmd->ioctl_fd)) == 0)
		;
	shmptr = &(attr->gp1_addr)[offset];
	*shmptr++ = GP1_USEFRAME | attr->statblkindx;
	*shmptr++ = GP1_SET_MATRIX_3D | 4;	/* matrix #4 is m1 */
	i = 4;
	mptr = m1;
	while (i--) {
		*((float *) shmptr)++ = *mptr++;
		*((float *) shmptr)++ = *mptr++;
		*((float *) shmptr)++ = *mptr++;
		*((float *) shmptr)++ = *mptr++;
	}
	*shmptr++ = GP1_SET_MATRIX_3D | 5;	/* matrix #5 is m2 */
	i = 4;
	mptr = m2;
	while (i--) {
		*((float *) shmptr)++ = *mptr++;
		*((float *) shmptr)++ = *mptr++;
		*((float *) shmptr)++ = *mptr++;
		*((float *) shmptr)++ = *mptr++;
	}
	*shmptr++ = GP1_MATMUL_3D;		/* matrix #3 = m1 * m2 */
	*shmptr++ = 4;
	*shmptr++ = 5;
	*shmptr++ = 3;
	*shmptr++ = GP1_GETMATRIX_3D | 3;	/* matrix #3 is result */
	result = shmptr;
	*shmptr++ = 1;				/* get matrix not done */
	shmptr += 32;				/* result goes here */
	*shmptr++ = GP1_EOCL;
	if (gp1_post(attr->gp1_addr, offset, dmd->ioctl_fd))
		{
		while (cnt == attr->resetcnt)
			;
		goto Restart;
		}
						/* wait til matmul done */
	if (_core_gp1_chkloc(result, dmd->ioctl_fd))
		{
		while (cnt == attr->resetcnt)
			;
		goto Restart;
		}
	result++;
	i = 4;
	mptr = m3;
	while (i--) {
		*mptr++ = *((float *) result)++;
		*mptr++ = *((float *) result)++;
		*mptr++ = *((float *) result)++;
		*mptr++ = *((float *) result)++;
	}
	cnt = attr->resetcnt;
	shmptr = &(attr->gp1_addr)[offset];
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	*((unsigned int *)shmptr) = bitvec;
	if (gp1_post(attr->gp1_addr, offset, dmd->ioctl_fd))
		{
		while (cnt == attr->resetcnt)
			;
		}
	return(0);
	}
