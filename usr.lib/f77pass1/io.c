#ifndef lint
static	char sccsid[] = "@(#)io.c 1.4 87/03/23 SMI"; /* from UCB X.X XX/XX/XX */
#endif

/* Routines to generate code for I/O statements.
   Some corrections and improvements due to David Wasley, U. C. Berkeley
*/

/* TEMPORARY */
#define TYIOINT TYLONG
#define SZIOINT SZLONG

#include "defs.h"
#include "io.h"
#include "ir_f77.h"
extern NODE *map_addr();	/* called from ioseta() */
extern Addrp mkaddr();


LOCAL char ioroutine[XL+1];

LOCAL int ioendlab;
LOCAL int ioerrlab;
LOCAL int endbit;
LOCAL int errbit;
LOCAL int jumplab;
LOCAL int skiplab;
LOCAL int statstruct = NO;
LOCAL ftnint blklen;

LOCAL offsetlist *mkiodata();
LOCAL expptr addrof();
chainp ioblkp_addr_list;

#define V(z)	ioc[z].iocval

#define IOALL 07777

LOCAL struct Ioclist
{
	char *iocname;
	int iotype;
	expptr iocval;
} 
ioc[ ] =
{
	{ 
		"", 0 	}
	,
	{ 
		"unit", IOALL 	}
	,
	{ 
		"fmt", M(IOREAD) | M(IOWRITE) 	}
	,
	{ 
		"err", IOALL 	}
	,
	{ 
		"end", M(IOREAD) 	}
	,
	{ 
		"iostat", IOALL 	}
	,
	{ 
		"rec", M(IOREAD) | M(IOWRITE) 	}
	,
	{ 
		"recl", M(IOOPEN) | M(IOINQUIRE) 	}
	,
	{ 
		"file", M(IOOPEN) | M(IOINQUIRE) 	}
	,
	{ 
		"status", M(IOOPEN) | M(IOCLOSE) 	}
	,
	{ 
		"access", M(IOOPEN) | M(IOINQUIRE) 	}
	,
	{ 
		"form", M(IOOPEN) | M(IOINQUIRE) 	}
	,
	{ 
		"blank", M(IOOPEN) | M(IOINQUIRE) 	}
	,
	{ 
		"exist", M(IOINQUIRE) 	}
	,
	{ 
		"opened", M(IOINQUIRE) 	}
	,
	{ 
		"number", M(IOINQUIRE) 	}
	,
	{ 
		"named", M(IOINQUIRE) 	}
	,
	{ 
		"name", M(IOINQUIRE) 	}
	,
	{ 
		"sequential", M(IOINQUIRE) 	}
	,
	{ 
		"direct", M(IOINQUIRE) 	}
	,
	{ 
		"formatted", M(IOINQUIRE) 	}
	,
	{ 
		"unformatted", M(IOINQUIRE) 	}
	,
	{ 
		"nextrec", M(IOINQUIRE) 	}
	,
	{ 
		"fileopt", M(IOOPEN) | M(IOINQUIRE) 	}
} 
;

#define NIOS (sizeof(ioc)/sizeof(struct Ioclist) - 1)
#define MAXIO	SZFLAG + 10*SZIOINT + 16*SZADDR

#define IOSUNIT 1
#define IOSFMT 2
#define IOSERR 3
#define IOSEND 4
#define IOSIOSTAT 5
#define IOSREC 6
#define IOSRECL 7
#define IOSFILE 8
#define IOSSTATUS 9
#define IOSACCESS 10
#define IOSFORM 11
#define IOSBLANK 12
#define IOSEXISTS 13
#define IOSOPENED 14
#define IOSNUMBER 15
#define IOSNAMED 16
#define IOSNAME 17
#define IOSSEQUENTIAL 18
#define IOSDIRECT 19
#define IOSFORMATTED 20
#define IOSUNFORMATTED 21
#define IOSNEXTREC 22
#define IOSFILEOPT 23

#define IOSTP V(IOSIOSTAT)


/* offsets in generated structures */

#define SZFLAG SZIOINT

/* offsets for external READ and WRITE statements */

#define XERR 0
#define XUNIT	SZFLAG
#define XEND	SZFLAG + SZIOINT
#define XFMT	2*SZFLAG + SZIOINT
#define XREC	2*SZFLAG + SZIOINT + SZADDR
#define XRLEN	2*SZFLAG + 2*SZADDR
#define XRNUM	2*SZFLAG + 2*SZADDR + SZIOINT

/* offsets for internal READ and WRITE statements */

#define XIERR	0
#define XIUNIT	SZFLAG
#define XIEND	SZFLAG + SZADDR
#define XIFMT	2*SZFLAG + SZADDR
#define XIRLEN	2*SZFLAG + 2*SZADDR
#define XIRNUM	2*SZFLAG + 2*SZADDR + SZIOINT
#define XIREC	2*SZFLAG + 2*SZADDR + 2*SZIOINT

/* offsets for OPEN statements */

#define XFNAME	SZFLAG + SZIOINT
#define XFNAMELEN	SZFLAG + SZIOINT + SZADDR
#define XSTATUS	SZFLAG + 2*SZIOINT + SZADDR
#define XACCESS	SZFLAG + 2*SZIOINT + 2*SZADDR
#define XFORMATTED	SZFLAG + 2*SZIOINT + 3*SZADDR
#define XRECLEN	SZFLAG + 2*SZIOINT + 4*SZADDR
#define XBLANK	SZFLAG + 3*SZIOINT + 4*SZADDR
#define XFILEOPT SZFLAG + 3*SZIOINT + 5*SZADDR
#define XFILEOPTLEN SZFLAG + 3*SZIOINT + 6*SZADDR

/* offset for CLOSE statement */

#define XCLSTATUS	SZFLAG + SZIOINT

/* offsets for INQUIRE statement */

#define XFILE	SZFLAG + SZIOINT
#define XFILELEN	SZFLAG + SZIOINT + SZADDR
#define XEXISTS	SZFLAG + 2*SZIOINT + SZADDR
#define XOPEN	SZFLAG + 2*SZIOINT + 2*SZADDR
#define XNUMBER	SZFLAG + 2*SZIOINT + 3*SZADDR
#define XNAMED	SZFLAG + 2*SZIOINT + 4*SZADDR
#define XNAME	SZFLAG + 2*SZIOINT + 5*SZADDR
#define XNAMELEN	SZFLAG + 2*SZIOINT + 6*SZADDR
#define XQACCESS	SZFLAG + 3*SZIOINT + 6*SZADDR
#define XQACCLEN	SZFLAG + 3*SZIOINT + 7*SZADDR
#define XSEQ	SZFLAG + 4*SZIOINT + 7*SZADDR
#define XSEQLEN	SZFLAG + 4*SZIOINT + 8*SZADDR
#define XDIRECT	SZFLAG + 5*SZIOINT + 8*SZADDR
#define XDIRLEN	SZFLAG + 5*SZIOINT + 9*SZADDR
#define XFORM	SZFLAG + 6*SZIOINT + 9*SZADDR
#define XFORMLEN	SZFLAG + 6*SZIOINT + 10*SZADDR
#define XFMTED	SZFLAG + 7*SZIOINT + 10*SZADDR
#define XFMTEDLEN	SZFLAG + 7*SZIOINT + 11*SZADDR
#define XUNFMT	SZFLAG + 8*SZIOINT + 11*SZADDR
#define XUNFMTLEN	SZFLAG + 8*SZIOINT + 12*SZADDR
#define XQRECL	SZFLAG + 9*SZIOINT + 12*SZADDR
#define XNEXTREC	SZFLAG + 9*SZIOINT + 13*SZADDR
#define XQBLANK	SZFLAG + 9*SZIOINT + 14*SZADDR
#define XQBLANKLEN	SZFLAG + 9*SZIOINT + 15*SZADDR
#define XQFILEOPT SZFLAG + 10*SZIOINT + 15*SZADDR
#define XQFILEOPTLEN SZFLAG + 10*SZIOINT + 16*SZADDR

fmtstmt(lp)
register struct Labelblock *lp;
{
	if(lp == NULL)
	{
		execerr("unlabeled format statement" , CNULL);
		return(-1);
	}
	if(lp->labtype == LABUNKNOWN)
	{
		lp->labtype = LABFORMAT;
		lp->labelno = newlabel();
	}
	else if(lp->labtype != LABFORMAT)
	{
		execerr("bad format number", CNULL);
		return(-1);
	}
	return(lp->labelno);
}



setfmt(lp)
struct Labelblock *lp;
{
	int n;
	char *s, *lexline();

	s = lexline(&n);
	put_fmtstr(lp->labelno,mkstrcon(n,s));
	flline();
}



startioctl()
{
	register int i;

	inioctl = YES;
	nioctl = 0;
	ioformatted = UNFORMATTED;
	for(i = 1 ; i<=NIOS ; ++i)
		V(i) = NULL;
}



endioctl()
{
	int i;
	expptr p;

	inioctl = NO;

	/* set up for error recovery */

	ioerrlab = ioendlab = skiplab = jumplab = 0;

	if(p = V(IOSEND))
		if(ISICON(p))
			ioendlab = execlab(p->constblock.const.ci) ->labelno;
		else
			err("bad end= clause");

	if(p = V(IOSERR))
		if(ISICON(p))
			ioerrlab = execlab(p->constblock.const.ci) ->labelno;
		else
			err("bad err= clause");

	if(IOSTP)
		if(IOSTP->tag!=TADDR || ! ISINT(IOSTP->addrblock.vtype) )
		{
			err("iostat must be an integer variable");
			frexpr(IOSTP);
			IOSTP = NULL;
		}

	if(iostmt == IOREAD)
	{
		if(IOSTP)
		{
			if(ioerrlab && ioendlab && ioerrlab==ioendlab)
				jumplab = ioerrlab;
			else
				skiplab = jumplab = newlabel();
		}
		else	{
			if(ioerrlab && ioendlab && ioerrlab!=ioendlab)
			{
				IOSTP = (expptr) mktemp(TYLONG, PNULL);
				skiplab = jumplab = newlabel();
			}
			else
				jumplab = (ioerrlab ? ioerrlab : ioendlab);
		}
	}
	else if(iostmt == IOWRITE)
	{
		if(IOSTP && !ioerrlab)
			skiplab = jumplab = newlabel();
		else
			jumplab = ioerrlab;
	}
	else
		jumplab = ioerrlab;

	endbit = IOSTP!=NULL || ioendlab!=0;	/* for use in startrw() */
	errbit = IOSTP!=NULL || ioerrlab!=0;
	if(iostmt!=IOREAD && iostmt!=IOWRITE) {
		if(ioblkp == NULL) {
			ioblkp = autovar( (MAXIO+SZIOINT-1)/SZIOINT , TYIOINT, PNULL);
			ioblkp->isaggregate = ISSTRUCT;
		}
		ioset(TYIOINT, XERR, ICON(errbit));
	}

	switch(iostmt)
	{
	case IOOPEN:
		dofopen();  
		break;

	case IOCLOSE:
		dofclose();  
		break;

	case IOINQUIRE:
		dofinquire();  
		break;

	case IOBACKSPACE:
		dofmove("f_back"); 
		break;

	case IOREWIND:
		dofmove("f_rew");  
		break;

	case IOENDFILE:
		dofmove("f_end");  
		break;

	case IOREAD:
	case IOWRITE:
		startrw();  
		break;

	default:
		fatali("impossible iostmt %d", iostmt);
	}
	for(i = 1 ; i<=NIOS ; ++i)
		if(i!=IOSIOSTAT && V(i)!=NULL)
			frexpr(V(i));
}



findiocname()
{
	register int i;
	int found, mask;

	found = 0;
	mask = M(iostmt);
	for(i = 1 ; i <= NIOS ; ++i)
		if(toklen==strlen(ioc[i].iocname) &&
                   test_for_keyword(token, ioc[i].iocname))
			if(ioc[i].iotype & mask)
				return(i);
			else	found = i;
	if(found)
		errstr("invalid control %s for statement", ioc[found].iocname);
	else
		errstr("unknown iocontrol %s", varstr(toklen, token) );
	return(IOSBAD);
}


ioclause(n, p)
register int n;
register expptr p;
{
	struct Ioclist *iocp;

	++nioctl;
	if(n == IOSBAD)
		return;
	if(n == IOSPOSITIONAL)
	{
		if(nioctl > IOSFMT)
		{
			err("illegal positional iocontrol");
			return;
		}
		n = nioctl;
	}

	if(p == NULL)
	{
		if(n == IOSUNIT)
			p = (expptr) (iostmt==IOREAD ? IOSTDIN : IOSTDOUT);
		else if(n != IOSFMT)
		{
			err("illegal * iocontrol");
			return;
		}
	}
	if(n == IOSFMT)
		ioformatted = (p==NULL ? LISTDIRECTED : FORMATTED);

	iocp = & ioc[n];
	if(iocp->iocval == NULL)
	{
		if(n!=IOSFMT && ( n!=IOSUNIT || (p!=NULL && p->headblock.vtype!=TYCHAR) ) )
			p = fixtype(p);
		iocp->iocval = p;
	}
	else
		errstr("iocontrol %s repeated", iocp->iocname);
}

/* io list item */

doio(list)
chainp list;
{
	expptr call0();

	if(ioformatted == NAMEDIRECTED)
	{
		if(list)
			err("no I/O list allowed in NAMELIST read/write");
	}
	else
	{
		doiolist(list);
		ioroutine[0] = 'e';
		putiocall( call0(TYLONG, ioroutine) );
	}
}





LOCAL doiolist(p0)
chainp p0;
{
	chainp p;
	register tagptr q;
	register expptr qe;
	register Namep qn;
	Addrp tp;
	int range;
	expptr expr;

	for (p = p0 ; p ; p = p->nextp)
	{
		q = p->datap;
		if(q->tag == TIMPLDO)
		{
			exdo(range=newlabel(), q->impldoblock.impdospec);
			doiolist(q->impldoblock.datalist);
			enddo(range);
			ckfree( (charptr) q);
		} else	{
			if(q->tag==TPRIM && q->primblock.argsp==NULL
			    && q->primblock.namep->vdim!=NULL) {
				vardcl(qn = q->primblock.namep);
				if(qn->vdim->nelt) {
					putio( fixtype(cpexpr(qn->vdim->nelt)), mkaddr(qn) );
				} else {
					err("attempt to i/o array of unknown size");
				}
			}
			else if( (qe = fixtype(cpexpr(q)))->tag==TADDR)
				putio(ICON(1), qe);
			else if(qe->headblock.vtype != TYERROR)
			{
				if(iostmt == IOWRITE)
				{
					ftnint lencat();
					expptr qvl;
					qvl = NULL;
					if( ISCHAR(qe) ) {
						qvl = (expptr)
							cpexpr(qe->headblock.vleng);
						tp = mktemp(qe->headblock.vtype,
						ICON(lencat(qe)));
					} else {
						if(ISICON(( (tagptr) qe))) {
							qe = mkconv(tyint,qe);
						}
						tp = mktemp(qe->headblock.vtype, qe->headblock.vleng);
					}
					put_eq (cpexpr(tp),qe);
					if(qvl)	/* put right length on block */
					{
						frexpr(tp->vleng);
						tp->vleng = qvl;
					}
					putio(ICON(1), tp);
				}
				else
					err("non-left side in READ list");
			}
			frexpr(q);
		}
	}
	frchain( &p0 );
}





LOCAL 
putio(nelt, addr)
expptr nelt;
register expptr addr;
{
	Vtype	type;
	char	*routine;
	expptr	size;
	expptr	typeexpr;
	register expptr q;

	type = addr->headblock.vtype;
	if(ioformatted!=LISTDIRECTED && ISCOMPLEX(type) )
	{
		nelt = mkexpr(OPSTAR, ICON(2), nelt);
		type = cxtoreal(type);
	}

	/* 
	 * The length of an item is passed to the I/O routines.
	 * For characters this is vleng, for other types this is
	 * the size of the type.  For non-character types, a third
 	 * parameter must be passed.  For character types, putcall()
	 * takes care of this.
	 */
	nelt = fixtype(mkconv(TYLONG, nelt));
	size = (expptr) ICON(tysize(type));
	size->constblock.vtype = TYLENG;
	if(ioformatted == LISTDIRECTED) {
		typeexpr = (expptr) ICON(type);
		typeexpr->constblock.vtype = TYLONG;
		routine = ( iostmt == IOREAD ? "do_l_in" : "do_l_out");
		if (type == TYCHAR) {
			q = call3(TYLONG, routine, typeexpr, 
				nelt, addr);
		} else {
			q = call4(TYLONG, routine, typeexpr, 
				nelt, addr, size);
		}
	} else {
		if(ioformatted == FORMATTED) {
			routine = ( iostmt == IOREAD ? "do_f_in" : "do_f_out");
		} else {
			routine = ( iostmt == IOREAD ? "do_u_in" : "do_u_out");
		}
		if (type == TYCHAR) {
			q = call2(TYLONG, routine, nelt, addr);
		} else {
			q = call3(TYLONG, routine, nelt, addr,
				size);
		}
	}
	putiocall(q);
}




endio()
{
	if(skiplab)
	{
		put_label(skiplab);
		if(ioendlab)
		{
			expptr test;
			test = mkexpr(OPGE, cpexpr(IOSTP), ICON(0));
			put_if (test,ioendlab);
		}
		if(ioerrlab)
		{
			expptr test;
			test = mkexpr
			    ( ((iostmt==IOREAD||iostmt==IOWRITE) ? OPLE : OPEQ),
			cpexpr(IOSTP), ICON(0));
			put_if (test,ioerrlab);
		}
	}
	if(IOSTP)
		frexpr(IOSTP);
}



LOCAL 
putiocall(q)
register expptr q;
{
	if(IOSTP) { /* save io call status - then test it if necessary */
		q->headblock.vtype = TYLONG;
		q = fixexpr( mkexpr(OPASSIGN, cpexpr(IOSTP), q));
		put_expr(q);
		if(jumplab) {
			put_if (mkexpr(OPEQ,cpexpr(IOSTP),ICON(0)),jumplab);
		}
	} else {
		if(jumplab) {
			put_if (mkexpr(OPEQ,q,ICON(0)),jumplab);
		} else {
			put_expr(q);
		}
	}

	frchain(&ioblkp_addr_list);
}

LOCAL
startrw()
{
	register expptr p;
	register Namep np;
	register Addrp unitp, fmtp, recp, tioblkp;
	register expptr nump;
	register ioblock *t;
	expptr mkaddcon();
	int k;
	flag intfile, sequential, ok, varfmt;
	int offset;

	/* First look at all the parameters and determine what is to be done */

	ok = YES;
	statstruct = YES;

	intfile = NO;
	if(p = V(IOSUNIT)) {
		if( ISINT(p->headblock.vtype) )
			unitp = (Addrp) cpexpr(p);
		else if(p->headblock.vtype == TYCHAR) {
			intfile = YES;
			if(p->tag==TPRIM && p->primblock.argsp==NULL &&
			    (np = p->primblock.namep)->vdim!=NULL) {
				vardcl(np);
				if(np->vdim->nelt) {
					nump = (expptr) cpexpr(np->vdim->nelt);
					if( ! ISCONST(nump) )
						statstruct = NO;
				} else {
					err("attempt to use internal unit array of unknown size");
					ok = NO;
					nump = ICON(1);
				}
				unitp = mkaddr(np);
			} else	{
				nump = ICON(1);
				unitp = (Addrp) fixtype(cpexpr(p));
			}
			if(! isstatic(unitp) )
				statstruct = NO;
		} else {
			err("bad unit specifier type");
			ok = NO;
		}
	} else {
		err("bad unit specifier");
		ok = NO;
	}

	sequential = YES;
	if(p = V(IOSREC)) {
		if( ISINT(p->headblock.vtype) ) {
			recp = (Addrp) cpexpr(p);
			sequential = NO;
		} else	{
			err("bad REC= clause");
			ok = NO;
		}
	} else {
		recp = NULL;
	}


	varfmt = YES;
	fmtp = NULL;
	if(p = V(IOSFMT)) {
		if(p->tag==TPRIM && p->primblock.argsp==NULL) {
			np = p->primblock.namep;
			if(np->vclass == CLNAMELIST) {
				ioformatted = NAMEDIRECTED;
				fmtp = (Addrp) fixtype(p);
				goto endfmt;
			}
			vardcl(np);
			if(np->vdim) {
				statstruct = NO;
				fmtp = mkaddr(np);
				goto endfmt;
			}
			if( ISINT(np->vtype) ){ 	/* ASSIGNed label */ 
				statstruct = NO;
				varfmt = NO;
				fmtp = (Addrp) fixtype(cpexpr(p));
				goto endfmt;
			}
		}
		p = V(IOSFMT) = fixtype(p);
		if(p->headblock.vtype == TYCHAR) {
			if(p->tag == TCONST) {
				p = V(IOSFMT) = (expptr) put_fmtstr(newlabel(),p);
				varfmt = NO;
			} else {
				statstruct = NO;
			}
			fmtp = (Addrp) cpexpr(p);
		} else if( ISICON(p) ) {
			if( (k = fmtstmt( mklabel(p->constblock.const.ci) )) > 0 ) {
				fmtp = (Addrp) mkaddcon(k);
				varfmt = NO;
			} else {
				ioformatted = UNFORMATTED;
			}
		} else	{
			err("bad format descriptor");
			ioformatted = UNFORMATTED;
			ok = NO;
		}
	} else {
		fmtp = NULL;
	}
	
endfmt:
	if(intfile && ioformatted==UNFORMATTED)
	{
		err("unformatted internal I/O not allowed");
		ok = NO;
	}
	if(!sequential && ioformatted==LISTDIRECTED)
	{
		err("direct list-directed I/O not allowed");
		ok = NO;
	}
	if(!sequential && ioformatted==NAMEDIRECTED)
	{
		err("direct namelist I/O not allowed");
		ok = NO;
	}

	if( ! ok )
		return;

	/*
	   Now put out the I/O structure, statically if all the clauses
	   are constants, dynamically otherwise
	*/

	if(statstruct) {
		tioblkp = ioblkp;
		ioblkp = ALLOC(Addrblock);
		ioblkp->tag = TADDR;
		ioblkp->vtype = TYIOINT;
		ioblkp->vclass = CLVAR;
		ioblkp->vstg = STGINIT;
		ioblkp->memno = newlabel();
		ioblkp->memoffset = ICON(0);
		ioblkp->isaggregate = ISSTRUCT;
		blklen = (intfile ? XIREC+SZIOINT :
							(sequential ? XFMT+SZADDR : XRNUM+SZIOINT) );
		t = ALLOC(IoBlock);
		t->blkno = ioblkp->memno;
		t->len = blklen;
		t->next = iodata;
		iodata = t;

		ioblkp->varsize = blklen;
		ioblkp->segdispl = 0;
	} else if(ioblkp == NULL) {
		ioblkp = autovar( (MAXIO+SZIOINT-1)/SZIOINT , TYIOINT, PNULL);
		ioblkp->isaggregate = ISSTRUCT;
	}

	ioset(TYIOINT, XERR, ICON(errbit));
	if(iostmt == IOREAD)
		ioset(TYIOINT, (intfile ? XIEND : XEND), ICON(endbit) );

	if(intfile)
	{
		ioset(TYIOINT, XIRNUM, nump);
		ioset(TYIOINT, XIRLEN, cpexpr(unitp->vleng) );
		ioseta(XIUNIT, unitp);
	}
	else
		ioset(TYIOINT, XUNIT, (expptr) unitp);

	if(recp)
		ioset(TYIOINT, (intfile ? XIREC : XREC) , (expptr) recp);

	if( varfmt || (fmtp && fmtp->vstg == STGINIT) ) {
		ioseta( intfile ? XIFMT : XFMT , fmtp);
	} else {
		ioset(TYADDR, intfile ? XIFMT : XFMT, fmtp);
	}

	ioroutine[0] = 's';
	ioroutine[1] = '_';
	ioroutine[2] = (iostmt==IOREAD ? 'r' : 'w');
	ioroutine[3] = (sequential ? 's' : 'd');
	ioroutine[4] = "ufln" [ord(ioformatted)];
	ioroutine[5] = (intfile ? 'i' : 'e');
	ioroutine[6] = '\0';
/*
  constant format strings have been converted to STGINIT addrblocks referencing
  a table of struct syls by put_fmtstr. Here we call the entry that
  knows about compiled format specifications and then restore the ioroutine
  name
*/

	if(ioroutine[4] == 'f' && ! varfmt) ioroutine[4] = 'F';
	putiocall( call1(TYLONG, ioroutine, cpexpr(ioblkp) ));
	if(ioroutine[4] == 'F') ioroutine[4] = 'f';

	if(statstruct)
	{
		frexpr(ioblkp);
		ioblkp = tioblkp;
		statstruct = NO;
	}
}



LOCAL dofopen()
{
	register expptr p;

	if( (p = V(IOSUNIT)) && ISINT(p->headblock.vtype) )
		ioset(TYIOINT, XUNIT, cpexpr(p) );
	else
		err("bad unit in open");
	if( (p = V(IOSFILE)) )
		if(p->headblock.vtype == TYCHAR)
			ioset(TYIOINT, XFNAMELEN, cpexpr(p->headblock.vleng) );
		else
			err("bad file in open");

	iosetc(XFNAME, p);

	if(p = V(IOSRECL))
		if( ISINT(p->headblock.vtype) )
			ioset(TYIOINT, XRECLEN, cpexpr(p) );
		else
			err("bad recl");
	else
		ioset(TYIOINT, XRECLEN, ICON(0) );

	iosetc(XSTATUS, V(IOSSTATUS));
	iosetc(XACCESS, V(IOSACCESS));
	iosetc(XFORMATTED, V(IOSFORM));
	iosetc(XBLANK, V(IOSBLANK));
	iosetlc(IOSFILEOPT, XFILEOPT, XFILEOPTLEN);

	putiocall( call1(TYLONG, "f_open", cpexpr(ioblkp) ));
}


LOCAL dofclose()
{
	register expptr p;

	if( (p = V(IOSUNIT)) && ISINT(p->headblock.vtype) )
	{
		ioset(TYIOINT, XUNIT, cpexpr(p) );
		iosetc(XCLSTATUS, V(IOSSTATUS));
		putiocall( call1(TYLONG, "f_clos", cpexpr(ioblkp)) );
	}
	else
		err("bad unit in close statement");
}


LOCAL dofinquire()
{
	register expptr p;
	if(p = V(IOSUNIT))
	{
		if( V(IOSFILE) )
			err("inquire by unit or by file, not both");
		ioset(TYIOINT, XUNIT, cpexpr(p) );
	}
	else if( ! V(IOSFILE) )
		err("must inquire by unit or by file");
	iosetlc(IOSFILE, XFILE, XFILELEN);
	iosetip(IOSEXISTS, XEXISTS);
	iosetip(IOSOPENED, XOPEN);
	iosetip(IOSNUMBER, XNUMBER);
	iosetip(IOSNAMED, XNAMED);
	iosetlc(IOSNAME, XNAME, XNAMELEN);
	iosetlc(IOSACCESS, XQACCESS, XQACCLEN);
	iosetlc(IOSSEQUENTIAL, XSEQ, XSEQLEN);
	iosetlc(IOSDIRECT, XDIRECT, XDIRLEN);
	iosetlc(IOSFORM, XFORM, XFORMLEN);
	iosetlc(IOSFORMATTED, XFMTED, XFMTEDLEN);
	iosetlc(IOSUNFORMATTED, XUNFMT, XUNFMTLEN);
	iosetip(IOSRECL, XQRECL);
	iosetip(IOSNEXTREC, XNEXTREC);
	iosetlc(IOSBLANK, XQBLANK, XQBLANKLEN);
	iosetlc(IOSFILEOPT, XQFILEOPT, XQFILEOPTLEN);

	putiocall( call1(TYLONG,  "f_inqu", cpexpr(ioblkp) ));
}



LOCAL dofmove(subname)
char *subname;
{
	register expptr p;

	if( (p = V(IOSUNIT)) && ISINT(p->headblock.vtype) )
	{
		ioset(TYIOINT, XUNIT, cpexpr(p) );
		putiocall( call1(TYLONG, subname, cpexpr(ioblkp) ));
	}
	else
		err("bad unit in I/O motion statement");
}



LOCAL
ioset(type, offset, p)
Vtype type;
int offset;
register expptr p;
{
	static char *badoffset = "badoffset in ioset";

	register Addrp q;
	register offsetlist *op;

	q = (Addrp) cpexpr(ioblkp);
	q->vtype = type;
	q->memoffset = fixtype( mkexpr(OPPLUS, q->memoffset, ICON(offset)) );

	if (statstruct && ISCONST(p)) {
		if (!ISICON(q->memoffset))
			fatal(badoffset);

		op = mkiodata(q->memno, q->memoffset->constblock.const.ci, blklen);
		if (op->tag != NDUNKNOWN)
			fatal(badoffset);

		if (type == TYADDR) {
			op->tag = NDLABEL;
			op->val.label = p->constblock.const.ci;
		} else {
			op->tag = NDDATA;
			op->val.cp = (Constp) convconst(type, 0, p);
		}

		frexpr((tagptr) p);
		frexpr((tagptr) q);
	} else {
		put_eq (q,p);
	}

	return;
}


LOCAL 
iosetc(offset, p)
int offset;
register expptr p;
{
	if(p == NULL)
		ioset(TYADDR, offset, ICON(0) );
	else if(p->headblock.vtype == TYCHAR)
		ioset(TYADDR, offset, addrof(cpexpr(p )));
	else
		err("non-character control clause");
}



LOCAL 
ioseta(offset, p)
int offset;
register Addrp p;
{
	static char *badoffset = "bad offset in ioseta";
	Namep np;
	extern struct Chain *ioaddr_list;

	int blkno;
	register offsetlist *op;

	if(statstruct) {
		blkno = ioblkp->memno;
		op = mkiodata(blkno, offset, blklen);
		if (op->tag != NDUNKNOWN)
			fatal(badoffset);

		if (p == NULL) {
			op->tag = NDNULL;
		} else if (p->tag == TADDR) {
			op->tag = NDADDR;
			/*
			 * don't know static addresses yet; must delay
			 * until endproc time.  This code was stolen from
			 * set_ioaddr() (ir_map_addr.c).
			 */
			np = p->namep;
			if(!np && p->vstg == STGINIT && p->vtype == TYCHAR) {
				/* address of a constant */
			} else if(!np || !ISCONST(p->memoffset)) {
				fatal("set_ioaddr: compile time address of non static");
			} else {
				/*
				 * We must call map_addr() to generate a
				 * variable reference leaf; but we don't
				 * want to constant-fold the expression into
				 * an ADDR_CONST (the offset part isn't known
				 * yet) So we create a new Addrblock and pass it
				 * to map_addr.
				 *
				 *      1e+308 on the vomit meter.
				 */
				NODE *nodep;
				Addrp temp;
				temp = mkaddr(np);
				nodep = (NODE*)map_addr(temp);
				frexpr(temp);
			}
			op->val.addr.memno =  (int) cpexpr(p);
			ioaddr_list = hookup(ioaddr_list, mkchain(&(op->val.addr), CHNULL));
		} else {
			badtag("ioseta", p->tag);
		}
	} else {
		ioset(TYADDR, offset, p ? addrof(p) : ICON(0) );
	}
}





LOCAL iosetip(i, offset)
int i, offset;
{
	register expptr p;

	if(p = V(i))
		if(p->tag==TADDR &&
		    ONEOF(p->addrblock.vtype, M(TYLONG)|M(TYLOGICAL)) )
			ioset(TYADDR, offset, addrof(cpexpr(p)) );
		else
			errstr("impossible inquire parameter %s", ioc[i].iocname);
	else
		ioset(TYADDR, offset, ICON(0) );
}



LOCAL iosetlc(i, offp, offl)
int i, offp, offl;
{
	register expptr p;
	if( (p = V(i)) && p->headblock.vtype==TYCHAR)
		ioset(TYIOINT, offl, cpexpr(p->headblock.vleng) );
	iosetc(offp, p);
}


LOCAL offsetlist *
mkiodata(blkno, offset, len)
int blkno;
ftnint offset;
ftnint len;
{
	register offsetlist *p, *q;
	register ioblock *t;
	register int found;

	found = NO;
	t = iodata;

	while (found == NO && t != NULL)
	{
		if (t->blkno == blkno)
			found = YES;
		else
			t = t->next;
	}

	if (found == NO)
	{
		t = ALLOC(IoBlock);
		t->blkno = blkno;
		t->next = iodata;
		iodata = t;
	}

	if (len > t->len)
		t->len = len;

	p = t->olist;

	if (p == NULL)
	{
		p = ALLOC(OffsetList);
		p->next = NULL;
		p->offset = offset;
		t->olist = p;
		return (p);
	}

	for (;;)
	{
		if (p->offset == offset)
			return (p);
		else if (p->next != NULL &&
		    p->next->offset <= offset)
			p = p->next;
		else
		{
			q = ALLOC(OffsetList);
			q->next = p->next;
			p->next = q;
			q->offset = offset;
			return (q);
		}
	}
}


outiodata()
{
	register ioblock *p;
	register ioblock *t;

	if (iodata == NULL) return;

	p = iodata;

	while (p != NULL)
	{
		pralign(ALILONG);
		fprintf(initfile,"v.%d", p->blkno);
		putc(':', initfile); putc('\n', initfile);
		outolist(p->olist, p->len);

		t = p;
		p = t->next;
		ckfree((char *) t);
	}

	iodata = NULL;
	return;
}



LOCAL
outolist(op, len)
register offsetlist *op;
register int len;
{
	static char *overlap = "overlapping i/o fields in outolist";
	static char *toolong = "offset too large in outolist";

	register offsetlist *t;
	register ftnint clen;
	register Constp cp;
	register Vtype type;

	clen = 0;

	while (op != NULL)
	{
		if (clen > op->offset)
			fatal(overlap);

		if (clen < op->offset)
		{
			prspace(op->offset - clen);
			clen = op->offset;
		}

		switch (op->tag)
		{
		default:
			badtag("outolist", op->tag);

		case NDDATA:
			cp = op->val.cp;
			type = cp->vtype;
			if (type != TYIOINT)
				badtype("outolist", type);
			prconi(initfile, type, cp->const.ci);
			clen += tysize(type);
			frexpr((tagptr) cp);
			break;

		case NDLABEL:
			prcona(initfile, op->val.label);
			clen += tysize(TYADDR);
			break;

		case NDADDR:
			praddr(initfile, op->val.addr.stg, op->val.addr.memno,
			op->val.addr.offset);
			clen += tysize(TYADDR);
			break;

		case NDNULL:
			praddr(initfile, STGNULL, 0, (ftnint) 0);
			clen += tysize(TYADDR);
			break;
		}

		t = op;
		op = t->next;
		ckfree((char *) t);
	}

	if (clen > len)
		fatal(toolong);

	if (clen < len)
		prspace(len - clen);

	return;
}

LOCAL expptr 
addrof(p)
expptr p;
{
	if(p->tag == TADDR) {
		ioblkp_addr_list = (chainp) 
			hookup(ioblkp_addr_list,mkchain(cpexpr(p),CHNULL));
	}
	return( mkexpr(OPADDR, p, PNULL) );
}
