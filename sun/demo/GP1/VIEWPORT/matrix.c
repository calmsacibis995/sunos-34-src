#ifndef lint
static	char sccsid[] = "@(#)matrix.c 1.5 87/02/23 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/**********
 *
 *
 *	@   @   @@@   @@@@@  @@@@    @@@   @   @
 *	@@ @@  @   @    @    @   @    @     @ @
 *	@ @ @  @@@@@    @    @@@@     @      @
 *	@   @  @   @    @    @   @    @     @ @
 *	@   @  @   @    @    @   @   @@@   @   @
 *
 *	MATRIX - Matrix manipulation routines
 *	Set_Matrix(vpobj, n, matrix) 	set matrix #n
 *	Get_Matrix(vpobj, n, matrix) 	get matrix #n
 *	Mul_Matrix(vpobj, a, b, c)	multiply matrices
 *	Xform_Matrix(vpobj, n, p)	multiply point P by matrix #n
 *
 *	Trans_Matrix(vp, p, m)		make translation matrix
 *	Scale_Matrix(vp, p, m)		make scaling matrix
 *	RotX_Matrix(vp, a, m)		make X axis rotation matrix
 *	RotY_Matrix(vp, a, m)		make Y axis rotation matrix
 *	RotZ_Matrix(vp, a, m)		make Z axis rotation matrix
 *
 *
 **********/
#include "gpbuf_int.h"
#include <math.h>

static	MATRIX	mat = {
	1.0,	0.0,	0.0,	0.0,
	0.0,	1.0,	0.0,	0.0,
	0.0,	0.0,	1.0,	0.0,
	0.0,	0.0,	0.0,	1.0 };

static	short	MatBits[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20 };

/****
 *
 * Set_Matrix(gpbobj, n, matrix)
 * Copy matrix MATRIX matrix number N. If 2D coordinates are being
 * used, a 2D matrix is copied. Currently integer 2D coordinates use a
 * 3D matrix because they are implemented as 3D vectors (there is no
 * integer 2D).
 *
 ****/
Set_Matrix(gpbobj, n, matrix)
/*    --needs--			 */
	GPBUF	gpbobj;
	int	n;		/* matrix number to set */
FAST	MATRIX	matrix;		/* matrix array to download */
{
/*    --calls--			 */
extern	void	gp_winview();	/* GPBUF set window viewing parameters */
local	int	gp_matrix_set(); /* MATRIX download matrix */
local	short	MatBits[];	/* matrix update bit mask table */
FAST	GPDATA	*gpb;

	gpb = ObjAddr(gpbobj, GPDATA);
	if (gpb->gpb_type != VP_GP1) return;	/* not a GP? */
	gpb->gpb_vstart = 0;			/* close poly/vector list */
	gp_matrix_set(gpb, n, matrix);		/* download matrix */
	if ((n > 1) && (n == gpb->gpb_attr.gpa_MATRIX))
	  {
	   if (gpb->gpb_flags & GPB_XFORM_FLAG) gp_winview(gpb);
	   else Mul_Matrix(gpbobj, n, GPB_VIEWMATRIX, GPB_XFORMMATRIX);
	  }
}

gp_matrix_set(gpb, n, matrix)
/*    --needs--			 */
FAST	GPDATA	*gpb;
	int	n;		/* matrix number to set */
FAST	MATRIX	matrix;		/* matrix array to download */
{
	if (!ROOMFOR(sizeof(short) + sizeof(MATRIX)))
	   gp_flush(gpb);			/* make room for SETMATRIX */
	gpb->gpb_matupd |= MatBits[n];		/* set matrix update bit */
	if (Type3D(gpb->gpb_coord))		/* 3D coordinates? */
	  {
	   PUT_CMD(gpb->gpb_ptr, GP1_SET_MATRIX_3D, n);
	   bcopy((char *) matrix, gpb->gpb_ptr, sizeof(MATRIX)); 
	   gpb->gpb_ptr += sizeof(MATRIX) / sizeof(short);
	   USEUP(sizeof(short) + sizeof(MATRIX));
	  }
	else					/* 2D coordinates */
	  {
	   FAST short *p;

	   p = gpb->gpb_ptr;
	   PUT_CMD(p,  GP1_SET_MATRIX_2D, n);
	   PUT_FLOAT(p, matrix[0][0]);
	   PUT_FLOAT(p, matrix[0][1]);
	   PUT_FLOAT(p, matrix[1][0]);
	   PUT_FLOAT(p, matrix[1][1]);
	   PUT_FLOAT(p, matrix[3][0]);
	   PUT_FLOAT(p, matrix[3][1]);
	   USEUP(sizeof(short) + 6 * sizeof(float));
	   gpb->gpb_ptr = p;
	  }
}

/****
 *
 * Get_Matrix(gpbobj, n, matrix)
 * Copy GP matrix number N into MATRIX. If 2D coordinates are being
 * used, a MATRIX2D is copied. Currently integer 2D coordinates use a
 * 3D matrix because they are implemented as 3D vectors (there is no
 * integer 2D).
 *
 ****/
Get_Matrix(gpbobj, n, matrix)
/*    --needs--			 */
	GPBUF	*gpbobj;
	int	n;		/* matrix number to set */
FAST	MATRIX	matrix;		/* array to contain uploaded matrix */
{
/*    --uses--			 */
extern	int	vpdbg;

FAST	GPDATA	*gpb;
FAST	PTR	ptr;
FAST	short	*done;

	gpb = ObjAddr(gpbobj, GPDATA);
	if (gpb->gpb_type != VP_GP1) return;	/* not a GP? */
	gpb->gpb_vstart = 0;			/* close poly/vector list */
	Flush_VP(gpbobj);			/* start with new buffer */
	ptr.sh = gpb->gpb_ptr;			/* -> top of buffer */
	if (Type3D(gpb->gpb_coord))		/* 3D coordinates? */
	  {
	   PUT_CMD(ptr.sh, GP1_GETMATRIX_3D, n);	/* matrix to get */
	   done = ptr.sh;			/* -> ready flag */
	   PUT_SHORT(ptr.sh, 1);			/* set ready flag */
	   ptr.fl += 16;			/* skip 4x4 matrix area */
           PUT_CMD(ptr.sh, GP1_EOCL, 0);		/* end, don't free */
	   POSTGPBUF(gpb, gpb->gpb_cmdofs);
	   gp1_wait0(done, gpb->gpb_gfd);	/* wait to get matrix */
	   bcopy(done + 1, matrix, sizeof(MATRIX));
	  }
	else					/* 2D coordinates */
	  {
	   PUT_CMD(ptr.sh, GP1_GETMATRIX_2D, n);	/* matrix to get */
	   done = ptr.sh;			/* -> ready flag */
	   PUT_SHORT(ptr.sh, 1);			/* set ready flag */
	   ptr.fl += 6;				/* skip 3x2 matrix area */
           PUT_CMD(ptr.sh, GP1_EOCL, 0);		/* end, don't free */
	   POSTGPBUF(gpb, gpb->gpb_cmdofs);
	   gp1_wait0(done, gpb->gpb_gfd);	/* wait to get matrix */
	   ptr.fl = (float *) (done + 1);	/* -> matrix result */
	   bzero(matrix, sizeof(MATRIX));	/* zero it out first */
	   matrix[0][0] = GET_FLOAT(ptr.fl);
	   matrix[0][1] = GET_FLOAT(ptr.fl);
	   matrix[1][0] = GET_FLOAT(ptr.fl);
	   matrix[1][1] = GET_FLOAT(ptr.fl);
	   matrix[3][0] = GET_FLOAT(ptr.fl);
	   matrix[3][1] = GET_FLOAT(ptr.fl);
	   matrix[3][2] = 1.0;
	  }
#ifdef	DEBUG
	if (vpdbg == 0) return;
       {
	short *saveptr;

	saveptr = gpb->gpb_ptr;
	gpb->gpb_ptr = ptr.sh;
	Print_VP(gpbobj);
	gpb->gpb_ptr = saveptr;
       }
#endif
}

/****
 *
 * Mul_Matrix(gpbobj, a, b, c)
 * Multiply matrices #A and #B together and store in #C.
 *
 ****/
Mul_Matrix(gpbobj, a, b, c)
/*    --needs--			 */
	GPBUF	gpbobj;
	short	a, b, c;	/* matrix numbers to multiply */
{
/*    --calls--			 */
extern	void	gp_winview();	/* GPBUF set window viewing parameters */
extern	int	gp_matrix_mul(); /* MATRIX multiply GP matrices */

/*    --uses--			 */
FAST	GPDATA	*gpb;

	gpb = ObjAddr(gpbobj, GPDATA);
	if (gpb->gpb_type != VP_GP1) return;	/* check its a GP */
	gpb->gpb_vstart = 0;			/* close poly/vector list */
	if (c == gpb->gpb_attr.gpa_MATRIX)
	  {
	   if (gpb->gpb_flags & GPB_XFORM_FLAG) gp_winview(gpb);
	   gp_matrix_mul(gpb, a, b, c);
	   gp_matrix_mul(gpb, c, GPB_VIEWMATRIX, GPB_XFORMMATRIX);
	  }
	else gp_matrix_mul(gpb, a, b, c);
}

gp_matrix_mul(gpb, a, b, c)
/*    --needs--			 */
FAST	GPDATA	*gpb;
	short	a, b, c;	/* matrix numbers to multiply */
{
/*    --uses--			 */
FAST	short	*ptr;
	short	op;

	if ((gpb->gpb_left -= 8 * sizeof(short)) <= 0)
	  {
	   gp_flush(gpb);
	   gpb->gpb_left -= 8 * sizeof(short);	/* count MULMATRIX size */
	  }
	ptr = gpb->gpb_ptr;
	PUT_CMD(ptr, op = Type3D(gpb->gpb_coord) ?
		GP1_MUL_MAT_3D : GP1_MUL_MAT_2D, 0);
	PUT_SHORT(ptr, a);			/* matrices to multiply */
	PUT_SHORT(ptr, b);
	PUT_SHORT(ptr, c);
	gpb->gpb_matupd |= MatBits[c];		/* set matrix update bit */
	gpb->gpb_ptr = ptr;			/* update pointer */
}

/****
 *
 * Xform_Matrix(gpbobj, n, p)
 * Transform POINT p by matrix #N. All calculations are done in floating
 * coordinates. The current transformation matrix is temporarily usurped
 * and replaced by matrix #N. The POINT structure passed will contain the
 * transformed coordinates. If P is NULL, the pen is used as the POINT.
 *
 * notes:
 * The gpb_matupd field is used to maintain flags that indicate which
 * matrices have been changed since the last time the buffer was flushed.
 * If we are using a matrix for transformation purposes, we have to
 * make sure that we flush the buffer before we access the matrix.
 *
 ****/
Xform_Matrix(gpbobj, n, p)
/*    --needs--			 */
	GPBUF	gpbobj;
	int	n;		/* matrix number to transform by */
FAST	POINT	*p;		/* POINT to transform */
{
/*    --uses--			 */
extern	int	vpdbg;
FAST	GPDATA	*gpb;
FAST	PTR	ptr, result;
FAST	short	*done;		/* -> ready flag */
	short	*bufofs;	/* -> start of new GP buffer */
	int	cmdofs, bv;	/* command offset, bit vector */

	gpb = ObjAddr(gpbobj, GPDATA);
	if (gpb->gpb_type != VP_GP1) return;	/* not a GP? */
	gpb->gpb_vstart = 0;			/* close poly/vector list */
	if (gpb->gpb_matupd) Flush_VP(gpbobj);	/* post matrix updates */
	NEWGPBUF(gpb, bufofs, cmdofs, bv);	/* allocate GP buffer */
	ptr.sh = bufofs;
	PUT_CMD(ptr.sh, GP1_SELECTMATRIX, n);	/* use matrix N */
	result.sh = ptr.sh + 2;			/* -> where to put result */
	if (p == NULL) p = &(gpb->gpb_pen);	/* use the pen */
/*
 * Copy the coordinates to transform into the GP.
 */
	switch (gpb->gpb_coord)			/* what kind of point? */
	  {
	   case VP_INT2D:
	   PUT_CMD(ptr.sh, GP1_MUL_POINT_INT_2D, 0);
	   PUT_SHORT(ptr.sh, 1);
	   PUT_INT(ptr.in, p->i.x);		/* integer coordinates */
	   PUT_INT(ptr.in, p->i.y);
	   break;

	   case VP_INT3D:
	   PUT_CMD(ptr.sh, GP1_MUL_POINT_INT_3D, 0);
	   PUT_SHORT(ptr.sh, 1);
	   PUT_INT(ptr.in, p->i.x);		/* integer coordinates */
	   PUT_INT(ptr.in, p->i.y);
	   PUT_INT(ptr.in, p->i.z);
	   PUT_INT(ptr.in, 1);
	   break;

	   case VP_FLT3D:
	   PUT_CMD(ptr.sh, GP1_MUL_POINT_FLT_3D, 0);
	   PUT_SHORT(ptr.sh, 1);
	   PUT_FLOAT(ptr.fl, p->f.x);		/* floating coordinates */
	   PUT_FLOAT(ptr.fl, p->f.y);
	   PUT_FLOAT(ptr.fl, p->f.z);
	   PUT_FLOAT(ptr.fl, 1.0);
	   break;

	   case VP_FLT2D:
	   PUT_CMD(ptr.sh, GP1_MUL_POINT_FLT_2D, 0);
	   PUT_SHORT(ptr.sh, 1);
	   PUT_FLOAT(ptr.fl, p->f.x);		/* floating coordinates */
	   PUT_FLOAT(ptr.fl, p->f.y);
	  }
/*
 * Finish the packet and post to the GP. Wait for confirmation.
 */
	done = ptr.sh;				/* -> ready flag */
	PUT_SHORT(ptr.sh, 1);
	PUT_CMD(ptr.sh, GP1_SELECTMATRIX, 0);	/* back to original matrix */
	PUT_CMD(ptr.sh, GP1_EOCL, 0);		/* end, don't free */
	POSTGPBUF(gpb, cmdofs);			/* post the buffer */
	gp1_wait0(done, gpb->gpb_gfd);		/* wait to get matrix */
#ifdef	DEBUG
	if (vpdbg)
	   if (TypeFlt(gpb->gpb_coord))
	      printf("Xform_Matrix: %f %f %f 1.0\n", p->f.x, p->f.y, p->f.z);
	   else printf("Xform_Matrix: %d %d %d 1\n", p->i.x, p->i.y, p->i.z);
#endif
/*
 * Copy the result into the original POINT area
 */
	switch (gpb->gpb_coord)			/* what kind of point? */
	  {
	   case VP_INT3D:
	   p->i.x = GET_INT(result.in);
	   p->i.y = GET_INT(result.in);
	   p->i.z = GET_INT(result.in);
	   break;

	   case VP_INT2D:
	   p->i.x = GET_INT(result.in);
	   p->i.y = GET_INT(result.in);
	   break;

	   case VP_FLT3D:
	   p->f.x = GET_FLOAT(result.in);
	   p->f.y = GET_FLOAT(result.in);
	   p->f.z = GET_FLOAT(result.in);
	   break;

	   case VP_FLT2D:
	   p->f.x = GET_FLOAT(result.in);
	   p->f.y = GET_FLOAT(result.in);
	   break;
	  }

#ifdef	DEBUG
	if (vpdbg)
	  {
	   short *saveptr;
	   int	saveofs;

	   saveptr = gpb->gpb_ptr;
	   saveofs = gpb->gpb_cmdofs;
	   gpb->gpb_ptr = ptr.sh;
	   gpb->gpb_cmdofs = cmdofs;
	   Print_VP(gpbobj);
	   gpb->gpb_ptr = saveptr;
	   gpb->gpb_cmdofs = saveofs;
          }
#endif
	ptr.sh = bufofs;
	PUT_CMD(ptr.sh, GP1_EOCL, GP1_FREEBLKS);	/* free the buffer */
	PUT_INT(ptr.in, bv);
	POSTGPBUF(gpb, cmdofs);			/* post the buffer */
}

/****
 *
 * Trans_Matrix(vp, p, m)
 * Computes a translation matrix and stores it in matrix #M.
 * If M is negative, the matrix is not stored in the GP but it
 * is computed and returned to the user. The X, Y, Z translation
 * factors are contained in the POINT P and are assumed to be in
 * floating point viewport coordinates.
 *
 * returns:
 *   translation matrix
 *
 ****/
float *
Trans_Matrix(vp, p, m)
/*    --needs--			 */
	VP	vp;		/* INP viewport */
FAST	POINT	*p;		/* INP -> translation factors */
	int	m;		/* INP number of matrix */
{
/*    --uses--			 */
extern	MATRIX	Identity3D;
local	MATRIX	mat;

	bcopy(Identity3D, mat, sizeof(MATRIX));
	mat[3][0] = p->f.x;
	mat[3][1] = p->f.y;
	mat[3][2] = p->f.z;
	if (m > 0) gp_matrix_set(vp, m, mat);
	return (&mat[0][0]);
}

/****
 *
 * Scale_Matrix(vp, p, m)
 * Computes a scaling matrix and stores it in matrix #M.
 * If M is negative, the matrix is not stored in the GP but it
 * is computed and returned to the user. The X, Y, Z scale
 * factors are contained in the POINT P and are assumed to be in
 * floating point viewport coordinates.
 *
 * returns:
 *   scale matrix
 *
 ****/
float *
Scale_Matrix(vp, p, m)
/*    --needs--			 */
	VP	vp;		/* INP viewport */
FAST	POINT	*p;		/* INP -> scale factors */
	int	m;		/* INP number of matrix */
{
/*    --uses--			 */
extern	MATRIX	Identity3D;
local	MATRIX	mat;

	bcopy(Identity3D, mat, sizeof(MATRIX));
	mat[0][0] = p->f.x;
	mat[1][1] = p->f.y;
	mat[2][2] = p->f.z;
	if (m > 0) gp_matrix_set(vp, m, mat);
	return (&mat[0][0]);
}

/****
 *
 * RotX_Matrix(vp, a, m)
 * Computes an X axis rotation matrix and stores it in matrix #M.
 * If M is negative, the matrix is not stored in the GP but it
 * is computed and returned to the user. The angle of rotation A
 * is assumed to be floating point.
 *
 * returns:
 *   X rotation matrix
 *
 ****/
float *
RotX_Matrix(vp, a, m)
/*    --needs--			 */
	VP	vp;		/* INP viewport */
FAST	double	a;		/* INP rotation angle */
	int	m;		/* INP number of matrix */
{
/*    --uses--			 */
extern	MATRIX	Identity3D;
local	MATRIX	mat;

	bcopy(Identity3D, mat, sizeof(MATRIX));
	mat[1][1] = mat[2][2] = cos(a);
	mat[1][2] = sin(a);
	mat[2][1] = -mat[1][2];
	if (m > 0) gp_matrix_set(vp, m, mat);
	return (&mat[0][0]);
}

/****
 *
 * RotY_Matrix(vp, a, m)
 * Computes an Y axis rotation matrix and stores it in matrix #M.
 * If M is negative, the matrix is not stored in the GP but it
 * is computed and returned to the user. The angle of rotation A
 * is assumed to be floating point.
 *
 * returns:
 *   Y rotation matrix
 *
 ****/
float *
RotY_Matrix(vp, a, m)
/*    --needs--			 */
	VP	vp;		/* INP viewport */
FAST	double	a;		/* INP rotation angle */
	int	m;		/* INP number of matrix */
{
/*    --uses--			 */
extern	MATRIX	Identity3D;
local	MATRIX	mat;

	bcopy(Identity3D, mat, sizeof(MATRIX));
	mat[0][0] = mat[2][2] = cos(a);
	mat[2][0] = sin(a);
	mat[0][2] = -mat[2][0];
	if (m > 0) gp_matrix_set(vp, m, mat);
	return (&mat[0][0]);
}

/****
 *
 * RotZ_Matrix(vp, a, m)
 * Computes an Z axis rotation matrix and stores it in matrix #M.
 * If M is negative, the matrix is not stored in the GP but it
 * is computed and returned to the user. The angle of rotation A
 * is assumed to be floating point.
 *
 * returns:
 *   Z rotation matrix
 *
 ****/
float *
RotZ_Matrix(vp, a, m)
/*    --needs--			 */
	VP	vp;		/* INP viewport */
FAST	double	a;		/* INP rotation angle */
	int	m;		/* INP number of matrix */
{
/*    --uses--			 */
extern	MATRIX	Identity3D;
local	MATRIX	mat;

	bcopy(Identity3D, mat, sizeof(MATRIX));
	mat[0][0] = mat[1][1] = cos(a);
	mat[0][1] = sin(a);
	mat[1][0] = -mat[0][1];
	if (m > 0) gp_matrix_set(vp, m, mat);
	return (&mat[0][0]);
}

/****
 *
 * Identity_Matrix(vp, n, mat)
 * Load the identity matrix into matrix #N and matrix MAT.
 *
 ****/
Identity_Matrix(vp, n, mat)
/*    --needs--			 */
	VP	vp;
	int	n;
	MATRIX	mat;
{
extern	MATRIX	Identity3D;

	bcopy(Identity3D, mat, sizeof(MATRIX));
	if (n > 0) gp_matrix_set(vp, n, mat);
}
