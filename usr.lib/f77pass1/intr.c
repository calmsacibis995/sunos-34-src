#ifndef lint
static	char sccsid[] = "@(#)intr.c 1.3 87/01/08 SMI"; /* from UCB X.X XX/XX/XX */
#endif

#include "defs.h"

extern ftnint intcon[14];
extern double realcon[6];
Exprp expand_mnmx();

union
{
	int ijunk;
	struct Intrpacked bits;
} 
packed;

struct Intrbits
{
	Intr intrgroup		/* :3 */;
	int intrstuff 		/* result type or number of generics */;
	int intrno 		/* :7 */;
};

LOCAL struct Intrblock
{
	char intrfname[VL];
	struct Intrbits intrval;
} 
intrtab[ ] = {
	"int", 		{ INTRCONV, ord(TYLONG) 	} ,
	"real", 	{ INTRCONV, ord(TYREAL) 	} ,
	"dble", 	{ INTRCONV, ord(TYDREAL) 	} ,
	"dreal",	{ INTRCONV, ord(TYDREAL) 	} ,
	"cmplx", 	{ INTRCONV, ord(TYCOMPLEX) 	} ,
	"dcmplx", 	{ INTRCONV, ord(TYDCOMPLEX) 	} ,
	"ifix", 	{ INTRCONV, ord(TYLONG) 	} ,
	"idint", 	{ INTRCONV, ord(TYLONG) 	} ,
	"float", 	{ INTRCONV, ord(TYREAL) 	} ,
	"dfloat",	{ INTRCONV, ord(TYDREAL) 	} ,
	"sngl", 	{ INTRCONV, ord(TYREAL) 	} ,
	"ichar", 	{ INTRCONV, ord(TYLONG) 	} ,
	"iachar", 	{ INTRCONV, ord(TYLONG) 	} ,
	"char", 	{ INTRCONV, ord(TYCHAR) 	} ,
	"achar", 	{ INTRCONV, ord(TYCHAR) 	} ,

	"max", 		{ INTRMAX, ord(TYUNKNOWN) 	} ,
	"max0", 	{ INTRMAX, ord(TYLONG) 	} ,
	"amax0", 	{ INTRMAX, ord(TYREAL) 	} ,
	"max1", 	{ INTRMAX, ord(TYLONG) 	} ,
	"amax1", 	{ INTRMAX, ord(TYREAL) 	} ,
	"dmax1", 	{ INTRMAX, ord(TYDREAL) 	} ,

	"and",		{ INTRBOOL, ord(TYUNKNOWN), ord(OPBITAND)	} ,
	"or",		{ INTRBOOL, ord(TYUNKNOWN), ord(OPBITOR)	} ,
	"xor",		{ INTRBOOL, ord(TYUNKNOWN), ord(OPBITXOR) 	} ,
	"not",		{ INTRBOOL, ord(TYUNKNOWN), ord(OPBITNOT)	} ,
	"lshift",	{ INTRBOOL, ord(TYUNKNOWN), ord(OPLSHIFT) 	} ,
	"rshift",	{ INTRBOOL, ord(TYUNKNOWN), ord(OPRSHIFT)	} ,
		
	"min", 		{ INTRMIN, ord(TYUNKNOWN) 	} ,
	"min0", 	{ INTRMIN, ord(TYLONG) 	} ,
	"amin0", 	{ INTRMIN, ord(TYREAL) 	} ,
	"min1", 	{ INTRMIN, ord(TYLONG) 	} ,
	"amin1", 	{ INTRMIN, ord(TYREAL) 	} ,
	"dmin1", 	{ INTRMIN, ord(TYDREAL) 	} ,

	"aint", 	{ INTRGEN, 2, 0 	} ,
	"dint", 	{ INTRSPEC, ord(TYDREAL), 1 	} ,

	"anint", 	{ INTRGEN, 2, 2 	} ,
	"dnint", 	{ INTRSPEC, ord(TYDREAL), 3 	} ,
	"nint", 	{ INTRGEN, 4, 4 	} ,
	"idnint", 	{ INTRGEN, 2, 6 	} ,

	"abs", 		{ INTRGEN, 6, 8 	} ,
	"iabs", 	{ INTRGEN, 2, 9 	} ,
	"dabs", 	{ INTRSPEC, ord(TYDREAL), 11 	} ,
	"cabs", 	{ INTRSPEC, ord(TYREAL), 12 	} ,
	"zabs", 	{ INTRSPEC, ord(TYDREAL), 13 	} ,
	"cdabs",	{ INTRSPEC, ord(TYDREAL), 13 	} ,

	"mod", 		{ INTRGEN, 4, 14 	} ,
	"amod", 	{ INTRSPEC, ord(TYREAL), 16 	} ,
	"dmod", 	{ INTRSPEC, ord(TYDREAL), 17 	} ,

	"sign", 	{ INTRGEN, 4, 18 	} ,
	"isign", 	{ INTRGEN, 2, 19 	} ,
	"dsign", 	{ INTRSPEC, ord(TYDREAL), 21 	} ,

	"dim", 		{ INTRGEN, 4, 22 	} ,
	"idim", 	{ INTRGEN, 2, 23 	} ,
	"ddim", 	{ INTRSPEC, ord(TYDREAL), 25 	} ,

	"dprod", 	{ INTRSPEC, ord(TYDREAL), 26 	} ,

	"len", 		{ INTRSPEC, ord(TYLONG), 27 	} ,
	"index", 	{ INTRSPEC, ord(TYLONG), 29 	} ,

	"imag", 	{ INTRGEN, 2, 31 	} ,
	"aimag", 	{ INTRSPEC, ord(TYREAL), 31 	} ,
	"dimag", 	{ INTRSPEC, ord(TYDREAL), 32 	} ,

	"conjg", 	{ INTRGEN, 2, 33 	} ,
	"dconjg", 	{ INTRSPEC, ord(TYDCOMPLEX), 34 	} ,

	"sqrt", 	{ INTRGEN, 4, 35 	} ,
	"dsqrt", 	{ INTRSPEC, ord(TYDREAL), 36 	} ,
	"csqrt", 	{ INTRSPEC, ord(TYCOMPLEX), 37 	} ,
	"zsqrt", 	{ INTRSPEC, ord(TYDCOMPLEX), 38 	} ,
	"cdsqrt",	{ INTRSPEC, ord(TYDCOMPLEX), 38 	} ,

	"exp", 		{ INTRGEN, 4, 39 	} ,
	"dexp", 	{ INTRSPEC, ord(TYDREAL), 40 	} ,
	"cexp", 	{ INTRSPEC, ord(TYCOMPLEX), 41 	} ,
	"zexp", 	{ INTRSPEC, ord(TYDCOMPLEX), 42 	} ,
	"cdexp",	{ INTRSPEC, ord(TYDCOMPLEX), 42 	} ,

	"log", 		{ INTRGEN, 4, 43 	} ,
	"alog", 	{ INTRSPEC, ord(TYREAL), 43 	} ,
	"dlog", 	{ INTRSPEC, ord(TYDREAL), 44 	} ,
	"clog", 	{ INTRSPEC, ord(TYCOMPLEX), 45 	} ,
	"zlog", 	{ INTRSPEC, ord(TYDCOMPLEX), 46 	} ,
	"cdlog",	{ INTRSPEC, ord(TYDCOMPLEX), 46 	} ,

	"log10", 	{ INTRGEN, 2, 47 	} ,
	"alog10", 	{ INTRSPEC, ord(TYREAL), 47 	} ,
	"dlog10", 	{ INTRSPEC, ord(TYDREAL), 48 	} ,

	"sin", 		{ INTRGEN, 4, 49 	} ,
	"dsin", 	{ INTRSPEC, ord(TYDREAL), 50 	} ,
	"csin", 	{ INTRSPEC, ord(TYCOMPLEX), 51 	} ,
	"zsin", 	{ INTRSPEC, ord(TYDCOMPLEX), 52 	} ,
	"cdsin",	{ INTRSPEC, ord(TYDCOMPLEX), 52 	} ,

	"cos", 		{ INTRGEN, 4, 53 	} ,
	"dcos", 	{ INTRSPEC, ord(TYDREAL), 54 	} ,
	"ccos", 	{ INTRSPEC, ord(TYCOMPLEX), 55 	} ,
	"zcos", 	{ INTRSPEC, ord(TYDCOMPLEX), 56 	} ,
	"cdcos",	{ INTRSPEC, ord(TYDCOMPLEX), 56 	} ,

	"tan", 		{ INTRGEN, 2, 57 	} ,
	"dtan", 	{ INTRSPEC, ord(TYDREAL), 58 	} ,

	"asin", 	{ INTRGEN, 2, 59 	} ,
	"dasin", 	{ INTRSPEC, ord(TYDREAL), 60 	} ,

	"acos", 	{ INTRGEN, 2, 61 	} ,
	"dacos", 	{ INTRSPEC, ord(TYDREAL), 62 	} ,

	"atan", 	{ INTRGEN, 2, 63 	} ,
	"datan", 	{ INTRSPEC, ord(TYDREAL), 64 	} ,
	"atan2", 	{ INTRGEN, 2, 65 	} ,
	"datan2", 	{ INTRSPEC, ord(TYDREAL), 66 	} ,

	"sinh", 	{ INTRGEN, 2, 67 	} ,
	"dsinh", 	{ INTRSPEC, ord(TYDREAL), 68 	} ,

	"cosh", 	{ INTRGEN, 2, 69 	} ,
	"dcosh", 	{ INTRSPEC, ord(TYDREAL), 70 	} ,

	"tanh", 	{ INTRGEN, 2, 71 	} ,
	"dtanh", 	{ INTRSPEC, ord(TYDREAL), 72 	} ,

	"lge",		{ INTRSPEC, ord(TYLOGICAL), 73	} ,
	"lgt",		{ INTRSPEC, ord(TYLOGICAL), 75	} ,
	"lle",		{ INTRSPEC, ord(TYLOGICAL), 77	} ,
	"llt",		{ INTRSPEC, ord(TYLOGICAL), 79	} ,

	"epbase",	{ INTRCNST, 4, 0 	} ,
	"epprec",	{ INTRCNST, 4, 4 	} ,
	"epemin",	{ INTRCNST, 2, 8 	} ,
	"epemax",	{ INTRCNST, 2, 10 	} ,
	"eptiny",	{ INTRCNST, 2, 12 	} ,
	"ephuge",	{ INTRCNST, 4, 14 	} ,
	"epmrsp",	{ INTRCNST, 2, 18 	} ,

	"fpexpn",	{ INTRGEN, 4, 81 	} ,
	"fpabsp",	{ INTRGEN, 2, 85 	} ,
	"fprrsp",	{ INTRGEN, 2, 87 	} ,
	"fpfrac",	{ INTRGEN, 2, 89 	} ,
	"fpmake",	{ INTRGEN, 2, 91 	} ,
	"fpscal",	{ INTRGEN, 2, 93 	} ,
	"fptype",	{ INTRGEN, 2, 95 	} ,

	"" };


LOCAL struct Specblock
{
	Vtype atype;
	Vtype rtype;
	char nargs;
	char spxname[XL];
	char othername;	/* index into callbyvalue table NOT USED AT PRESENT */
} spectab[ ] = {
/* 0 */ 	{ TYREAL,TYREAL,1,"r_int" 	} ,
/* 1 */ 	{ TYDREAL,TYDREAL,1,"d_int" 	} ,
/* 2 */ 	{ TYREAL,TYREAL,1,"r_nint" 	} ,
/* 3 */ 	{ TYDREAL,TYDREAL,1,"d_nint" 	} ,
/* 4 */ 	{ TYREAL,TYSHORT,1,"h_nint" 	} ,
/* 5 */ 	{ TYREAL,TYLONG,1,"i_nint" 	} ,
/* 6 */ 	{ TYDREAL,TYSHORT,1,"h_dnnt" 	} ,
/* 7 */ 	{ TYDREAL,TYLONG,1,"i_dnnt" 	} ,
/* 8 */ 	{ TYREAL,TYREAL,1,"r_abs" 	} ,
/* 9 */ 	{ TYSHORT,TYSHORT,1,"h_abs" 	} ,
/* 10 */ 	{ TYLONG,TYLONG,1,"i_abs" 	} ,
/* 11 */ 	{ TYDREAL,TYDREAL,1,"d_abs" 	} ,
/* 12 */ 	{ TYCOMPLEX,TYREAL,1,"c_abs" 	} ,
/* 13 */ 	{ TYDCOMPLEX,TYDREAL,1,"z_abs" 	} ,
/* 14 */ 	{ TYSHORT,TYSHORT,2,"h_mod" 	} ,
/* 15 */ 	{ TYLONG,TYLONG,2,"i_mod" 	} ,
/* 16 */ 	{ TYREAL,TYREAL,2,"r_mod" 	} ,
/* 17 */ 	{ TYDREAL,TYDREAL,2,"d_mod" 	} ,
/* 18 */ 	{ TYREAL,TYREAL,2,"r_sign" 	} ,
/* 19 */ 	{ TYSHORT,TYSHORT,2,"h_sign" 	} ,
/* 20 */ 	{ TYLONG,TYLONG,2,"i_sign" 	} ,
/* 21 */ 	{ TYDREAL,TYDREAL,2,"d_sign" 	} ,
/* 22 */ 	{ TYREAL,TYREAL,2,"r_dim" 	} ,
/* 23 */ 	{ TYSHORT,TYSHORT,2,"h_dim" 	} ,
/* 24 */ 	{ TYLONG,TYLONG,2,"i_dim" 	} ,
/* 25 */ 	{ TYDREAL,TYDREAL,2,"d_dim" 	} ,
/* 26 */ 	{ TYREAL,TYDREAL,2,"d_prod" 	} ,
/* 27 */ 	{ TYCHAR,TYSHORT,1,"h_len" 	} ,
/* 28 */ 	{ TYCHAR,TYLONG,1,"i_len" 	} ,
/* 29 */ 	{ TYCHAR,TYSHORT,2,"h_indx" 	} ,
/* 30 */ 	{ TYCHAR,TYLONG,2,"i_indx" 	} ,
/* 31 */ 	{ TYCOMPLEX,TYREAL,1,"r_imag" 	} ,
/* 32 */ 	{ TYDCOMPLEX,TYDREAL,1,"d_imag" 	} ,
/* 33 */ 	{ TYCOMPLEX,TYCOMPLEX,1,"r_cnjg" 	} ,
/* 34 */ 	{ TYDCOMPLEX,TYDCOMPLEX,1,"d_cnjg" 	} ,
/* 35 */ 	{ TYREAL,TYREAL,1,"r_sqrt"  	} ,
/* 36 */ 	{ TYDREAL,TYDREAL,1,"d_sqrt", 1 	} ,
/* 37 */ 	{ TYCOMPLEX,TYCOMPLEX,1,"c_sqrt" 	} ,
/* 38 */ 	{ TYDCOMPLEX,TYDCOMPLEX,1,"z_sqrt" 	} ,
/* 39 */ 	{ TYREAL,TYREAL,1,"r_exp"  	} ,
/* 40 */ 	{ TYDREAL,TYDREAL,1,"d_exp", 2 	} ,
/* 41 */ 	{ TYCOMPLEX,TYCOMPLEX,1,"c_exp" 	} ,
/* 42 */ 	{ TYDCOMPLEX,TYDCOMPLEX,1,"z_exp" 	} ,
/* 43 */ 	{ TYREAL,TYREAL,1,"r_log"  	} ,
/* 44 */ 	{ TYDREAL,TYDREAL,1,"d_log", 3 	} ,
/* 45 */ 	{ TYCOMPLEX,TYCOMPLEX,1,"c_log" 	} ,
/* 46 */ 	{ TYDCOMPLEX,TYDCOMPLEX,1,"z_log" 	} ,
/* 47 */ 	{ TYREAL,TYREAL,1,"r_lg10" 	} ,
/* 48 */ 	{ TYDREAL,TYDREAL,1,"d_lg10" 	} ,
/* 49 */ 	{ TYREAL,TYREAL,1,"r_sin"	  	} ,
/* 50 */ 	{ TYDREAL,TYDREAL,1,"d_sin", 4 	} ,
/* 51 */ 	{ TYCOMPLEX,TYCOMPLEX,1,"c_sin" 	} ,
/* 52 */ 	{ TYDCOMPLEX,TYDCOMPLEX,1,"z_sin" 	} ,
/* 53 */ 	{ TYREAL,TYREAL,1,"r_cos"	  	} ,
/* 54 */ 	{ TYDREAL,TYDREAL,1,"d_cos", 5 	} ,
/* 55 */ 	{ TYCOMPLEX,TYCOMPLEX,1,"c_cos" 	} ,
/* 56 */ 	{ TYDCOMPLEX,TYDCOMPLEX,1,"z_cos" 	} ,
/* 57 */ 	{ TYREAL,TYREAL,1,"r_tan"	  	} ,
/* 58 */ 	{ TYDREAL,TYDREAL,1,"d_tan", 6 	} ,
/* 59 */ 	{ TYREAL,TYREAL,1,"r_asin"  	} ,
/* 60 */ 	{ TYDREAL,TYDREAL,1,"d_asin", 7 	} ,
/* 61 */ 	{ TYREAL,TYREAL,1,"r_acos"  	} ,
/* 62 */ 	{ TYDREAL,TYDREAL,1,"d_acos", 8 	} ,
/* 63 */ 	{ TYREAL,TYREAL,1,"r_atan"	 	} ,
/* 64 */ 	{ TYDREAL,TYDREAL,1,"d_atan", 9 	} ,
/* 65 */ 	{ TYREAL,TYREAL,2,"r_atn2" 		} ,
/* 66 */ 	{ TYDREAL,TYDREAL,2,"d_atn2", 10 	} ,
/* 67 */ 	{ TYREAL,TYREAL,1,"r_sinh"	 	} ,
/* 68 */ 	{ TYDREAL,TYDREAL,1,"d_sinh", 11 	} ,
/* 69 */ 	{ TYREAL,TYREAL,1,"r_cosh"		} ,
/* 70 */ 	{ TYDREAL,TYDREAL,1,"d_cosh", 12 	} ,
/* 71 */ 	{ TYREAL,TYREAL,1,"r_tanh"	 	} ,
/* 72 */ 	{ TYDREAL,TYDREAL,1,"d_tanh", 13 	} ,
/* 73 */ 	{ TYCHAR,TYLOGICAL,2,"hl_ge" 	} ,
/* 74 */ 	{ TYCHAR,TYLOGICAL,2,"l_ge" 	} ,
/* 75 */ 	{ TYCHAR,TYLOGICAL,2,"hl_gt" 	} ,
/* 76 */ 	{ TYCHAR,TYLOGICAL,2,"l_gt" 	} ,
/* 77 */ 	{ TYCHAR,TYLOGICAL,2,"hl_le" 	} ,
/* 78 */ 	{ TYCHAR,TYLOGICAL,2,"l_le" 	} ,
/* 79 */ 	{ TYCHAR,TYLOGICAL,2,"hl_lt" 	} ,
/* 80 */ 	{ TYCHAR,TYLOGICAL,2,"l_lt" 	} ,
/* 81 */ 	{ TYREAL,TYSHORT,1,"hr_expn" 	} ,
/* 82 */ 	{ TYREAL,TYLONG,1,"ir_expn" 	} ,
/* 83 */ 	{ TYDREAL,TYSHORT,1,"hd_expn" 	} ,
/* 84 */ 	{ TYDREAL,TYLONG,1,"id_expn" 	} ,
/* 85 */ 	{ TYREAL,TYREAL,1,"r_absp" 	} ,
/* 86 */ 	{ TYDREAL,TYDREAL,1,"d_absp" 	} ,
/* 87 */ 	{ TYREAL,TYDREAL,1,"r_rrsp" 	} ,
/* 88 */ 	{ TYDREAL,TYDREAL,1,"d_rrsp" 	} ,
/* 89 */ 	{ TYREAL,TYREAL,1,"r_frac" 	} ,
/* 90 */ 	{ TYDREAL,TYDREAL,1,"d_frac" 	} ,
/* 91 */ 	{ TYREAL,TYREAL,2,"r_make" 	} ,
/* 92 */ 	{ TYDREAL,TYDREAL,2,"d_make" 	} ,
/* 93 */ 	{ TYREAL,TYREAL,2,"r_scal" 	} ,
/* 94 */ 	{ TYDREAL,TYDREAL,2,"d_scal" 	},
/* 95 */ 	{ TYREAL,TYLONG,1,"r_type" 	} ,
/* 96 */ 	{ TYDREAL,TYLONG,1,"d_type" 	}
} ;

LOCAL struct Incstblock
{
	Vtype atype;
	Vtype rtype;
	char constno;
} consttab[ ] = {
	{ TYSHORT, TYLONG, 0 	} ,
	{ TYLONG, TYLONG, 1 	} ,
	{ TYREAL, TYLONG, 2 	} ,
	{ TYDREAL, TYLONG, 3 	} ,

	{ TYSHORT, TYLONG, 4 	} ,
	{ TYLONG, TYLONG, 5 	} ,
	{ TYREAL, TYLONG, 6 	} ,
	{ TYDREAL, TYLONG, 7 	} ,

	{ TYREAL, TYLONG, 8 	} ,
	{ TYDREAL, TYLONG, 9 	} ,
	{ TYREAL, TYLONG, 10 	} ,
	{ TYDREAL, TYLONG, 11 	} ,

	{ TYREAL, TYREAL, 0 	} ,
	{ TYDREAL, TYDREAL, 1 	} ,

	{ TYSHORT, TYLONG, 12 	} ,
	{ TYLONG, TYLONG, 13 	} ,
	{ TYREAL, TYREAL, 2 	} ,
	{ TYDREAL, TYDREAL, 3 	} ,

	{ TYREAL, TYREAL, 4 	} ,
	{ TYDREAL, TYDREAL, 5 	}
};

/* For each machine, two arrays must be initialized.
intcon contains
	radix for short int
	radix for long int
	radix for single precision
	radix for double precision
	precision for short int
	precision for long int
	precision for single precision
	precision for double precision
	emin for single precision
	emin for double precision
	emax for single precision
	emax for double prcision
	largest short int
	largest long int

realcon contains
	tiny for single precision
	tiny for double precision
	huge for single precision
	huge for double precision
	mrsp (epsilon) for single precision
	mrsp (epsilon) for double precision

the realcons should probably be filled in in binary if TARGET==HERE
*/

char callbyvalue[ ][XL] =
{
	"sqrt",
	"exp",
	"log",
	"sin",
	"cos",
	"tan",
	"asin",
	"acos",
	"atan",
	"atan2",
	"sinh",
	"cosh",
	"tanh"
};

expptr intrcall(np, argsp, nargs)
Namep np;
struct Listblock *argsp;
int nargs;
{
	int i; 
	Vtype rettype;
	Addrp ap;
	register struct Specblock *sp;
	register struct Chain *cp;
	expptr inline(), mkcxcon(), mkrealcon();
	register struct Incstblock *cstp;
	expptr q, ep;
	Vtype mtype;
	Opcode op;
	Intr	f1field;
	Vtype	f2field;
	int 	f3field;

	packed.ijunk = np->vardesc.varno;
	f1field = (Intr) packed.bits.f1;
	f2field = (Vtype) packed.bits.f2;
	f3field = packed.bits.f3;
	if(nargs == 0)
		goto badnargs;

	mtype = TYUNKNOWN;
	for(cp = argsp->listp ; cp ; cp = cp->nextp)
	{
		/* TEMPORARY */ ep = (expptr) (cp->datap);
		/* TEMPORARY */	if( ISCONST(ep) && ep->headblock.vtype==TYSHORT )
			/* TEMPORARY */		cp->datap = (tagptr) mkconv(tyint, ep);
		mtype = maxtype(mtype, ep->headblock.vtype);
	}

	switch(f1field)
	{
	case INTRBOOL:
		op = (Opcode) f3field;
		if( ! ONEOF(mtype, MSKINT|MSKLOGICAL) )
			goto badtype;
		if(op == OPBITNOT)
		{
			if(nargs != 1)
				goto badnargs;
			q = mkexpr(OPBITNOT, argsp->listp->datap, ENULL);
		}
		else
		{
			if(nargs != 2)
				goto badnargs;
			q = mkexpr(op, argsp->listp->datap,
			argsp->listp->nextp->datap);
		}
		frchain( &(argsp->listp) );
		ckfree( (charptr) argsp);
		return(q);

	case INTRCONV:
		if (nargs == 1)
		{
			if(argsp->listp->datap->headblock.vtype == TYERROR)
			{
				ckfree( (charptr) argsp->listp->datap);
				frchain( &(argsp->listp) );
				ckfree( (charptr) argsp);
				return( errnode() );
			}
		}
		else if (nargs == 2)
		{
			if(argsp->listp->nextp->datap->headblock.vtype == 
			    TYERROR ||
			    argsp->listp->datap->headblock.vtype == TYERROR)
			{
				ckfree( (charptr) argsp->listp->nextp->datap);
				ckfree( (charptr) argsp->listp->datap);
				frchain( &(argsp->listp) );
				ckfree( (charptr) argsp);
				return( errnode() );
			}
		}
		rettype = f2field;
		if(rettype == TYLONG)
			rettype = tyint;
		if( ISCOMPLEX(rettype) && nargs==2) {
			expptr qr, qi;
			qr = (expptr) (argsp->listp->datap);
			qi = (expptr) (argsp->listp->nextp->datap);
			if(ISCONST(qr) && ISCONST(qi)) {
				q = mkcxcon(qr,qi);
			} else	{
				/*
				**	composition of complexes from 2 elements is not
				**	done in line because of hassles addressing the elements
				*/
				if(rettype == TYCOMPLEX) {
					q = call2(	TYCOMPLEX, "c_cmplx",
								mkconv(TYREAL,qr), mkconv(TYREAL,qi));
				} else {
					q = call2(	TYDCOMPLEX, "d_cmplx",
								mkconv(TYDREAL,qr), mkconv(TYDREAL,qi));
				}
			}
		} else if(nargs == 1) {
			if(rettype == TYCHAR) {
				q = call1(TYCHAR,"i_conv_c",mkconv(TYLONG,argsp->listp->datap));
				q->exprblock.vleng = ICON(1);
			} else if(mtype == TYCHAR) {
				q = call1(TYLONG,"c_conv_i",argsp->listp->datap);
				q = mkconv(tyint, q);
			} else {
				q = mkconv(rettype, argsp->listp->datap);
			}
		} else goto badnargs;

		q->headblock.vtype = rettype;
		frchain(&(argsp->listp));
		ckfree( (charptr) argsp);
		return(q);


	case INTRCNST:
		cstp = consttab + f3field;
		for(i=0 ; i<ord(f2field) ; ++i)
			if(cstp->atype == mtype)
				goto foundconst;
			else
				++cstp;
		goto badtype;

foundconst:
		switch(cstp->rtype)
		{
		case TYLONG:
			return(mkintcon(intcon[cstp->constno]));

		case TYREAL:
		case TYDREAL:
			return(mkrealcon(cstp->rtype,
			realcon[cstp->constno]) );

		default:
			fatal("impossible intrinsic constant");
		}

	case INTRGEN:
		sp = spectab + f3field;
		if(no66flag)
			if(sp->atype == mtype)
				goto specfunct;
			else err66("generic function");

		for(i=0; i < ord(f2field) ; ++i) {
			if(sp->atype == mtype)
				goto specfunct;
			else
				++sp;
		}
		goto badtype;

	case INTRSPEC:
		sp = spectab + f3field;
specfunct:
		if(tyint==TYLONG && ONEOF(sp->rtype,M(TYSHORT)|M(TYLOGICAL))
		    && (sp+1)->atype==sp->atype)
			++sp;

		if(nargs != sp->nargs)
			goto badnargs;
		if(mtype != sp->atype)
			goto badtype;
		fixargs(YES, argsp);
		if(q = inline(sp-spectab, mtype, argsp->listp)) {
			frchain( &(argsp->listp) );
			ckfree( (charptr) argsp);
		} else {
			ap = builtin(sp->rtype, varstr(XL, sp->spxname) );
			q = fixexpr( mkexpr(OPCALL, ap, argsp) );
			if (sp->rtype == TYREAL)
			{
				q = mkconv(TYREAL, q);
			}
		}
		return(q);

	case INTRMIN:
	case INTRMAX:
		if(nargs < 2)
			goto badnargs;
		if( ! ONEOF(mtype, MSKINT|MSKREAL) )
			goto badtype;
		q = (expptr) expand_mnmx(mtype, (f1field==INTRMIN ? OPLT : OPGT), argsp->listp);

		q->headblock.vtype = mtype;
		rettype = f2field;
		if(rettype == TYLONG)
			rettype = tyint;
		else if(rettype == TYUNKNOWN)
			rettype = mtype;
		return( mkconv(rettype, q) );

	default:
		fatali("intrcall: bad intrgroup %d", f1field);
	}
badnargs:
	errstr("bad number of arguments to intrinsic %s",
	varstr(VL,np->varname) );
	goto bad;

badtype:
	errstr("bad argument type to intrinsic %s", varstr(VL, np->varname) );

bad:
	return( errnode() );
}




intrfunct(s)
char s[VL];
{
	register struct Intrblock *p;
	char nm[VL];
	register int i;

	for(i = 0 ; i<VL ; ++s)
		nm[i++] = (*s==' ' ? '\0' :
			   (isupper(*s) ? tolower(*s) : *s) );

	for(p = intrtab; p->intrval.intrgroup!=INTREND ; ++p)
	{
		if( !bcmp(nm, p->intrfname, VL) )
		{
			packed.bits.f1 = ord(p->intrval.intrgroup);
			packed.bits.f2 = p->intrval.intrstuff;
			packed.bits.f3 = p->intrval.intrno;
			return(packed.ijunk);
		}
	}

	return(0);
}





Addrp intraddr(np)
Namep np;
{
	Addrp q;
	register struct Specblock *sp;
	int f3field;

	if(np->vclass!=CLPROC || np->vprocclass!=PINTRINSIC)
		fatalstr("intraddr: %s is not intrinsic", varstr(VL,np->varname));
	packed.ijunk = np->vardesc.varno;
	f3field = packed.bits.f3;

	switch(packed.bits.f1)
	{
	case INTRGEN:
		/* imag, log, and log10 arent specific functions 
		** all other gen names double as specifics
		*/
		if(f3field==31 || f3field==43 || f3field==47)
			goto bad;

	case INTRSPEC:
		sp = spectab + f3field;
		if(tyint==TYLONG && sp->rtype==TYSHORT)
			++sp;
		q = builtin(sp->rtype, varstr(XL,sp->spxname) );
		return(q);

	case INTRCONV:
	case INTRMIN:
	case INTRMAX:
	case INTRBOOL:
	case INTRCNST:
bad:
		errstr("cannot pass %s as actual",
		varstr(VL,np->varname));
		return( (Addrp) errnode() );
	}
	fatali("intraddr: impossible f1=%d\n", (int) packed.bits.f1);
	/* NOTREACHED */
}





LOCAL
expptr 
inline(fno, type, args)
int fno;
Vtype type;
struct Chain *args;
{
	register expptr q, t, t1;

	switch(fno)
	{
	case 9:	/* short int abs */
	case 10:	/* long int abs */
		if( !ISINT(type) ) {
			return(NULL);
		}
		if( addressable(q = (expptr) (args->datap)) )
		{
			t = q;
			q = NULL;
		}
		else
			t = (expptr) mktemp(type,PNULL);
		t1 = mkexpr(OPQUEST,
		mkexpr(OPLE, mkconv(type,ICON(0)), cpexpr(t)),
		mkexpr(OPCOLON, cpexpr(t),
		mkexpr(OPNEG, cpexpr(t), ENULL) ));
		if(q)
			t1 = mkexpr(OPCOMMA, mkexpr(OPASSIGN, cpexpr(t),q), t1);
		frexpr(t);
		return(t1);

	case 26:	/* dprod */
		q = mkexpr(OPSTAR, mkconv(TYDREAL,args->datap), args->nextp->datap);
		return(q);

	case 27:	/* len of character string */
		q = (expptr) cpexpr(args->datap->headblock.vleng);
		frexpr(args->datap);
		return(q);

	case 14:	/* half-integer mod */
	case 15:	/* mod */
		return( mkexpr(OPMOD, (expptr) (args->datap),
		(expptr) (args->nextp->datap) ));
	}
	return(NULL);
}

#define APPEND(z)	\
res = res->exprblock.rightp = mkexpr (OPCOMMA, z, newtemp)

LOCAL Exprp 
expand_mnmx(type,op,lp)
Vtype type;
Opcode op;
chainp lp;
{
	expptr qp;
	chainp p1;
	Addrp sp, tp;
	Addrp newtemp;
	Exprp result;
	expptr res;

	sp = mktemp(type, PNULL);
	tp = mktemp(type, PNULL);
	qp = mkexpr(OPCOLON, cpexpr(tp), cpexpr(sp));
	qp = mkexpr(OPQUEST, mkexpr(op, cpexpr(tp),cpexpr(sp)), qp);
	qp = fixexpr(qp);

	newtemp = mktemp (type,PNULL);

	(expptr) result = res = mkexpr (OPCOMMA,
	fixexpr(mkexpr( OPASSIGN, cpexpr(sp), lp->datap )), cpexpr(newtemp));

	for(p1 = lp->nextp ; p1 ; p1 = p1->nextp)
	{
		APPEND (fixexpr(mkexpr( OPASSIGN, cpexpr(tp), p1->datap )));
		if(p1->nextp)
			APPEND (fixexpr(mkexpr (OPASSIGN, cpexpr(sp), cpexpr(qp))) );
		else
			APPEND (fixexpr(mkexpr (OPASSIGN, cpexpr(newtemp), qp)));
	}

	frtemp(sp);
	frtemp(tp);
	frtemp(newtemp);
	frchain( &lp );

	return (result);
}
