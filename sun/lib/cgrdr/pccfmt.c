#ifndef lint
static	char sccsid[] = "@(#)pccfmt.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

# define	FORT
# include "cpass2.h"
	/* this file implements the pcc side of the iropt -> pcc interface */

NODE *
pccfmt_icon(offset, string, ctype)
int offset;
char *string;
int ctype;
{
NODE *p;

	p = talloc();
	p->in.op = ICON;
	p->in.type = ctype;
	p->tn.rval = 0;
	p->tn.lval = offset;
	if(string[0] != '\0') {
		p->in.name = tstr(string);
	} else {
		p->in.name = "";
	}
	p->in.rall = NOPREF;
	return p;
}

NODE *
pccfmt_name(offset,string,ctype)
int offset;
char *string;
int ctype;
{
NODE *p;

	p = talloc();
	p->in.op = NAME;
	p->in.type = ctype;
	p->tn.rval = 0;
	p->tn.lval = offset;
	p->in.name = tstr(string);
	p->in.rall = NOPREF; 
	return p; 
}

NODE *
pccfmt_oreg(regno,offset,string,ctype)
int regno, offset;
char *string;
int ctype;
{
NODE *p;

	p = talloc();
	p->in.op = OREG;
	p->in.type = ctype;
	p->tn.rval = regno;;
	p->tn.lval = offset;
	p->in.name = tstr(string);
	p->in.rall = NOPREF; 
	return p; 
}

NODE *
pccfmt_reg(regno,ctype)
int regno;
int ctype;
{
NODE *p;

	p = talloc();
	p->in.op = REG;
	p->in.type = ctype;
	p->tn.rval = regno;;
	rbusy( p->tn.rval, p->in.type );
	p->tn.lval = 0;
	p->in.name = tstr("");
	p->in.rall = NOPREF; 
	return p; 
}

pccfmt_doexpr(line_number,p)
int line_number;
NODE *p;
{
	lineno = line_number;
	if( lflag ) lineid( lineno, filename );
	if( edebug ) fwalk( p, eprint, 0 );
	nrecur = 0;
#ifdef MYREADER
	MYREADER(p);
#endif
	delay( p );
	reclaim( p, RNULL, 0 );
	allchk();
	tcheck();
	tmpoff = baseoff;
}

pccfmt_label(labelno,string)
int labelno;
char *string;
{
	if(*string != '\0') {
		fputs(string,stdout);
		putchar( ':' );
	} else {
		putchar('L');
		print_d(labelno);
		putchar(':');
	}
	putchar( '\n' );
}

pccfmt_goto(labelno)
int labelno;
{
	branch( labelno );
}

NODE *
pccfmt_indirgoto(lp,ctype)
NODE *lp;
int ctype;
{
NODE *p;

	p = talloc();
	p->in.op = GOTO;
	p->in.type = ctype;
	p->in.left = lp;
	p->tn.rval = 0;
	p->in.rall = NOPREF; 
}

NODE *
pccfmt_binop(opno, lp, rp, ctype)
int opno;
NODE *lp, *rp;
int ctype;
{
NODE *p;

	p = talloc();
	switch(opno) {
		case EQ:
		case NE:
		case LT:
		case LE:
		case GT:
		case GE:
		case ULT:
		case ULE:
		case UGT:
		case UGE:
			p->bn.op = opno;
			p->bn.type = ctype;
			p->bn.reversed = 0;
			p->in.left = lp;
			p->in.right = rp;
			p->in.rall = NOPREF; 
			break;

		default:
			p->in.op = opno;
			p->in.type = ctype;
			p->in.left = lp;
			p->in.right = rp;
			p->in.rall = NOPREF; 
			break;
	}
	return p; 
}

NODE *
pccfmt_unop(opno, lp, ctype)
int opno;
NODE *lp;
int ctype;
{
NODE *p;

	p = talloc();
	p->in.op = opno;
	p->in.type = ctype;
	p->in.left = lp;
	p->tn.rval = 0;
	p->in.rall = NOPREF; 
	return p; 
}

pccfmt_pass(string)
char *string;
{
	puts(string);
}

pccfmt_lbrac(procno, regmask, frame_size)
int procno, regmask, frame_size;
{
long newftnno;

	newftnno = procno;
	maxtreg = regmask;
	tmpoff = baseoff = frame_size;
	if (tmpoff%ALSTACK)
			cerror("intermediate file-stack misalignment");
	if( ftnno != newftnno ){
			/* beginning of function */
			maxoff = baseoff;
			ftnno = newftnno;
			maxtemp = 0;
	} else {
		if( baseoff > maxoff ) maxoff = baseoff;
		/* maxoff at end of ftn is max of autos and temps
		   over all blocks in the function */
   }
	saveregs();
	setregs();
}

pccfmt_rbrac()
{
	SETOFF( maxoff, ALSTACK );
	eobl2();
}

pccfmt_procname(string)
char *string;
{
	strcpy(filename, string);
}

pccfmt_eof()
{
	return( nerrors );
}

pccfmt_stop(op, size, align)
int op, size, align;
{
}

NODE *
pccfmt_st_op(opno, size, align, lp, rp, ctype)
int opno, size, align;
NODE *lp, *rp;
int ctype;
{
NODE *p;

	p = talloc();
	p->stn.stsize = size;
	p->stn.stalign = align;
	p->in.op = opno;
	p->in.type = ctype;

	switch(optype(opno)) {

		case BITYPE:
			p->in.left = lp;
			p->in.right = rp;
			p->in.rall = NOPREF; 
			break;

		case UTYPE:
			p->in.left = lp;
			p->tn.rval = 0;
			p->in.rall = NOPREF; 
			break;

		default:
			uerror( "illegal leaf node: %d", p->in.op );
			exit(1);
	}
	return p; 
}

NODE*
pccfmt_tmp(nwords, ctype)
int nwords, ctype;
{
NODE *p;

	p = talloc();
	p->in.op = OREG;
	p->in.type = ctype;
	p->tn.rval = TMPREG;
	p->tn.lval = freetemp(nwords);
	p->tn.lval = BITOOR(p->tn.lval);
	p->in.name = "";
	p->in.rall = NOPREF; 
	return p; 
}

NODE *
pccfmt_addroftmp(tmp)
NODE *tmp;
{
NODE *p, *lp, *rp;

	lp = pccfmt_reg(TMPREG, tmp->in.type | PTR);
	rp = pccfmt_icon(tmp->tn.lval, "", INT);
	p  = pccfmt_binop(PLUS, lp, rp, tmp->in.type | PTR);
	return p;
}
