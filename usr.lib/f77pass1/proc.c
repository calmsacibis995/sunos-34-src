#ifndef lint
static	char sccsid[] = "@(#)proc.c 1.3 87/01/08 SMI"; /* from UCB X.X XX/XX/XX */
#endif


#include "defs.h"

#include "ir_f77.h"

#include "machdefs.h"

#include <a.out.h>
#ifndef N_SO
#	include <stab.h>
#endif

Addrp argvec;

/* start a new procedure */

newproc()
{
	if(parstate != OUTSIDE)
	{
		execerr("missing end statement", CNULL);
		endproc();
	}

	parstate = INSIDE;
	procclass = CLMAIN;	/* default */
}



/* end of procedure. generate variables, epilogs, and prologs */

endproc()
{
	struct Labelblock *lp;

	if(ord(parstate) < ord(INDATA))
		enddcl();
	if(ctlstack >= ctls)
		err("DO loop or BLOCK IF not closed");
	for(lp = labeltab ; lp < labtabend ; ++lp)
		if(lp->stateno!=0 && lp->labdefined==NO)
			errstr("missing statement number %s", convic(lp->stateno) );

	set_bss_offsets();
	outiodata();
	epicode();
	procode();
	donmlist();
	dobss();
	ir_endproc();
	procinit();	/* clean up for next procedure */
}



/* End of declaration section of procedure.  Allocate storage. */

enddcl()
{
	register struct Entrypoint *ep;

	parstate = INEXEC;
	docommon();
	doequiv();
	docomleng();
	for(ep = entries ; ep ; ep = ep->entnextp) {
		doentry(ep);
	}
}

/* ROUTINES CALLED WHEN ENCOUNTERING ENTRY POINTS */

/* Main program or Block data */

startproc(name, class)
char *name;
Vclass class;
{
	register struct Entrypoint *p;
	struct Extsym * main;

	if(class == CLMAIN) {
		main = newentry( mkname(4, "MAIN"));
	} else if(name) {
		/* create entries for named block datas to catch duplicates */
		main = newentry( mkname(strlen(name), name));
	} else { /* unnamed BLOCK DATA */
		main = (struct Extsym *) NULL;
	}

	if(name) {
		procname = copys(name);
	}

	p = ALLOC(Entrypoint);
	p->entrylabel = new_current_block(CNULL,NO);
	p->entryname = main;
	entries = p;

	procclass = class;
	retlabel = newlabel();
	putc(' ', diagfile); fputs(class==CLMAIN ? "MAIN" : "BLOCK DATA", diagfile);
	if(name) {
		putc(' ', diagfile); fputs(name, diagfile);
	};
	putc(':', diagfile); putc('\n', diagfile);
}

/* make an Extsym for an entry point */

struct Extsym *newentry(v)
register Namep v;
{
	register struct Extsym *p;

	p = mkext( varunder(VL, v->varname) );

	if(p==NULL || p->extinit || ! ONEOF(p->extstg, M(STGUNKNOWN)|M(STGEXT)) )
	{
		if(p == 0)
			dclerr("invalid entry name", v);
		else	dclerr("external name already used", v);
		return(0);
	}
	v->vstg = STGAUTO; /* return val is an auto*/
	v->vprocclass = PTHISPROC;
	v->vclass = CLPROC;
	p->extstg = STGEXT; /* func name is STGEXT */
	p->extinit = YES;
	return(p);
}


entrypt(class, type, length, entry, args)
Vclass class;
Vtype type;
ftnint length;
struct Extsym *entry;
chainp args;
{
	register Namep q;
	register struct Entrypoint *p, *ep;

	if(class == CLENTRY) {
		fputs("       entry ", diagfile);
	}
	putc('\t', diagfile); fputs(nounder(XL, entry->extname), diagfile);
	putc(':', diagfile); putc('\n', diagfile);
	q = mkname(VL, nounder(XL,entry->extname) );

	if( (type = lengtype(type, (int) length)) != TYCHAR)
		length = 0;
	if(class == CLPROC)
	{
		procclass = CLPROC;
		proctype = type;
		procleng = length;

		retlabel = newlabel();
		if(type == TYSUBR)
			ret0label = newlabel();
	}

	p = ALLOC(Entrypoint);
	if(class != CLENTRY) {
		procname = entry->extname;
		p->entrylabel = new_current_block(CNULL,NO);
	} else {
		p->entrylabel = newlabel();
		put_label(p->entrylabel);
	}

	if(entries)	/* put new block at end of entries list */
	{
		for(ep = entries; ep->entnextp; ep = ep->entnextp)
			;
		ep->entnextp = p;
	}
	else
		entries = p;

	p->entryname = entry;
	p->arglist = args;
	p->enamep = q;

	if(class == CLENTRY)
	{
		class = CLPROC;
		if(proctype == TYSUBR)
			type = TYSUBR;
	}

	q->vclass = class;
	q->vprocclass = PTHISPROC;
	settype(q, type, (int) length);
	/* hold all initial entry points till end of declarations */
	if(ord(parstate) >= ord(INDATA)) {
		doentry(p);
	}
}

/* generate epilogs */

LOCAL epicode()
{
	register int i;
	int exit_label;

	if(procclass==CLPROC)
	{
		if(proctype==TYSUBR)
		{
			put_label(ret0label);
			if(substars)
				put_fval(TYLONG, ICON(0) );
			put_label(retlabel);
			goret(TYSUBR);
		}
		else	{
			put_label(retlabel);
			if(multitype)
			{
				exit_label = newlabel();
				typeaddr = autovar(1, TYADDR, PNULL);
				put_indirgoto( cpexpr(typeaddr) );
				for(i = 0; i < NTYPES ; ++i)
					if(rtvlabel[i] != 0)
					{
						put_label(rtvlabel[i]);
						retval(i);
						put_goto(exit_label);
					}
				put_label(exit_label);
				goret(TYSUBR);
			}
			else
				retval(proctype);
		}
	}

	else 
	{
		put_label(retlabel);
		goret(TYSUBR);
	}
}


/* generate code to return value of type  t */

LOCAL retval(t)
register Vtype	 t;
{
	register Addrp p;

	switch(t)
	{
	case TYCHAR:
	case TYCOMPLEX:
	case TYDCOMPLEX:
		break;

	case TYLOGICAL:
		t = tylogical;
	case TYADDR:
	case TYSHORT:
	case TYLONG:
		p = (Addrp) cpexpr(retslot);
		p->vtype = t;
		put_fval(t, p);
		break;

	case TYREAL:
	case TYDREAL:
		p = (Addrp) cpexpr(retslot);
		p->vtype = t;
		put_fval(t, p);
		break;

	default:
		badtype("retval", t);
	}
	if(!multitype) {
		goret(t);
	}
}


/* Allocate extra argument array if needed. Generate prologs. */

LOCAL procode()
{
	register struct Entrypoint *p;
	int argvecsize;
	int entrynum;

	if(lastargslot>ARGOFFSET && nentry>1) {
#if TARGET == VAX
		argvecsize = 1+lastargslot/SZADDR;
#else
		argvecsize = lastargslot/SZADDR;
#endif
		argvec = autovar(argvecsize, TYADDR, PNULL);
	} else {
		argvec = NULL;
	}

	entrynum = 0;
	for(p = entries ; p ; p = p->entnextp) {
		prolog(p, argvecsize,argvec, entrynum++);
		if(dbxflag) {
			entrystab(p,procclass);
		}
	}

	put_endproc(procno);
}

/*
 *	manipulate argument lists (allocate argument slot positions)
 *	keep track of return types and labels
 */

LOCAL 
doentry(ep)
struct Entrypoint *ep;
{
	register Vtype type;
	register Namep np;
	chainp p;
	register Namep q;
	Addrp mkarg();

	++nentry;
	if(procclass == CLBLOCK){
		return;
	}

	impldcl( np = mkname(VL, nounder(XL, ep->entryname->extname) ) );
	type = np->vtype;
	if(proctype == TYUNKNOWN)
		if( (proctype = type) == TYCHAR)
			procleng = (np->vleng ? np->vleng->constblock.const.ci : (ftnint) (-1));

	if(proctype == TYCHAR)
	{
		if(type != TYCHAR)
			err("noncharacter entry of character function");
		else if( (np->vleng ? np->vleng->constblock.const.ci : (ftnint) (-1)) != procleng)
			err("mismatched character entry lengths");
	}
	else if(type == TYCHAR)
		err("character entry of noncharacter function");
	else if(type != proctype)
		multitype = YES;

	if(rtvlabel[ord(type)] == 0)
		rtvlabel[ord(type)] = newlabel();
	ep->typelabel = rtvlabel[ord(type)];

	if(type == TYCHAR) {
		if(chslot < 0) {
			chslot = nextarg(TYADDR);
			chlgslot = nextarg(TYLENG);
		}
		np->vstg = STGARG;
		np->voffset = np->vardesc.varno = chslot;
		if(procleng < 0) {
			np->vleng = (expptr) mkarg(TYLENG, chlgslot);
		}
	} else if( ISCOMPLEX(type) ) {
		np->vstg = STGARG;
		if(cxslot < 0)
			cxslot = nextarg(TYADDR);
		np->voffset = np->vardesc.varno = cxslot;
	} else if(type != TYSUBR) {
		if(nentry == 1)
			retslot = autovar(1, TYDREAL, PNULL);
		np->vstg = STGAUTO;
		np->voffset = retslot->segdispl;
	}

/*
**	find the end of the arg list
*/

	for(p = ep->arglist ; p ; p = p->nextp)
		if(! (( q = (Namep) (p->datap) )->vdcldone) ) {
			q->voffset = q->vardesc.varno = nextarg(TYADDR);
		}

	for(p = ep->arglist ; p ; p = p->nextp) {
		if(! (( q = (Namep) (p->datap) )->vdcldone) ) {
			impldcl(q);
			q->vdcldone = YES;
			if((q->vtype == TYCHAR) && q->vclass != CLPROC) {
				if(q->vleng == NULL) {		/* character*(*) */
					q->vleng = (expptr) mkarg(TYLENG, nextarg(TYLENG) );
				} else {
					if(nentry == 1) nextarg(TYLENG);
				}
			}
			if(dbxflag) {
				namestab(q);
			}
		}
	}
}

/*
 * A dummy procedure argument which is a character function
 * is indistinguishable from a character variable from the declarations
 * unless it is declared external.  It is recognized as a character
 * function the first time it is referenced.
 * This procedure adjusts the offsets of any arguments following the
 * point where the implicit length of the character function would
 * have been passed if it had been a character variable.
 *
 * Note: This procedure manipulates the IR leaves which really
 *       shouldn't be done here.
 */
char_function_arg(cnp)
Namep cnp;
{
	Namep np;
	chainp p;


	/* if multiple entry points, no adjustment is necessary since */
	/* the arguments are copied to temporary storage              */
	if(nentry > 1)
		return;

	/* find entry in argument list */
	for(p = entries->arglist ; p; p = p->nextp)
		if(((Namep) (p->datap)) == cnp)
			break;

	if(! p)
		fatal("char_function_arg: cannot find arg");

	for(p = p->nextp; p; p = p->nextp) {
		np = (Namep) (p->datap);
		if(np->vtype==TYCHAR && np->vclass!=CLPROC)
			if(np->vleng && ! ISCONST(np->vleng) ) {
				np->vleng->addrblock.memno -= tysize(TYLENG);
				if (np->vleng->addrblock.leafp)
					((LEAF *) (np->vleng->addrblock.leafp))->val.addr.offset
							-= tysize(TYLENG);
			}
	}
}

LOCAL nextarg(type)
int type;
{
	int k;
	k = lastargslot;
	lastargslot += tysize(type);
	return(k);
}

/* convert bss storage to init for all varriables in data */
mark_init()
{
register struct Hashentry *p;
register Namep q;

	for(p = hashtab ; p<lasthash ; ++p) {
		if((q = p->varp) && q->init) {
			if(q->vstg != STGBSS) {
				warn1("bad initialization for %s ", varstr(VL,q->varname) );
				badstg("mark_init",q->vstg);
			} else {
				q->vstg = STGINIT;
			}
		}
	} /* make sure we don't do this twice */
	parstate = INEXEC;
}

/* generate variable references */
LOCAL 
dobss()
{
	register struct Hashentry *p;
	register Namep q;
	register int i;
	int	align;
	ftnint	leng, iarrl;
	char	*memname();
	Vclass	qclass;
	Vtype	qtype;
	Vstg	qstg;
	struct Equivblock *ep;

	pruse(asmfile, USEBSS);

	for(p = hashtab ; p<lasthash ; ++p)
		if(q = p->varp) {

			qstg = q->vstg;
			qtype = q->vtype;
			qclass = q->vclass;


			if( (qclass==CLUNKNOWN && qstg!=STGARG) ||
			    (qclass==CLVAR && qstg==STGUNKNOWN) )
				warn1("local variable %s never used", varstr(VL,q->varname) );
			else if(qclass==CLPROC && q->vprocclass==PEXTERNAL && qstg!=STGARG)
				mkext(varunder(VL, q->varname)) ->extstg = STGEXT;

			if(qclass==CLVAR && qstg!=STGARG) {
				if(q->vdim && q->vdim->nelt && !ISICON(q->vdim->nelt) )
					dclerr("adjustable dimension on non-argument", q);
				if(qtype==TYCHAR && (q->vleng==NULL || !ISICON(q->vleng)))
					dclerr("adjustable leng on nonargument", q);
			}
		}


	for(i = 0; i < nequiv ; ++i) {
		ep = &eqvclass[i];
		if(ep->init == YES) {
			prlocdata(memname(STGINIT, ep->eqvstart), ep->eqvleng, TYDREAL, 
				ep->initoffset, &ep->inlcomm);
		}
	}

	for(p = hashtab ; p<lasthash ; ++p) if(q = p->varp) {
		qstg = q->vstg;
		qclass = q->vclass;
		if((q->init == YES) && (q->leafp != 0)) {
			if(qstg != STGINIT) {
				fatal(" do_bss: initialized non static");
			}
			prlocdata(memname(STGINIT, q->vardesc.varno), q->varsize,
						q->vtype, q->initoffset, &(q->inlcomm));
		}
	}

	close(vdatafile);
	close(vchkfile);
	unlink(vdatafname);
	unlink(vchkfname);
	vdatahwm = 0;
}



donmlist()
{
	register struct Hashentry *p;
	register Namep q;

	pruse(asmfile, USEINIT);

	for(p=hashtab; p<lasthash; ++p)
		if( (q = p->varp) && q->vclass==CLNAMELIST)
			namelist(q);
}


doext()
{
	struct Extsym *p;

	for(p = extsymtab ; p<nextext ; ++p)
		prext(p);
}




ftnint iarrlen(q)
register Namep q;
{
	ftnint leng;

	leng = tysize(q->vtype);
	if(leng <= 0)
		return(-1);
	if(q->vdim)
		if( q->vdim->nelt && ISICON(q->vdim->nelt) )
			leng *= q->vdim->nelt->constblock.const.ci;
		else	return(-1);
	if(q->vleng)
		if( ISICON(q->vleng) )
			leng *= q->vleng->constblock.const.ci;
		else 	return(-1);
	return(leng);
}

/* This routine creates a static block representing the namelist.
   An equivalent declaration of the structure produced is:
	struct namelist
		{
		char namelistname[16];
		struct namelistentry
			{
			char varname[16];
			char *varaddr;
			int type; # negative means -type= number of chars
			struct dimensions *dimp; # null means scalar
			} names[];
		};

	struct dimensions
		{
		int numberofdimensions;
		int numberofelements
		int baseoffset;
		int span[numberofdimensions];
		};
   where the namelistentry list terminates with a null varname
   If dimp is not null, then the corner element of the array is at
   varaddr.  However,  the element with subscripts (i1,...,in) is at
   varaddr - dimp->baseoffset + sizeoftype * (i1+span[0]*(i2+span[1]*...)
*/

namelist(np)
Namep np;
{
	register chainp q;
	register Namep v;
	register struct Dimblock *dp;
	char *memname();
	Vtype type;
	int dimno, dimoffset;
	flag bad;


	preven(ALILONG);
	fprintf(asmfile, LABELFMT, memname(STGINIT, np->vardesc.varno));
	putstr(asmfile, varstr(VL, np->varname), 16);
	dimno = ++lastvarno;
	dimoffset = 0;
	bad = NO;

	for(q = np->varxptr.namelist ; q ; q = q->nextp)
	{
		vardcl( v = (Namep) (q->datap) );
		type = v->vtype;
		if( ONEOF(v->vstg, MSKSTATIC) )
		{
			preven(ALILONG);
			putstr(asmfile, varstr(VL,v->varname), 16);
			praddr(asmfile, v->vstg, v->vardesc.varno, v->voffset);
			prconi(asmfile, TYLONG,
				type==TYCHAR ? -(v->vleng->constblock.const.ci)
					     : (ftnint) type);
			if(v->vdim)
			{
				praddr(asmfile, STGINIT, dimno, (ftnint)dimoffset);
				dimoffset += 3 + v->vdim->ndim;
			}
			else
				praddr(asmfile, STGNULL,0,(ftnint) 0);
		}
		else
		{
			dclerr("may not appear in namelist", v);
			bad = YES;
		}
	}

	if(bad)
		return;

	putstr(asmfile, "", 16);

	if(dimoffset > 0)
	{
		fprintf(asmfile, LABELFMT, memname(STGINIT,dimno));
		for(q = np->varxptr.namelist ; q ; q = q->nextp)
			if(dp = q->datap->nameblock.vdim)
			{
				int i;
				prconi(asmfile, TYLONG, (ftnint) (dp->ndim) );
				prconi(asmfile, TYLONG,
				(ftnint) (dp->nelt->constblock.const.ci) );
				prconi(asmfile, TYLONG,
				(ftnint) (dp->baseoffset->constblock.const.ci));
				for(i=0; i<dp->ndim ; ++i)
					prconi(asmfile, TYLONG,
					dp->dims[i].dimsize->constblock.const.ci);
			}
	}

}

LOCAL docommon()
{
register struct Extsym *p;
register chainp q;
struct Dimblock *t;
expptr neltp;
register Namep v;
ftnint size;
Vtype type;
char buff[100];

	for(p = extsymtab ; p<nextext ; ++p)
		if(p->extstg==STGCOMMON) {
			for(q = p->extp ; q ; q = q->nextp) {
				v = (Namep) (q->datap);
				if(v->vdcldone == NO)
					vardcl(v);
				type = v->vtype;
				if(p->extleng % tyalign(type) != 0) {
#ifdef MC68000		/* on 68000 everything could be 16 bit aligned */
					if( p->extleng % ALISHORT != 0 ) {
						dclerr("common alignment", v);
						p->extleng = roundup(p->extleng,tyalign(type));
					} else {
						warn1( "Misalignment on a short word boundary: %s", varstr(VL, v->varname ) );
					}
#else
					dclerr("common alignment", v);
					p->extleng = roundup(p->extleng, tyalign(type));
#endif
				}
				v->voffset = p->extleng;
				v->vardesc.varno = p-extsymtab;
				if(type == TYCHAR)
					size = v->vleng->constblock.const.ci;
				else	size = tysize(type);
				if(t = v->vdim) {
					if( (neltp = t->nelt) && ISCONST(neltp) ) {
						size *= neltp->constblock.const.ci;
					} else {
						dclerr("adjustable array in common", v);
					}
				}
				p->extleng += size;
			}

		}
}

LOCAL docomleng()
{
register struct Extsym *p;
char buff[100];
register chainp q;
Namep v;

	for(p = extsymtab ; p < nextext ; ++p) {
		if(p->extstg == STGCOMMON) {

			if(p->maxleng!=0 && p->extleng!=0 && p->maxleng!=p->extleng
			    && bcmp("_BLNK__ ",p->extname, XL) )
				warn1("incompatible lengths for common block %s",
				nounder(XL, p->extname) );
			if(p->maxleng < p->extleng) {
				p->maxleng = p->extleng;
			}
			if(dbxflag) {
				prstab_com(varstr(XL,p->extname), N_BCOMM);
				for(q = p->extp ; q ; q = q->nextp) {
					v = (Namep) (q->datap);
					namestab(v);
				}
				prstab_com(varstr(XL,p->extname), N_ECOMM);
			}
			frchain( &(p->extp) );
			p->extleng = 0;
		}
	}
}




/* ROUTINES DEALING WITH AUTOMATIC AND TEMPORARY STORAGE */

/*  frees a temporary block  */

frtemp(p)
Tempp p;
{
	Addrp t;

	t = (Addrp) p;

	/* restore clobbered character string lengths */
	if(t->vtype==TYCHAR && t->varleng!=0)
	{
		frexpr(t->vleng);
		t->vleng = ICON(t->varleng);
	}

	/* put block on chain of temps to be reclaimed */
	holdtemps = mkchain(t, holdtemps);
}



/* allocate an automatic variable slot */

Addrp 
autovar(nelt, t, lengp)
register int nelt;
Vtype t;
expptr lengp;
{
	ftnint leng;
	register Addrp q;
	register int size;
	int alignment;

	if(t == TYCHAR) {
		if( lengp && ISICON(lengp) ) {
			leng = lengp->constblock.const.ci;
		} else	{
			fatal("automatic variable of nonconstant length");
		}
	} else {
		leng = tysize(t);
	}
	/* align all temps on 4 byte boundaries since we don't know which will have
	** their addresses taken 
	*/
	alignment = tyalign(TYREAL);
	autoleng = roundup(autoleng, alignment);

	q = ALLOC(Addrblock);
	size = nelt*leng;
	q->tag = TADDR;
	q->vtype = t;
	if(t == TYCHAR)
	{
		q->vleng = ICON(leng);
		q->varleng = leng;
	}
	q->vstg = STGAUTO;
	q->memno = newlabel();
	q->ntempelt = nelt;
	q->memoffset = ICON( 0 );
	q->varsize = size;

	autoleng += size;
	q->segdispl =  -autoleng;

	return(q);
}



/*
 *  create a temporary block (TTEMP) when optimizing,
 *  an ordinary TADDR block when not optimizing
 */

Tempp mktmpn(nelt, type, lengp)
int nelt;
register Vtype type;
expptr lengp;
{
	return ( (Tempp) mkaltmpn(nelt,type,lengp) );
}



Addrp mktemp(type, lengp)
Vtype type;
expptr lengp;
{
	return( (Addrp) mktmpn(1,type,lengp) );
}



/*  allocate a temporary location for the given temporary block;
    if already allocated, return its location  */

Addrp altmpn(tp)
Tempp tp;

{
	Addrp t, q;

	if (tp->tag != TTEMP)
		badtag ("altmpn",tp->tag);

	t = tp->memalloc;
	if (t->vstg != STGUNKNOWN)
		return (t);

	q = mkaltmpn (tp->ntempelt, tp->vtype, tp->vleng);
	bcopy((char*)q, (char*)t, sizeof(struct Addrblock));
	ckfree( (charptr) q);
	return(t);
}



/*  create and allocate space immediately for a temporary  */

Addrp mkaltemp(type,lengp)
Vtype type;
expptr lengp;
{
	return (mkaltmpn(1,type,lengp));
}



Addrp mkaltmpn(nelt,type,lengp)
int nelt;
register Vtype type;
expptr lengp;
{
	ftnint leng;
	chainp p, oldp;
	register Addrp q;

	if(type==TYUNKNOWN || type==TYERROR)
		badtype("mkaltmpn", type);

	if(type==TYCHAR)
		if( lengp && ISICON(lengp) )
			leng = lengp->constblock.const.ci;
		else	{
			err("adjustable length");
			return( (Addrp) errnode() );
		}

	/*
	 * if a temporary of appropriate shape is on the templist,
	 * remove it from the list and return it
	 */

	for(oldp=CHNULL, p=templist  ;  p  ;  oldp=p, p=p->nextp)
	{
		q = (Addrp) (p->datap);
		if(q->vtype==type && q->ntempelt==nelt &&
		    (type!=TYCHAR || q->vleng->constblock.const.ci==leng) )
		{
			if(oldp)
				oldp->nextp = p->nextp;
			else
				templist = p->nextp;
			ckfree( (charptr) p);

			if (debugflag[14])
				fprintf(diagfile,"mkaltmpn reusing offset %d\n",
				q->memoffset->constblock.const.ci);
			return(q);
		}
	}
	q = autovar(nelt, type, lengp);
	q->istemp = YES;

	if (debugflag[14])
		fprintf(diagfile,"mkaltmpn new offset %d\n",
		q->memoffset->constblock.const.ci);
	return(q);
}

/* VARIOUS ROUTINES FOR PROCESSING DECLARATIONS */

struct Extsym *comblock(len, s)
register int len;
register char *s;
{
	struct Extsym *p;

	if(len == 0)
	{
		s = BLANKCOMMON;
		len = strlen(s);
	}
	p = mkext( varunder(len, s) );
	if(p->extstg == STGUNKNOWN)
		p->extstg = STGCOMMON;
	else if(p->extstg != STGCOMMON)
	{
		errstr("%s cannot be a common block name", s);
		return(0);
	}

	return( p );
}


incomm(c, v)
struct Extsym *c;
Namep v;
{
	if(v->vstg != STGUNKNOWN)
		dclerr("incompatible common declaration", v);
	else
	{
		v->vstg = STGCOMMON;
		c->extp = hookup(c->extp, mkchain(v,CHNULL) );
	}
}

settype(v, type, length)
register Namep  v;
register Vtype type;
register int length;
{
	if(type == TYUNKNOWN)
		return;

	if(type==TYSUBR && v->vtype!=TYUNKNOWN && v->vstg==STGARG) {
		v->vtype = TYSUBR;
		frexpr(v->vleng);
	} else if(ord(type) < ord(TYUNKNOWN)) {	/* storage class set */
		if(v->vstg == STGUNKNOWN)
			v->vstg = (Vstg) -ord(type);
		else if(v->vstg != (Vstg) -ord(type))
			dclerr("incompatible storage declarations", v);
	} else if(v->vtype == TYUNKNOWN) {
		if( (v->vtype = lengtype(type, length))==TYCHAR && length>=0)
			v->vleng = ICON(length);
	} else if(v->vtype!=type || 
	       (type==TYCHAR && v->vleng->constblock.const.ci!=length) ) {
		dclerr("incompatible type declarations", v);
	}
}

Vtype
lengtype(type, length)
register Vtype type;
register int length;
{
	switch(type)
	{
	case TYREAL:
		if(length == 8)
			return(TYDREAL);
		if(length == 4)
			goto ret;
		break;

	case TYCOMPLEX:
		if(length == 16)
			return(TYDCOMPLEX);
		if(length == 8)
			goto ret;
		break;

	case TYSHORT:
	case TYDREAL:
	case TYDCOMPLEX:
	case TYCHAR:
	case TYUNKNOWN:
	case TYSUBR:
	case TYERROR:
		goto ret;

	case TYLOGICAL:
		if(length == tysize(TYLOGICAL))
			goto ret;
		break;

	case TYINT:
	case TYLONG:
		if(length == 0)
			return(tyint);
		if(length == 2)
			return(TYSHORT);
		if(length == 4)
			return(TYLONG);
		break;
	default:
		badtype("lengtype", type);
	}

	if(length != 0)
		err("incompatible type-length combination");

ret:
	return(type);
}





setintr(v)
register Namep  v;
{
	register int k;

	if(v->vstg == STGUNKNOWN)
		v->vstg = STGINTR;
	else if(v->vstg!=STGINTR)
		dclerr("incompatible use of intrinsic function", v);
	if(v->vclass==CLUNKNOWN)
		v->vclass = CLPROC;
	if(v->vprocclass == PUNKNOWN)
		v->vprocclass = PINTRINSIC;
	else if(v->vprocclass != PINTRINSIC)
		dclerr("invalid intrinsic declaration", v);
	if(k = intrfunct(v->varname))
		v->vardesc.varno = k;
	else
		dclerr("unknown intrinsic function", v);
}



setext(v)
register Namep  v;
{
	if(v->vclass == CLUNKNOWN)
		v->vclass = CLPROC;
	else if(v->vclass != CLPROC)
		dclerr("invalid external declaration", v);

	if(v->vprocclass == PUNKNOWN)
		v->vprocclass = PEXTERNAL;
	else if(v->vprocclass != PEXTERNAL)
		dclerr("invalid external declaration", v);
}




/* create dimensions block for array variable */

setbound(v, nd, dims)
register Namep  v;
int nd;
struct { 
	expptr lb, ub; 
} 
dims[ ];
{
	register expptr q, t;
	register struct Dimblock *p;
	int i;

	if(v->vclass == CLUNKNOWN)
		v->vclass = CLVAR;
	else if(v->vclass != CLVAR)
	{
		dclerr("only variables may be arrays", v);
		return;
	}

	v->vdim = p = (struct Dimblock *)
		ckalloc( sizeof(int) + (3+6*nd)*sizeof(expptr) );
	p->ndim = nd;
	p->nelt = ICON(1);

	for(i=0 ; i<nd ; ++i)
	{
		if(dbxflag) {
			/* 
			 * Save the bounds trees built up by 
			 * the grammar routines for use in stabs 
			 */
			if(dims[i].lb == NULL) 
				p->dims[i].lb=ICON(1);
			else 
				p->dims[i].lb= (expptr) cpexpr(dims[i].lb);
			if(ISCONST(p->dims[i].lb)) 
				p->dims[i].lbaddr = (expptr) PNULL;
			else 
				p->dims[i].lbaddr = (expptr) autovar(1, tyint, PNULL);

			if(dims[i].ub == NULL) 
				p->dims[i].ub=ICON(1);
			else 
				p->dims[i].ub = (expptr) cpexpr(dims[i].ub);
			if(ISCONST(p->dims[i].ub)) 
				p->dims[i].ubaddr = (expptr) PNULL;
			else 
				p->dims[i].ubaddr = (expptr) autovar(1, tyint, PNULL);
		}
		if( (q = dims[i].ub) == NULL)
		{
			if(i == nd-1)
			{
				frexpr(p->nelt);
				p->nelt = NULL;
			}
			else
				err("only last bound may be asterisk");
			p->dims[i].dimsize = ICON(1);
			;
			p->dims[i].dimexpr = NULL;
		}
		else
		{
			if(dims[i].lb)
			{
				q = mkexpr(OPMINUS, q, cpexpr(dims[i].lb));
				q = mkexpr(OPPLUS, q, ICON(1) );
			}
			if( ISCONST(q) )
			{
				p->dims[i].dimsize = q;
				p->dims[i].dimexpr = (expptr) PNULL;
			}
			else	{
				p->dims[i].dimsize = (expptr) autovar(1, tyint, PNULL);
				p->dims[i].dimexpr = q;
			}
			if(p->nelt)
				p->nelt = mkexpr(OPSTAR, p->nelt,
				cpexpr(p->dims[i].dimsize) );
		}
	}

	q = dims[nd-1].lb;
	if(q == NULL)
		q = ICON(1);

	for(i = nd-2 ; i>=0 ; --i)
	{
		t = dims[i].lb;
		if(t == NULL)
			t = ICON(1);
		if(p->dims[i].dimsize)
			q = mkexpr(OPPLUS, t, mkexpr(OPSTAR, cpexpr(p->dims[i].dimsize), q) );
	}

	if( ISCONST(q) )
	{
		p->baseoffset = q;
		p->basexpr = NULL;
	}
	else
	{
		p->baseoffset = (expptr) autovar(1, tyint, PNULL);
		p->basexpr = q;
	}
}
