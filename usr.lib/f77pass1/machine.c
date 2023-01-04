#ifndef lint
static	char sccsid[] = "@(#)machine.c 1.4 87/03/31 SMI"; /* from UCB X.X XX/XX/XX */
#endif

#include "defs.h"

#include <a.out.h>
#ifndef N_SO
#	include <stab.h>
#endif

#include "pccdefs.h"

ftnint intcon[14] =
{ 
	2, 2, 2, 2,
	15, 31, 24, 56,
	-128, -128, 127, 127,
	32767, 2147483647 };

long realcon[6][2] =
{
	{ 
		0001, 0 	}
	,		/* least single precision number */
	{ 
		0000, 0001 	}
	,		/* least double precision number */
	{ 
		0x3f800000, 0 	}
	,	/* single precision infinity     */
	{ 
		0x3ff00000, 0 	}
	,	/* double precision infinity     */
	{ 
		0x34000000, 0 	}
	,	/* lease e s.t. 1.0+e != 1.0	 */
	{ 
		0x3cb00000, 0 	}	/* lease e s.t. 1.0d0+e != 1.0d0 */
};
/*
**
**		double realcon[6] =
**		{
**			2.9387358771e-39,
**			2.938735877055718800e-39,
**			1.7014117332e+38,
**			1.701411834604692250e+38,
**			5.960464e-8,
**			1.38777878078144567e-17,
**		};
*/

/*
 * This union gives various access to eight bytes.
 * The layout is machine dependent and this allows 
 * constants to be extracted to be printed in hex.
 */
union 	byte8	{
	int	i[2];		/* 4 bytes at a time */
	short	s[4];		/* 2 bytes at a time */
	char	c[8];		/* 1 byte  at a time */
};

static int paramcopy;

static int lb_count = 0; 	/* used if stmtprofflag set */
extern char dotd_file[];


/*ARGSUSED*/
goret(type)
int	type;
{
	/* 
	** a jump to label -1 is an encoding for exit procedure
	*/
	put_goto(-1);
}




/*
 * move argument slot arg1 (relative to ap)
 * to slot arg2 (relative to ARGREG)
 * ARGSUSED
 */

mvarg(type, arg1, arg2)
int type, arg1, arg2;
{
int size;

	size=tysize(type);
	map_asm_ij("	movl	a6@(%ld),a6@(%ld)", arg1+8,arg2+argloc);
}




void print_d(value, fp) register int value; register FILEP fp;
{	char buffer[8]; register char *p= buffer+8; register int i= 0;

	if (value == 0) {
		putc('0', fp); return;};
	if (value < 0) {
		putc('-', fp); value= -value;};
	if (value < 32000) { register short v= value;
		while (v != 0) {
			*--p= (v % 10) + '0';
			v /= 10; i++; };}
	else {
		while (value != 0) {
			*--p= (value % 10) + '0';
			value /= 10; i++; };};
	for (; i>0; i--) putc(*p++, fp);
}

void print_x(value, fp ) register long value; register FILEP fp;
{	char buffer[8]; register char *p= buffer+8; register int i= 0;

	if (value == 0) {
		putc('0', fp);
		return;
	}
	if( value == 0x80000000 ) { /* special case */
		fwrite( "0x80000000", 1, 10, fp );
		return;
	}
	if (value < 0) {
		putc('-', fp);
		value= -value;
	}
	while (value != 0) {
		*--p= "0123456789abcdef"[value & 0xf];
		value >>= 4;
		i++;
	}
	putc('0', fp);
	putc('x', fp);
	fwrite(p, 1, i, fp);
}

prlabel(fp, k)
register FILEP fp;
int k;
{
	putc('v', fp); putc('.', fp);
	print_d(k, fp);
	putc(':', fp); putc('\n', fp);
}



prconi(fp, type, n)
register FILEP fp;
Vtype type;
register ftnint n;
{	char buffer[8]; register char *p= buffer+8; register int i= 0;

	fputs(type==TYSHORT ? "\t.word\t" : "\t.long\t", fp);
	if (n == 0) {
		putc('0', fp); putc('\n', fp); return;};
	if (n < 0) {
		putc('-', fp); n= -n;};
	while (n != 0) {
		*--p= "0123456789abcdef"[n & 0xf];
		n >>= 4; i++; }
	putc('0', fp); putc('x', fp);
	fwrite(p, 1, i, fp);
	putc('\n', fp);
}



prcona(fp, n)
register FILEP fp;
register ftnint n;
{
	fputs("\t.long\tv.", fp);
	print_d(n, fp);
	putc('\n', fp);
}


praddr(fp, stg, varno, offset)
FILE *fp;
Vstg stg; 
int varno;
ftnint offset;
{
	char *memname();

	if(stg == STGNULL)
		fprintf(fp, "\t.long\t0\n");
	else {
		fprintf(fp, "\t.long\t%s", memname(stg,varno));
		if(offset)
			fprintf(fp, "+%ld", offset);
		fprintf(fp, "\n");
	}
}




preven(k)
int k;
{
	if( k<2) return;
	fprintf(asmfile, "\t.align 4\n" );
}



char *
memname(stg, mem)
int stg, mem;
{
	static char s[20];

	switch(stg) {
		case STGCOMMON:
			sprintf(s, "_%s", varstr(XL, extsymtab[mem].extname) );
			break;

		case STGBSS:
			if(mem) {
				sprintf(s, "ARR_SEG%d", procno);
			} else {
				sprintf(s, "VAR_SEG%d", procno);
			}
			break;

		case STGINIT:
			sprintf(s, "v.%d", mem);
			break;

		default:
			badstg("memname", stg);

	}
	return(s);
}




prlocvar(s, len)
char *s;
ftnint len;
{
	int olduse;
	/* fprintf(asmfile, "\t.lcomm\t%s,%ld\n", s, len); */
	olduse = pruse(asmfile, USEBSS);
	fprintf(asmfile, "%s:	.skip %d\n", s, len);
	pruse(asmfile, olduse);
}


prext(ep)
register struct Extsym *ep;
{
	static char *seekerror = "seek error on tmp file";
	static char *readerror = "read error on tmp file";

	register int	leng;
	register int	i;
	register int	n;
	register int	repl;
	char	*tag;
	long	pos;
	union	byte8	oldvalue;
	union	byte8	newvalue;

	tag = varstr(XL, ep->extname);
	leng = ep->maxleng;

	if (leng == 0)
	{
		return;
	}

	if (ep->init == NO)
	{
		fputs("\t.comm\t_", asmfile); fputs(tag, asmfile); putc(',', asmfile);
		print_d(leng, asmfile); putc('\n', asmfile);
		return;
	}

	fputs("\t.globl\t_", asmfile); fputs(tag, asmfile); putc('\n', asmfile);
	pralign(2);
	putc('_', initfile); fputs(tag, initfile); putc(':', initfile); putc('\n', initfile);

	pos = lseek(cdatafile, ep->initoffset, 0);
	if (pos == -1)
	{
		err(seekerror);
		done(1);
	}

	oldvalue.i[0] = 0;
	oldvalue.i[1] = 0;
	n = read(cdatafile, (char *) &oldvalue, sizeof(oldvalue));
	if (n < 0)
	{
		err(readerror);
		done(1);
	}

	if (leng <= 8)
	{
		i = leng;
		while (i > 0 && oldvalue.c[--i] == '\0') /* SKIP */;
		if (oldvalue.c[i] == '\0')
			prspace(leng);
		else if (leng == 8)
			prquad(&oldvalue);
		else
			prsdata(&oldvalue, leng);

		return;
	}

	repl = 1;
	leng -= 8;

	while (leng >= 8)
	{
		newvalue.i[0] = 0;
		newvalue.i[1] = 0;

		n = read(cdatafile, (char *) &newvalue, sizeof(newvalue));
		if (n < 0)
		{
			err(readerror);
			done(1);
		}

		leng -= 8;

		if (oldvalue.i[0] == newvalue.i[0] &&
		    oldvalue.i[1] == newvalue.i[1])
			repl++;
		else
		{
			if (oldvalue.i[0] == 0 && oldvalue.i[1] == 0)
				prspace(8*repl);
			else if (repl == 1)
				prquad(&oldvalue);
			else
#ifdef NOTDEF
				prfill(repl, &oldvalue);
#else
			{
				while (repl-- > 0)
					prquad(&oldvalue);
			}
#endif
			oldvalue.i[0] = newvalue.i[0];
			oldvalue.i[1] = newvalue.i[1];
			repl = 1;
		}
	}

	newvalue.i[0] = 0;
	newvalue.i[1] = 0;

	if (leng > 0)
	{
		n = read(cdatafile, (char *) &newvalue, leng);
		if (n < 0)
		{
			err(readerror);
			done(1);
		}
	}

	if (oldvalue.i[0] == 0 && oldvalue.i[1] == 0 &&
	    newvalue.i[0] == 0 && newvalue.i[1] == 0)
	{
		prspace(8*repl + leng);
		return;
	}

	if (oldvalue.i[0] == 0 && oldvalue.i[1] == 0)	
		prspace(8*repl);
	else if (repl == 1)
		prquad(&oldvalue);
	else
#ifdef NOTDEF
		prfill(repl, &oldvalue);
#else
	{
		while (repl-- > 0)
			prquad(&oldvalue);
	}
#endif

	prsdata(&newvalue, leng);

	return;
}


prolog(ep, argvecsize,argvec,entrynum)
struct Entrypoint *ep;
int argvecsize;
Addrp  argvec;
int entrynum;
{
	int i, argslot;
	int size;
	register chainp p;
	register Namep q;
	register struct Dimblock *dp;
	expptr tp;
	int label;

	if(procclass == CLBLOCK) {
		if(ep->entryname) {
			/* put out an entry point for named block datas to catch duplicates*/
			new_current_block(varstr(XL, ep->entryname->extname),YES);
			pr_link(entrynum);
			put_goto(ep->entrylabel);
		} else {
			new_current_block(CNULL,NO);
		}
		return;
	} else if(procclass == CLMAIN) {
		new_current_block(varstr(XL, "MAIN_"),YES);
		pr_link(entrynum);
	} else if(ep->entryname) {
		new_current_block(varstr(XL, ep->entryname->extname),YES);
		pr_link(entrynum);
	}

	if(argvec) {
		argloc = argvec->segdispl;
		if(proctype == TYCHAR) {
			mvarg(TYADDR, 0, chslot);
			mvarg(TYLENG, SZADDR, chlgslot);
			argslot = SZADDR + SZLENG;
		} else if( ISCOMPLEX(proctype) ) {
			mvarg(TYADDR, 0, cxslot);
			argslot = SZADDR;
		} else {
			argslot = 0;
		}

		for(p = ep->arglist ; p ; p =p->nextp) {
			q = (Namep) (p->datap);
			mvarg(TYADDR, argslot, q->vardesc.varno);
			argslot += SZADDR;
		}
		for(p = ep->arglist ; p ; p = p->nextp) {
			q = (Namep) (p->datap);
			if(q->vtype==TYCHAR && q->vclass!=CLPROC) {
				if(q->vleng && ! ISCONST(q->vleng) )
					mvarg(TYLENG, argslot,
					q->vleng->addrblock.memno);
				argslot += SZLENG;
			}
		}
		if (lastargslot >paramcopy) paramcopy = lastargslot;
	}

for(p = ep->arglist ; p ; p = p->nextp) {
	q = (Namep) (p->datap);
	if(dp = q->vdim) {
			for(i = 0 ; i < dp->ndim ; ++i) {
				if(dp->dims[i].dimexpr) 
					put_eq( fixtype(cpexpr(dp->dims[i].dimsize)), 
						   fixtype(cpexpr(dp->dims[i].dimexpr)));
			}
            if(dbxflag) {
   			for(i = 0 ; i < dp->ndim ; ++i) {
   				if(dp->dims[i].lbaddr)
   					put_eq( fixtype(cpexpr(dp->dims[i].lbaddr)),
   					fixtype(cpexpr(dp->dims[i].lb)));
   				if(dp->dims[i].ubaddr)
   					put_eq( fixtype(cpexpr(dp->dims[i].ubaddr)),
   					fixtype(cpexpr(dp->dims[i].ub)));
               } 
            }
			size = tysize(q->vtype);
			if(q->vtype == TYCHAR)
				if( ISICON(q->vleng) )
					size *= q->vleng->constblock.const.ci;
				else
					size = -1;

			if(dp->basexpr)
				put_eq((expptr) cpexpr(fixtype(dp->baseoffset)),
				      (expptr) cpexpr(fixtype(dp->basexpr)));

#ifdef NOTDEF
			/*
			 * on VAX, get more efficient subscripting if subscripts
		     * have zero-base, so fudge the argument 
			 * pointers for arrays.  Not done if array 
			 * bounds are being checked.
			 */
			if(! checksubs && !dbxflag) {
				if(dp->basexpr) {
					if(size > 0)
						tp = (expptr) ICON(size);
					else
						tp = (expptr) cpexpr(q->vleng);
					putforce(TYLONG,
					fixtype( mkexpr(OPSTAR, tp,
					cpexpr(dp->baseoffset)) ));
					map_asm_i("	subl	d0,a5@(%d)",
					p->datap->nameblock.vardesc.varno +
					    ARGOFFSET);
				} 
				else if(dp->baseoffset->constblock.const.ci != 0) {
					char buff[25];
					if(size > 0) {
						sprintf(buff, "	subl	#%ld,a5@(%d)",
						dp->baseoffset->constblock.const.ci * size,
						p->datap->nameblock.vardesc.varno +
						    ARGOFFSET);
					} 
					else	{
						putforce(TYLONG, mkexpr(OPSTAR, cpexpr(dp->baseoffset),
						cpexpr(q->vleng) ));
						sprintf(buff, "	subl	d0,a5@(%d)",
						p->datap->nameblock.vardesc.varno +
						    ARGOFFSET);
					}
					map_asm(buff);
				}
			}
# endif
		}
	}

	if(typeaddr)
		put_eq( cpexpr(typeaddr), mkaddcon(ep->typelabel) );
	/* might give a long jump problem ?
	*/
	put_goto(ep->entrylabel);
}

pr_link(entrynum)
int entrynum;
{
	/* formerly, we put out link code here - now this is done by pcc */
	if(profileflag) {
		fprintf(asmfile, "LPG%d_%d:	.long	0\n", procno, entrynum);
		map_asm_ij("	lea	LPG%d_%d,a0", procno, entrynum);
		map_asm("	jsr	mcount");
	}
    if(stmtprofflag) {
		map_asm("	tstl	___bb_init");
		map_asm_i("	jne	LLL%d", lb_count++ );
		map_asm("	addql	#1, ___bb_init");
		map_asm("	.data1");
		map_asm_i("LLL%d:", lb_count);
		map_asm_s("	.ascii	\"%s\\0\"", dotd_file );
		map_asm("	.text");
		map_asm("	movl	___count,sp@-");
		map_asm_i("	pea	LLL%d", lb_count);
		map_asm("	jbsr	___bb_init_func");
		map_asm("	addqw	#8,sp");
		map_asm("	movl	d0,___bb");
		map_asm_i("LLL%d:", lb_count-1 );
		map_asm("	tstl	___tcov_init");
		map_asm_i("	jne	LLL%d", ++lb_count );
		map_asm("	jbsr	___tcov_init_func");
		map_asm_i("LLL%d:", lb_count++ );
	}
}

/*
 * Equate to symbols
 */
prset(to, from)
char	*to;
char	*from;
{
	fprintf(initfile, "\t%s = %s\n", to, from);
}

/*
 * Allocate some empty space.
 */
prspace(nbytes)
int	nbytes;
{
	fputs("\t.skip\t", initfile);
	print_d(nbytes, initfile);
	putc('\n', initfile);
}

/*
 * These routines brought in from vax.c
 * ARGSUSED
 */
pralign(k)
int k;
{
	/*
	 * On the Sun we have only one type of alignment
	 */
	fputs("\t.align 4\n", initfile);
}


/*
 * Take a constant of some type a put it in a static area
 * that can be returned as a char pointer.
 * Assign it thru the proper types so that type conversion takes place.
 * In particular, a real cannot be assumed to be the first word of a double.
 */
char *
packbytes(cp)
register Constp cp;
{
	static	short	shrt;
	static 	int	lng;
	static	float	flt;
	static	double	dbl;
	static	float	cmplx[2];
	static	double	dcmplx[2];

	if( (ISREAL(cp->vtype) || ISCOMPLEX(cp->vtype) ) && ! cp->isbinary) {
		fatal("packbytes: string float in intialization");
	}

	switch (cp->vtype) {
	case TYSHORT:
		shrt = cp->const.ci;
		return ((char *) &shrt);

	case TYLONG:
	case TYLOGICAL:
		lng = cp->const.ci;
		return ((char *) &lng);

	case TYREAL:
		flt = *((float *)cp->const.cbytep[0]);
		return((char *) &flt);

	case TYDREAL:
		dbl = *((double *)cp->const.cbytep[0]);
		return ((char *) &dbl);

	case TYCOMPLEX:
		cmplx[0] = *((float *)cp->const.cbytep[0]);
		cmplx[1] = *((float *)cp->const.cbytep[1]);
		return((char *) cmplx);

	case TYDCOMPLEX:
		dcmplx[0] = *((double *)cp->const.cbytep[0]);
		dcmplx[1] = *((double *)cp->const.cbytep[1]);
		return((char *) dcmplx);

	default:
		badtype("packbytes", cp->vtype);
	}
	/* NOTREACHED */
}



prlocdata(sname, leng, type, initoffset, inlcomm)
char *sname;
ftnint leng;
int type;
long initoffset;
char *inlcomm;
{
	static char *seekerror = "seek error on tmp file";
	static char *readerror = "read error on tmp file";

	static char *labelfmt = "%s:\n";

	register int k;
	register int i;
	register int repl;
	register int first;
	register long pos;
	register long n;
	union	byte8	oldvalue;
	union	byte8	newvalue;

	*inlcomm = NO;
	k = leng;
	first = YES;

	pos = lseek(vdatafile, initoffset, 0);
	if (pos == -1)
	{
		err(seekerror);
		done(1);
	}

	oldvalue.i[0] = 0;
	oldvalue.i[1] = 0;
	n = read(vdatafile, (char *) &oldvalue, sizeof(oldvalue));
	if (n < 0)
	{
		err(readerror);
		done(1);
	}

	if (k <= 8)
	{
		i = k;
		while (i > 0 && oldvalue.c[--i] == '\0')
			/*  SKIP  */ ;
		if (oldvalue.c[i] == '\0')
		{
			if (SMALLVAR(leng))
			{
				pralign(tyalign(type));
				fprintf(initfile, labelfmt, sname);
				prspace(leng);
			}
			else
			{
				preven(ALIDOUBLE);
				prlocvar(sname, leng);
				*inlcomm = YES;
			}
		}
		else
		{
			pralign(tyalign(type));
			fprintf(initfile, labelfmt, sname);
			if (leng == 8) 
				prquad(&oldvalue);
			else
				prsdata(&oldvalue, (int) leng);
		}
		return;
	}

	repl = 1;
	k -= 8;

	while (k >=8)
	{
		newvalue.i[0] = 0;
		newvalue.i[1] = 0;

		n = read(vdatafile, (char *) &newvalue, sizeof(newvalue));
		if (n < 0)
		{
			err(readerror);
			done(1);
		}

		k -= 8;

		if (oldvalue.i[0] == newvalue.i[0] &&
		    oldvalue.i[1] == newvalue.i[1])
			repl++;
		else
		{
			if (first == YES)
			{
				pralign(tyalign(type));
				fprintf(initfile, labelfmt, sname);
				first = NO;
			}

			if (oldvalue.i[0] == 0 && oldvalue.i[1] == 0)
				prspace(8*repl);
			else
			{
				while (repl-- > 0) {
					prquad(&oldvalue);
				}
			}
			oldvalue.i[0] = newvalue.i[0];
			oldvalue.i[1] = newvalue.i[1];
			repl = 1;
		}
	}

	newvalue.i[0] = 0;
	newvalue.i[1] = 0;

	if (k > 0)
	{
		n = read(vdatafile, (char *) &newvalue, k);
		if (n < 0)
		{
			err(readerror);
			done(1);
		}
	}

	if (oldvalue.i[0] == 0 && oldvalue.i[1] == 0 &&
	    newvalue.i[0] == 0 && newvalue.i[1] == 0) {
		if (first == YES && !SMALLVAR(leng))
		{
			prlocvar(sname, leng);
			*inlcomm = YES;
		}
		else
		{
			if (first == YES)
			{
				pralign(tyalign(type));
				fprintf(initfile, labelfmt, sname);
			}
			prspace(8*repl + k);
		}
		return;
	}

	if (first == YES)	
	{
		pralign(tyalign(type));
		fprintf(initfile, labelfmt, sname);
	}


	if (oldvalue.i[0] == 0 && oldvalue.i[1] == 0)
		prspace(8*repl);
	else
	{
		while (repl-- > 0)
			prquad(&oldvalue);
	}

	prsdata(&newvalue, k);

	return;
}


prquad(p)
union	byte8	*p;
{
	fputs("\t.long\t", initfile);
	print_x(p->i[0], initfile);
	putc(',', initfile);
	print_x(p->i[1], initfile);
	putc('\n', initfile);
}

prsdata(p, len)
union	byte8	*p;
register int len;
{
	static char *longfmt = "\t.long\t0x%x\n";
	static char *wordfmt = "\t.word\t0x%x\n";
	static char *bytefmt = "\t.byte\t0x%x\n";

	register int i;

	i = 0;
	if ((len - i) >= 4)
	{
		fprintf(initfile, longfmt, p->i[0]);
		i += 4;
	}
	if ((len - i) >= 2)
	{
		fprintf(initfile, wordfmt, 0xffff & p->s[i/2]);
		i += 2;
	}
	if ((len - i) > 0)
		fprintf(initfile, bytefmt, 0xff & p->c[i]);

	return;
}
