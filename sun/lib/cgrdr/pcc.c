#ifndef lint
static	char sccsid[] = "@(#)pcc.c 1.5 87/03/31 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include    "cg_ir.h"
#include    "pcc_defines.h"
#include    <stdio.h>
	/*	this file implements the iropt side of the iropt-> pcc interface
	**	pcc nodes are passed around as opaque pointers of type CNODE
	**	in general, functions with names that start with pcc_... map an 
	**	iropt construct to a pcc one - for simplicity this convention is 
	**	extended to functions that don't return values
	*/

/* representation of a null character in a FORTRAN string */
#define STRINGNULL (3)

/* size and alignment of a double complex "structure" */
#define Z_SZ 16
#define Z_AL 2

extern CNODE *pccfmt_icon(), *pccfmt_name(), *pccfmt_oreg(), *pccfmt_reg();
extern CNODE *pccfmt_binop(), *pccfmt_unop(), *pccfmt_indirgoto();
extern CNODE *pccfmt_st_op(), *pccfmt_tmp(), *pccfmt_addroftmp();

static CNODE *pcc_addrof(), *pcc_assign(), *pcc_binval(), *pcc_call();
static CNODE *pcc_zx_param(), *pcc_cx_binval(), *pcc_cx_unval();
static CNODE *pcc_cbranch(), *pcc_err();
static CNODE *pcc_fval(), *pcc_goto(), *pcc_indirgoto(), *pcc_label(); 
static CNODE *pcc_pass(), *pcc_repeat(), *pcc_stmt(), *pcc_switch();
static CNODE *pcc_unval();

static CNODE *pcc_leaf(), *pcc_force();

static struct map_op {
	int pcc_op;
	CNODE * ((*map_ftn)());
	char * implement_complex;
} op_map_tab[] = {
	/*LEAFOP*/ { BAD_PCCOP, pcc_err , "" },
	/*ENTRYDEF*/ { BAD_PCCOP, pcc_err, "" },
	/*EXITUSE*/ { BAD_PCCOP, pcc_err, "" },
	/*IMPLICITDEF*/ { BAD_PCCOP, pcc_err, "" },
	/*IMPLICITUSE*/ { BAD_PCCOP, pcc_err, "" },
	/*PLUS*/ {	PLUS_PCCOP, pcc_binval, "_c_add" },
	/*MINUS*/ {	MINUS_PCCOP, pcc_binval, "_c_minus" },
	/*MULT*/ {	STAR_PCCOP, pcc_binval, "_c_mult" },
	/*DIV*/ {	SLASH_PCCOP, pcc_binval, "_c_div" },
	/*REMAINDER*/ { MOD_PCCOP, pcc_binval, "" },
	/*AND*/ { BITAND_PCCOP, pcc_binval, "" },
	/*OR*/ { BITOR_PCCOP, pcc_binval, "" },
	/*XOR*/ { BITXOR_PCCOP, pcc_binval, "" },
	/*NOT*/ { NOT_PCCOP, pcc_unval, "" },
	/*LSHIFT*/ { LSHIFT_PCCOP, pcc_binval, "" },
	/*RSHIFT*/ { RSHIFT_PCCOP, pcc_binval, "" },
	/*SCALL*/ {	CALL_PCCOP, pcc_call, "" },
	/*FCALL*/ {	CALL_PCCOP, pcc_call, "" },
	/*EQ*/ { EQ_PCCOP, pcc_binval, "_c_eq" },
	/*NE*/ { NE_PCCOP, pcc_binval, "_c_ne" },
	/*LE*/ { LE_PCCOP, pcc_binval, "" },
	/*LT*/ { LT_PCCOP, pcc_binval, "" },
	/*GE*/ { GE_PCCOP, pcc_binval, "" },
	/*GT*/ { GT_PCCOP, pcc_binval, "" },
	/*CONV*/ { CONV_PCCOP, pcc_unval, "" },
	/*COMPL*/ { BITNOT_PCCOP, pcc_unval, "" },
	/*NEG*/ { NEG_PCCOP, pcc_unval, "_c_neg" },
	/*ADDROF*/ { BAD_PCCOP, pcc_addrof, "" },
	/*IFETCH*/ { INDIRECT_PCCOP, pcc_unval, "" },
	/*ISTORE*/ { ASSIGN_PCCOP, pcc_assign, "" },
	/*GOTO*/ {	GOTO_PCCOP, pcc_goto, "" },
	/*CBRANCH*/ { CBRANCH_PCCOP, pcc_cbranch, "" },
	/*SWITCH*/ { BAD_PCCOP, pcc_switch, "" },
	/*REPEAT*/ { BAD_PCCOP, pcc_repeat, "" },
	/*ASSIGN*/ { ASSIGN_PCCOP, pcc_assign, "" },
	/*PASS*/ {	PASS_PCCOP, pcc_pass, "" },
	/*STMT*/ {	STMT_PCCOP, pcc_stmt, "" },
	/*LABELDEF*/ {	LABEL_PCCOP, pcc_label, "" },
	/*INDIRGOTO*/ {	GOTO_PCCOP, pcc_indirgoto, "" },
	/*FVAL*/ {	FORCE_PCCOP, pcc_fval, "" },
	/*LABELREF*/ {	BAD_PCCOP, pcc_err, "" },
	/*PARAM*/ {	BAD_PCCOP, pcc_err, "" }
};
BOOLEAN is_c_special_op(), is_same_tree(), fix_addresses();
int source_lineno;

				/* used when stmtprofflag is on */
extern int stmtprofflag;
extern FILE *dotd_fp;
int bbnum = 0;
static int bblineno = 1;
static BOOLEAN labfound = FALSE;	   
static BOOLEAN hasincr = FALSE;
static BOOLEAN firstone = TRUE;

CNODE *
pcc_iropt_node(np)
register NODE *np;
{
CNODE * ((*map_ftn)());

    if(np->operand.tag == ISTRIPLE) {
		map_ftn = op_map_tab[(int) ((TRIPLE*)np)->op ].map_ftn;
		return map_ftn(np);
	} else {
		return pcc_leaf((LEAF*) np);
	}
}

/*
**  pcc doesn't support addrof: if the address is a compile time
**  constant generate an icon else (locations with register bases) 
** 	do explicit arithmetic
*/
static CNODE *
pcc_addrof(np)
NODE *np;
{
register LEAF *leafp;
TWORD tword;
char name[20];
ADDRESS *ap;
CNODE *p, *lp, *rp;
TRIPLE *tp;

	if(np->operand.tag == ISTRIPLE) {
		tp = (TRIPLE*) np;
		if(tp->left->operand.tag == ISTRIPLE) {
			quita("pcc_addrof: addrof expr op >%s<",op_descr[ORD(tp->op)].name);
		} else {
			leafp = (LEAF*) tp->left;
		}
	} else {
		leafp = (LEAF*) np;
	}
	tword = leafp->type.tword; 
	tword = INCREF( tword);
	tword = pcc_type(tword);
	switch(leafp->class) {

		case CONST_LEAF:
			if(ISFTN(leafp->type.tword)) {
				p = pccfmt_icon(0, leafp->val.const.c.cp, tword);
			} else {
				if(leafp->visited ==  FALSE) {
					endata_const(leafp);
				}
				sprintf(name,"L%dD%d",hdr.procno,leafp->leafno);
				p = pccfmt_icon(0, name, tword);
			}
			break;

		case VAR_LEAF:
			if(leafp->val.addr.seg->descr.content == REG_SEG ) {
				quit("pcc_addrof: address of variable in register segment");
			}
			ap = &leafp->val.addr;
			if(ap->seg->base == NOBASEREG) {
					if(ap->labelno != 0) {
						sprintf(name,"v.%d",ap->labelno);
						p = pccfmt_icon(0,name,tword);
					} else {
						sprintf(name,"%s",ap->seg->name);
						p = pccfmt_icon(ap->offset,name,tword);
					}
			} else {
				lp = pccfmt_reg(ap->seg->base,tword);
				rp = pccfmt_icon(ap->offset, "", pcc_type(INT));
				p = pccfmt_binop(PLUS_PCCOP, lp, rp, tword);
			}
			break;

		default:
			quit("pcc_addrof: address of address constant");
	}
	return p;
}

static CNODE *
pcc_assign(tp)
TRIPLE *tp;
{
CNODE *p, *lp, *rp;
TWORD ctype, tword;
TRIPLE *right;
int op;

	/* iropt does not care about the type of assign ops - f1 does */
	if( tp->op == ISTORE ) {
		tword = DECREF(tp->left->operand.type.tword);
		ctype = pcc_type(tword);
		lp = pcc_iropt_node(tp->left);
		lp = pccfmt_unop(INDIRECT_PCCOP, lp, ctype );
		if(tword == DCOMPLEX) {
			if(tp->right->operand.tag == ISLEAF) {
				rp = pcc_addrof(tp->right);
			} else if( tp->right->operand.tag == ISTRIPLE && 
				tp->right->triple.op == IFETCH) {
				rp = pcc_iropt_node(tp->right->triple.left);
			} else {
				rp = pcc_iropt_node(tp->right);
			}
			p = pccfmt_st_op(STASG_PCCOP, Z_SZ, Z_AL, lp, rp, STRTY | PCCPTR );
			p = pccfmt_unop(INDIRECT_PCCOP, p, STRTY );
		} else {
			rp = pcc_iropt_node(tp->right);
			p = pccfmt_binop(ASSIGN_PCCOP, lp, rp, ctype);
		}
	} else { /* op == ASSIGN */
		tword = tp->left->operand.type.tword;
		if(tword == DCOMPLEX) {
			lp = pcc_iropt_node(tp->left);
			if(tp->right->operand.tag == ISLEAF) {
				rp = pcc_addrof(tp->right);
			} else if( tp->right->operand.tag == ISTRIPLE && 
				tp->right->triple.op == IFETCH) {
				rp = pcc_iropt_node(tp->right->triple.left);
			} else {
				rp = pcc_iropt_node(tp->right);
			}
			p = pccfmt_st_op(STASG_PCCOP, Z_SZ, Z_AL, lp, rp, STRTY | PCCPTR );
			p = pccfmt_unop(INDIRECT_PCCOP, p, STRTY );
		} else {
			ctype = pcc_type(tword);
			lp = pcc_iropt_node(tp->left);
			right = (TRIPLE*) tp->right;
			if(right->tag == ISTRIPLE && is_c_special_op( right) && 
				is_same_tree(tp->op, tp->left, right))
			{ /* turn it into an ASG tree */
					op = ASG op_map_tab[(int) right->op].pcc_op;
					right = (TRIPLE*) right->right;
			} else {
				op = ASSIGN_PCCOP;
			}
			rp = pcc_iropt_node(right);
			p = pccfmt_binop(op, lp, rp, ctype);
		}
	}
	pccfmt_doexpr(source_lineno,p);
	return (CNODE*) NULL;
}

static CNODE *
pcc_binval(tp)
TRIPLE *tp;
{
TWORD tword, ltword, rtword;
CNODE *p, *lp, *rp;

	ltword = tp->left->operand.type.tword;
	rtword = tp->right->operand.type.tword;

	if( (BTYPE(ltword) == ltword && B_ISCOMPLEX(ltword)) ||
		(BTYPE(rtword) == rtword && B_ISCOMPLEX(rtword))  )
	{
		if(ltword == rtword) {
			return pcc_cx_binval(tp);
		} else { /* operators of a complex op must be of same type */
			quit("pcc_binval: missing CONV in complex op");
		}
	}

	tword = tp->type.tword;
	if(tp->op == MULT) { /* try to turn it into a shift */
		if( BTYPE(tword) == tword &&  B_ISINT(tword) )
		{
			if( tp->left->operand.tag == ISLEAF &&
				( (LEAF *)tp->left)->class == CONST_LEAF )
			{ NODE *node;
				node = tp->left;
				tp->left = tp->right;
				tp->right = node;
			}
			if( tp->right->operand.tag == ISLEAF &&
				( (LEAF *)tp->right)->class == CONST_LEAF )
			{ int i;
				i = log2( ((LEAF *)tp->right)->val.const.c.i );
				if( i < 0 ) { /* not power of 2 */
					goto contin;
				}
				if( i == 0 ) /* multiply by 1 */
				{
					p = pcc_iropt_node( tp->left );
					return p;
				}
				/* else */
				tp->op = LSHIFT;
				tp->right = (NODE*) ileaf(i);
			}
		}
	}
contin:

	lp = pcc_iropt_node(tp->left);
	rp = pcc_iropt_node(tp->right);
	p =pccfmt_binop(op_map_tab[(int)tp->op].pcc_op, lp, rp, pcc_type(tword));
	return p;
}

/*
 * pass a double complex parameter by reference
 */
static CNODE *
pcc_zx_param(np)
	NODE *np;
{
	if(np->operand.tag == ISLEAF) {
		return pcc_addrof(np);
	}
	if( np->operand.tag == ISTRIPLE && np->triple.op == IFETCH) {
		return pcc_iropt_node(np->triple.left);
	}
	/*
	 * a zcomplex-valued expression is always translated into
	 * (libcall(&tmp,...), &tmp), so we assume here that the
	 * result will always be an lvalue
	 */
	return pcc_iropt_node(np);
}

static CNODE *
pcc_cx_binval(tp)
TRIPLE *tp;
{
	CNODE *p, *lp, *rp, *tmp;
	char *fname;
	TYPE t;
	TWORD btype;

	t = tp->left->operand.type;
	fname = op_map_tab[(int) tp->op ].implement_complex;
	if( fname[0] == '\0' ) {
		quit("pcc_cx_binval: bad op > %s <", op_descr[ORD(tp->op)].name);
	}
	fname[0] = 'F';
	if(t.tword == DCOMPLEX) {
		/*
		 * if result is boolean, translate as
		 *	libcall(lhs,rhs)
		 * else result is zcomplex; translate as
		 *	(libcall(&tmp, lhs, rhs), &tmp);
		 */
		fname[1] = 'z';
		if (tp->type.tword != COMPLEX && tp->type.tword != DCOMPLEX) {
			/* libcall(lhs, rhs) */
			btype = pcc_type(tp->type.tword);
			lp = pcc_zx_param(tp->left);
			rp = pcc_zx_param(tp->right);
			rp = pccfmt_binop(LISTOP_PCCOP, lp, rp, INT);
			lp = pccfmt_icon(0, fname, (PCCFUN|btype) );
			p  = pccfmt_binop(CALL_PCCOP, lp, rp, btype );
		} else {
			/* (&tmp,lhs) */
			btype = pcc_type(t.tword);
			tmp = pccfmt_tmp(Z_SZ/sizeof(int), btype);
			lp = pccfmt_addroftmp(tmp);
			rp = pcc_zx_param(tp->left);
			lp = pccfmt_binop(LISTOP_PCCOP, lp, rp, INT);

			/* (&tmp,lhs,rhs) */
			rp = pcc_zx_param(tp->right);
			rp = pccfmt_binop(LISTOP_PCCOP, lp, rp, INT);

			/* libcall(&tmp,lhs,rhs) */
			lp = pccfmt_icon(0, fname, (PCCFUN|INT));
			lp  = pccfmt_binop(CALL_PCCOP, lp, rp, INT );

			/* (libcall(&tmp,lhs,rhs), &tmp) */
			rp = pccfmt_addroftmp(tmp);
			p = pccfmt_binop(COMOP_PCCOP, lp, rp, (PCCPTR|btype));
			tfree(tmp);
		}
	} else {
		/* libcall(lhs, rhs) */
		fname[1] = 'c';
		btype = pcc_type(tp->type.tword);
		lp = pcc_iropt_node(tp->left);
		rp = pcc_iropt_node(tp->right);
		rp = pccfmt_binop(LISTOP_PCCOP, lp, rp, INT);
		lp = pccfmt_icon(0, fname, (PCCFUN|btype) );
		p  = pccfmt_binop(CALL_PCCOP, lp, rp, btype );
	}
	return p;
}

static CNODE *
pcc_call(tp) 
TRIPLE *tp;
{
LEAF *func;
TRIPLE *args;
TWORD tyfunc, tylist;
TRIPLE *param;
NODE *indirect_through;
CNODE *lp, *rp, *p, *left, *pcc_intr_call();
BOOLEAN rewrite_lib_call();

	func= (LEAF*) tp->left;
	if(func->class == CONST_LEAF) {
		if(func->type.aux.func_descr == INTR_FUNC) {
			return pcc_intr_call(tp);
		}
		if(func->type.aux.func_descr == SUPPORT_FUNC) {
			/* try to replace the call by a simpler tree */
			if(rewrite_lib_call(tp, &p) == TRUE) {
				return p;
			}
		}
		tyfunc = tp->type.tword;
		tyfunc = INCREF(tyfunc);
		lp = pccfmt_icon(0, func->val.const.c.cp, pcc_type(tyfunc));
	} else {
		lp = pcc_iropt_node(func);
	}

	tylist = pcc_type(INT);
	args = (TRIPLE*) tp->right;
	left = (CNODE*) NULL;
	if(args) {
		TFOR(param,args->tnext) {
			rp = pcc_iropt_node(param->left);
			if(left) {
				left = pccfmt_binop(LISTOP_PCCOP, left, rp, tylist);
			} else {
				left = rp;
			}
		} 
	}

	if(args) {
		p = pccfmt_binop(CALL_PCCOP, lp, left, pcc_type(tp->type.tword));
	} else {
		p = pccfmt_unop(CALL0_PCCOP, lp, pcc_type(tp->type.tword));
	}
	if(tp->op == SCALL) {
		pccfmt_doexpr(source_lineno,p);
		p = (CNODE*) NULL;
	}
	return p;
}

static CNODE *
pcc_cbranch(tp)
TRIPLE *tp;
{
TRIPLE *false_lab, *true_lab;
CNODE *p, *lp, *rp;
TWORD tyint;

	tyint = pcc_type(INT);
	lp = pcc_iropt_node(tp->left);
	true_lab = (TRIPLE*) tp->right; /* guess and ... */
	false_lab = true_lab->tnext;
	if(false_lab->right->leaf.val.const.c.i) { /* reverse if necessary */
		true_lab = false_lab;
		false_lab = (TRIPLE*) tp->right;
	}
	rp = pccfmt_icon(false_lab->left->leaf.val.const.c.i, "", tyint );
	p = pccfmt_binop(CBRANCH_PCCOP, lp, rp, tyint);
	pccfmt_doexpr(source_lineno, p);
	/*
	** for cbranch's emit a goto to the true label since
	** this may not necessarily follow; thus an ir cbranch
	** maps to a pcc cbranch and a goto
	*/
	pccfmt_goto(true_lab->left->leaf.val.const.c.i);
	return (CNODE*) NULL;
}


static CNODE *
pcc_err(tp) 
TRIPLE *tp;
{
	quita("pcc_err: >%s<", op_descr[ORD(tp->op)].name);
}

static CNODE *
pcc_fval(tp)
TRIPLE *tp;
{
CNODE *p, *lp;

	lp = pcc_iropt_node(tp->left);
	p = pccfmt_unop(FORCE_PCCOP, lp, pcc_type(tp->left->operand.type.tword));
	pccfmt_doexpr(source_lineno,p);
	return (CNODE*) NULL;
}

static CNODE *
pcc_force(np)
NODE *np;
{
CNODE *p, *lp;

	lp = pcc_iropt_node(np);
	p = pccfmt_unop(FORCE_PCCOP, lp, pcc_type(np->operand.type.tword));
	pccfmt_doexpr(source_lineno,p);
	return (CNODE*) NULL;
}

static CNODE *
pcc_goto(tp)
TRIPLE *tp;
{
TRIPLE *labelref;
int labelno;
char buff[20];

	labelref = (TRIPLE*) tp->right;
	if(labelref->op != LABELREF) {
		quita("pcc_goto: >%s<", op_descr[ORD(labelref->op)].name);
	}
	labelno = labelref->left->leaf.val.const.c.i;
	if(labelno == -1) { /* an "exit" goto */
	 	sprintf(buff,"\tjra LE%d",hdr.procno);
		pccfmt_pass(buff);
	} else {
		pccfmt_goto(labelno);
	}
	return (CNODE*) NULL;
}

static CNODE *
pcc_indirgoto(tp)
TRIPLE *tp;
{
CNODE *p, *lp;

	lp = pcc_iropt_node(tp->left);
	p = pccfmt_indirgoto(lp, pcc_type(tp->left->operand.type.tword));
	pccfmt_doexpr(source_lineno,p);
	return (CNODE*) NULL;
	
}

static CNODE *
pcc_label(tp)  
TRIPLE *tp;
{
LEAF *leafp;
char buff[100];

	if((leafp = (LEAF*)tp->left) && leafp->val.const.c.i) {
		pccfmt_label(leafp->val.const.c.i,"");
	}
	
	if (stmtprofflag ) {
		if ( firstone == TRUE ) {
			firstone = FALSE;
			goto ret;
		}
		if (hasincr == TRUE) goto ret;
		hasincr = TRUE;
		pccfmt_pass("	movl	___bb,a0");
		sprintf(buff,"	addql	#1,a0@(%d)", 4*bbnum);
		pccfmt_pass(buff);
		bbnum++;
		}

ret:
	return (CNODE*) NULL;
}

static CNODE *
pcc_pass(tp)
TRIPLE *tp;
{
	pccfmt_pass(tp->left->leaf.val.const.c.cp);
	return (CNODE*) NULL;
}

static CNODE *
pcc_repeat(tp)
TRIPLE *tp;
{
TWORD ctype, ltype;
TRIPLE *loop_lab, *end_lab;
int regno;
ADDRESS *ap;
char buff[100];
CNODE *p;

	ltype = tp->left->leaf.type.tword;
	ctype = pcc_type(ltype);
	end_lab = (TRIPLE*) tp->right;
	loop_lab = end_lab->tnext;
	ap = &(tp->left->leaf.val.addr);

	if( ap->seg->descr.class == DREG_SEG ) { /* use a dbra */
		regno = ap->offset;
		sprintf(buff,"	dbra	d%d,L%d",regno,loop_lab->left->leaf.val.const.c.i);
		pccfmt_pass(buff);
		if(ltype == INT || ltype == LONG ) {
			/* test if there are any more iterations to do :
			** clear low word and subtract 1  - if result is not 
			** negative continue
			*/
			sprintf(buff,"	clrw	d%d",regno); 
				pccfmt_pass(buff);
			sprintf(buff,"	subql	#0x1,d%d",regno); 
				pccfmt_pass(buff);
			sprintf(buff,"	jpl	L%d",loop_lab->left->leaf.val.const.c.i);
				pccfmt_pass(buff);
		}

		pccfmt_goto(end_lab->left->leaf.val.const.c.i);
	} else { /* repeat count in a temp or "a" register: use decrement and test*/
		p = pccfmt_binop(MINUSEQ_PCCOP, 
				pcc_leaf(tp->left) , pcc_leaf(ileaf(1)), ctype);
		pccfmt_doexpr(source_lineno,p);
		

		if( ap->seg->descr.class == AREG_SEG ) { /* subtraction didn't set cc */
			sprintf(buff,"	cmpl	#0,a%d",ap->offset);
			pccfmt_pass(buff);
		}
		sprintf(buff,"	jpl	L%d",loop_lab->left->leaf.val.const.c.i);
		pccfmt_pass(buff);
		pccfmt_goto(end_lab->left->leaf.val.const.c.i);
	}
	return (CNODE *) (NULL);
}

static CNODE *
pcc_stmt(tp)
TRIPLE *tp;
{
}


static CNODE *
pcc_switch(tp) 
TRIPLE *tp;
{
int nlab=0;
int skip_label, labarray;
char line[132], tab_start[132];
register TRIPLE *tp2;

	pcc_force(tp->left);
	skip_label = tp->right->triple.left->leaf.val.const.c.i;
	TFOR(tp2,(TRIPLE*)tp->right) {
		nlab++;
	}
	/*
	**	always true that 0 < nlab < 2^16-1 else it wouldn't fit on 1 statement 
	**	if d0<0 or d0=> nlab the jcc branch is taken. For d0==0
	**	the first entry in the jump table is the skip label
	*/

	sprintf(line,"\tcmpl\t#%d,d0\n\tbcc\tL%d", nlab,skip_label);
	pccfmt_pass(line);
	/* ccom style idiom for switch table N.B. recognized by c2 */
	sprintf(line,"\taddw\td0,d0\n\tmovw\tpc@(6,d0:w),d0\n\tjmp\tpc@(2,d0:w)");
	pccfmt_pass(line);

	labarray = new_label();
	sprintf(line,"L%dI%d:",hdr.procno,labarray);
	pccfmt_pass(line);
	sprintf(tab_start,"L%dI%d",hdr.procno,labarray);
	TFOR(tp2,(TRIPLE*)tp->right) {
		sprintf(line,"\t.word L%d-%s",tp2->left->leaf.val.const.c.i,tab_start);
		pccfmt_pass(line);
	}
	return (CNODE*) NULL;
}

static CNODE *
pcc_unval(tp) 
TRIPLE *tp;
{
CNODE *lp, *p;
TWORD tword, ltword;

	tword = tp->type.tword;
	ltword = tp->left->operand.type.tword;

	if(  /* an operation on a complex value */
		(BTYPE(ltword) == ltword && B_ISCOMPLEX(ltword))  ||
		/* or a conversion which yields a complex */
		( tp->op == CONV && (BTYPE(tword) == tword && B_ISCOMPLEX(tword))))
	{
		return pcc_cx_unval(tp);
	}

	lp = pcc_iropt_node(tp->left);
	p = pccfmt_unop(op_map_tab[(int)tp->op].pcc_op, 
					lp, pcc_type(tp->type.tword));
	return p;
}

static CNODE *
pcc_leaf(l)
LEAF *l;
{
char buff[132];
int pcctword, regno;
register ADDRESS * ap;
CNODE *p, *lp, *rp;

	pcctword = pcc_type(l->type.tword);
	ap= &l->val.addr;
	
	switch(l->class) {
		case VAR_LEAF:
			if(ap->seg->base == NOBASEREG) {
				if(ap->seg->descr.content == REG_SEG) {
					if( ap->seg == &seg_tab[DREG_SEGNO] ) {
						regno = ap->offset;
					} else {
						if( ap->seg == &seg_tab[AREG_SEGNO] ) {
							regno = AREGOFFSET + ap->offset;
						} else {
							regno = FREGOFFSET + ap->offset;
						}
					}
					p = pccfmt_reg(regno,pcctword);
				} else if(ap->labelno != 0) {
					sprintf(buff,"v.%d+%d",ap->labelno, ap->offset);
					p = pccfmt_name(0, buff, pcctword);
				} else {
					p = pccfmt_name(ap->offset, ap->seg->name, pcctword);
				}
			} else {
				p = pccfmt_oreg(ap->seg->base, ap->offset, "", pcctword);
			}
			break;
	
		case CONST_LEAF:
			if(B_ISINT(l->type.tword)) {
				p = pccfmt_icon(l->val.const.c.i,"",pcctword);
			} else if(l->type.tword == LABELNO) {
				sprintf(buff,"L%d",l->val.const.c.i);
				p = pccfmt_icon(0,buff,PCCINT);
			} else {
				if(l->visited ==  FALSE) {
					endata_const(l);
				}
				sprintf(buff,"L%dD%d",hdr.procno,l->leafno);
				p = pccfmt_name(0, buff, pcctword);
			}
			break;
	
		case ADDR_CONST_LEAF:
			if(ap->seg->base == NOBASEREG) {
				if(ap->labelno != 0) {
					sprintf(buff,"v.%d",ap->labelno);
					p = pccfmt_icon(ap->offset, buff, pcctword);
				} else {
					sprintf(buff,"%s",ap->seg->name);
					p = pccfmt_icon(ap->offset, buff, pcctword);
				}
			} else {
				lp = pccfmt_reg(ap->seg->base,pcctword);
				rp = pccfmt_icon(ap->offset, "", INT);
				p = pccfmt_binop(PLUS_PCCOP, lp, rp, pcctword);
			}
			break;

		default:
			quita("bad leaf >%d< in pcc_leaf",(int) l->class);
	}
	return p;
}

#define PCC_BTMASK 017

LOCAL
pcc_type(tword)
register TWORD tword;
{
	
	if(ISARY(tword)) {
		tword = DECREF(tword);
		tword = INCREF(tword);
	}
	if( ! ISPCCTYPE(tword) ) switch (tword&BTMASK) {
	
		case DCOMPLEX:
			tword= (tword&~BTMASK) | STRTY;
			break;

		case COMPLEX:
			tword= (tword&~BTMASK) | DOUBLE;
			break;
	
		case EXTENDEDF:
			tword = (tword&~BTMASK) | DOUBLE;
			break;
	
		case LABELNO:
		case BOOL:
			tword= (tword&~BTMASK) | INT;
			break;
	
		case STRING:
			tword= (tword&~BTMASK) | CHAR;
			break;
	
		case VOID:
			tword= (tword&~BTMASK);
			break;
	
		default:
			quita("pcc_type: unknown ir type %d",tword & BTMASK);
			break;
	}
	return((( tword >>TSHIFT)&~PCC_BTMASK)| ( tword &PCC_BTMASK));
}

map_to_pcc()
{
register BLOCK *bp;
register LIST *lp;
char name[100];
char *buff;
register TRIPLE *tp;
int frame_size;


	/* proc marker for c2's benefit */
	if( hdr.proc_type == 0 ) {/* VOID */
		pccfmt_pass("\t.text\n|#PROC# 00");
	} else {
		pccfmt_pass("\t.text\n|#PROC# 07");
	}
	
	if(*hdr.procname != '\0') {
		pccfmt_procname(hdr.procname);
	}

	fix_addresses();
	for(bp=entry_block ; bp ; bp = bp->next) {
		if(bp->labelno <= 0) { /*unreachable code */
			continue;
		}
		if(bp->entryname && *(bp->entryname) != '\0') {
			/* local hack to make the c2 happy */
			sprintf(name,"\t.globl\t_%s\n",bp->entryname);
			pccfmt_pass(name);

			sprintf(name,"_%s",bp->entryname);
			pccfmt_label(0,name);
		}
		if(bp->is_ext_entry) { /* put out an lbrac */

			firstone = TRUE;

			frame_size = seg_tab[AUTO_SEGNO].len*32;
			pccfmt_lbrac(hdr.procno, hdr.regmask, frame_size);
		}


        if (stmtprofflag) {
			if (labfound == FALSE && hasincr == TRUE) 
				fprintf (dotd_fp, "-1   0\n");

			labfound = FALSE;
			hasincr = FALSE;
		}
		if(bp->last_triple) TFOR(tp,bp->last_triple->tnext) {

			switch(tp->op) {
				case PASS:
					pccfmt_pass(tp->left->leaf.val.const.c.cp);
					break;

				case STMT:
					source_lineno = tp->left->leaf.val.const.c.i;
					if (stmtprofflag) 
						if (labfound==FALSE && hasincr == TRUE) {
							labfound = TRUE;
							bblineno = source_lineno;
							fprintf (dotd_fp, "%d 0\n", bblineno);
						}
					break;


				default:
					if( ISOP(tp->op,ROOT_OP) ) {
						source_lineno = tp->tripleno; 
						(op_map_tab[(int) tp->op ].map_ftn)(tp);
					}
					break;
			}
		}
	}
	pccfmt_rbrac();
}


/*
**	add a constant to data segment so it's possible to refer to it's
**  address by label
*/
LOCAL
endata_const(leafp)
LEAF *leafp;
{
char line[255];
register char *cp1,*cp2;
struct constant *const;
register int stl,cstl;

	sprintf(line,"	.data1\n.align 2\nL%dD%d:",hdr.procno,leafp->leafno);
	pccfmt_pass(line);
	const = &leafp->val.const;
	switch(BTYPE(leafp->type.tword)) {

		case STRING:
			strcpy(line,"\t.ascii\t\"");
			stl = strlen(const->c.cp);
			cp1=const->c.cp;
			cp2 = &line[9];
			while(stl != 0){
				for(; (stl > 0) && (cp2 <= &line[248]); cp1++) {
					if(*cp1 == '"') {
						*cp2++ = '\\';
						*cp2++ = *cp1;
					} else if(*cp1 == '\n') {
						*cp2++ = '\\';
						*cp2++ = '0';
						*cp2++ = '1';
						*cp2++ = '2';
					} else if(*cp1 == STRINGNULL) {
						*cp2++ = '\\';
						*cp2++ = '0';
						*cp2++ = '0';
						*cp2++ = '0';
					} else if(*cp1 == '\\') {
						*cp2++ = '\\';
						*cp2++ = '\\';
					} else {
						*cp2++ = *cp1;
					}
					stl--;
				}
				if(stl){
					*cp2++ = '"'; *cp2 = '\0';
					pccfmt_pass(line);
					cp2 = &line[9];
				}
			}
			*cp2++ = '\\'; *cp2++ = '0'; *cp2++ = '"'; *cp2 = '\0';
			break;

		case INT:
			sprintf(line,"\t.long 0x%X",const->c.i);
			break;

		case BOOL:
			if( leafp->type.aux.size == 4 ) {
				sprintf(line,"\t.long 0x%X",const->c.i);
			} else {
				sprintf(line,"\t.word 0x%X",const->c.i);
			}
			break;
			
		case SHORT:
			sprintf(line,"\t.word 0x%X",const->c.i);
			break;
			
		case FLOAT:	/* use bit represenatation */
			if(const->isbinary == TRUE) {
				sprintf(line,"\t.long 0x%X", l_align(const->c.bytep[0]));
			} else {
				sprintf(line,"\t.single 0R%s",const->c.fp[0]);
			}
			break;

		case DOUBLE:
			if(const->isbinary == TRUE) {
				sprintf(line,"\t.long 0x%X,0x%X",
					l_align(const->c.bytep[0]),
					l_align(const->c.bytep[0]+4));
			} else {
				sprintf(line,"\t.double 0R%s",const->c.fp[0]);
			}
			break;

		case COMPLEX:	
			if(const->isbinary == TRUE) {
				sprintf(line,"\t.long 0x%X,0x%X", 
					l_align(const->c.bytep[0]), l_align(const->c.bytep[1]));
			} else {
				sprintf(line,"\t.single 0R%s\n\t.single 0R%s",
					const->c.fp[0], const->c.fp[1]);
			}
			break;

		case DCOMPLEX:	
			if(const->isbinary == TRUE) {
				sprintf(line,"\t.long 0x%X,0x%X\n\t.long 0x%X,0x%X",
					l_align(const->c.bytep[0]), l_align(const->c.bytep[0]+4),
					l_align(const->c.bytep[1]), l_align(const->c.bytep[1]+4));
			} else {
				sprintf(line,"\t.double 0R%s\n\t.double 0R%s",
					const->c.fp[0], const->c.fp[1]);
			}
			break;

		default:
			quita("endata_const: unknown type %X",BTYPE(leafp->type.tword) );
	}
	pccfmt_pass(line);
	pccfmt_pass("\t.text");
	leafp->visited = TRUE;
}

LOCAL BOOLEAN
is_c_special_op( p ) TRIPLE *p;
{ 
TWORD tword;
   /* return yes, if the p->op is one of the C language binary ops that */
  /* can turn into the ASSIGN and op, such as +, -, | etc. */

	if( p->op == PLUS || p->op == MINUS || p->op == MULT ||
			p->op == REMAINDER || p->op == DIV || 
			p->op == AND || p->op == OR || p->op == XOR || 
			p->op == LSHIFT || p->op == RSHIFT ) {

		tword = p->type.tword;
		if( BTYPE(tword) == tword && B_ISCOMPLEX(tword) ) {
			return FALSE;
		} else {
			return( TRUE );
		}
	}
	
	
	/* else */
	return( FALSE );
}

/* return log base 2 of n if n a power of 2; otherwise -1 */
LOCAL 
log2(n) int n;
{
	int k;

	/* trick based on binary representation */

	if(n<=0 || (n & (n-1))!=0)
		return(-1);

	for(k = 0 ;  n >>= 1  ; ++k) {}

	return(k);
}

LOCAL BOOLEAN
is_same_tree( op, left, right )
register IR_OP op;
register NODE *left;
register TRIPLE *right;
{
	register NODE *operand1, *operand2;
	
	operand1 = right->left;
	operand2 = right->right;
	
	if( op == ASSIGN && left->operand.tag == ISLEAF )
	{
		if( left == operand1 )
		{
			return TRUE;
		}
		else
		{
			if( ISOP( right->op, COMMUTE_OP ) && left == operand2 )
			{
				right->right = operand1;
				right->left = operand2;
				return TRUE;
			}
		}
		return FALSE;
	}

	if( op == ISTORE )
	{
		if( operand1->operand.tag == ISTRIPLE &&
				((TRIPLE *)operand1)->op == IFETCH &&
						left == ((TRIPLE *)operand1)->left )
		{
			return TRUE;
		}
		else
		{
			if(ISOP(right->op, COMMUTE_OP) && operand2->operand.tag == ISTRIPLE &&
					((TRIPLE *)operand2)->op == IFETCH &&
						left == ((TRIPLE *)operand2)->left )
			{
				right->right = operand1;
				right->left = operand2;
				return TRUE;
			}
		}
	}
	return FALSE;
}

static BOOLEAN
fix_addresses()
{
register ADDRESS *ap;
register LEAF *leafp;
SEGMENT *segp;
char buff[100];
BOOLEAN argvec;
int argvec_loc;

	segp = &seg_tab[BSS_SMALL_SEGNO];
	if(segp->len > 0) {
		pccfmt_pass("\t.bss\n\t.align 4");
		sprintf(buff,"%s:\t.skip %d",segp->name, segp->len);
		pccfmt_pass(buff);
	}

	segp = &seg_tab[BSS_LARGE_SEGNO];
	if(segp->len > 0) {
		pccfmt_pass("\t.bss\n\t.align 4");
		sprintf(buff,"%s:\t.skip %d",segp->name, segp->len);
		pccfmt_pass(buff);
	}
	pccfmt_pass("\t.text");

	if(seg_tab[ARG_SEGNO].base == NOBASEREG) {
		argvec = FALSE;
	} else {
		argvec = TRUE;
		argvec_loc = seg_tab[ARG_SEGNO].offset; 
	}

	for(leafp=leaf_tab; leafp; leafp=leafp->next_leaf) {
		if( leafp->class == VAR_LEAF || leafp->class == ADDR_CONST_LEAF ) {
			ap = &leafp->val.addr;
			if(ap->seg->descr.class == ARG_SEG ) {
				if(argvec==TRUE) {
					ap->offset += argvec_loc;
				} else {
					ap->seg->base = STACK_REG;
					ap->offset += ARGFPOFFSET;
				}
			}
		}
	}
}

static CNODE *
pcc_cx_unval(tp)
TRIPLE *tp;
{
#define	TO_INDEX sizeof("F?_conv_?") -2
#define	FROM_INDEX sizeof("F?") -2
TYPE from_type, to_type;
char *fname;
LEAF *temp;
TWORD tword;
CNODE *p, *lp, *rp, *tmp;

	if(tp->op == CONV) {
		fname = "F?_conv_?";
		to_type = tp->type;
		from_type = tp->left->operand.type;

		switch(to_type.tword) {
			case SHORT:
			case LONG:
			case INT:
				fname[TO_INDEX] = 'i';
				break;
			case FLOAT:
				fname[TO_INDEX] = 'f';
				break;
			case DOUBLE:
				fname[TO_INDEX] = 'd';
				break;
			case COMPLEX:
				fname[TO_INDEX] = 'c';
				break;
			case DCOMPLEX:
				fname[TO_INDEX] = 'z';
				break;
			default :
				quit("pcc_cx_unval: complex conversion, bad to type");
				break;
		}
		switch(from_type.tword) {
			case SHORT:
			case LONG:
			case INT:
				fname[FROM_INDEX] = 'i';
				break;
			case FLOAT:
				fname[FROM_INDEX] = 'f';
				break;
			case DOUBLE:
				fname[FROM_INDEX] = 'd';
				break;
			case COMPLEX:
				fname[FROM_INDEX] = 'c';
				break;
			case DCOMPLEX:
				fname[FROM_INDEX] = 'z';
				break;
			default:
				quit("pcc_cx_unval: complex conversion, bad from type");
				break;
		}
		lp = pccfmt_icon(0, fname, pcc_type(to_type.tword)|PCCFUN);
		if(from_type.tword == DCOMPLEX) {
			rp = pcc_zx_param(tp->left);
		} else {
			rp = pcc_iropt_node(tp->left);
		}

		if(to_type.tword == DCOMPLEX) {
			tmp = pccfmt_tmp(Z_SZ/sizeof(int), pcc_type(DCOMPLEX));
			rp = pccfmt_binop(LISTOP_PCCOP, pccfmt_addroftmp(tmp), rp, INT);
			lp  = pccfmt_binop(CALL_PCCOP, lp, rp, INT );
			p = pccfmt_binop(COMOP_PCCOP, lp, 
					pccfmt_addroftmp(tmp), STRTY|PCCPTR);
			tfree(tmp);
		} else {
			p = pccfmt_binop(CALL_PCCOP, lp, rp, pcc_type(to_type.tword));
		}
	} else {
		tword = tp->left->operand.type.tword;
		fname = op_map_tab[(int) tp->op ].implement_complex;
		fname[0] = 'F';
		if( fname[0] == '\0' ) {
			quit("pcc_cx_unval: bad op > %s <", op_descr[ORD(tp->op)].name);
		}
		if(tword == DCOMPLEX) {
			fname[1] = 'z';
			lp = pccfmt_icon(0, fname, (INT|PCCFUN) );
			rp = pcc_zx_param(tp->left);

			tmp = pccfmt_tmp(Z_SZ/sizeof(int), pcc_type(DCOMPLEX));
			rp = pccfmt_binop(LISTOP_PCCOP, pccfmt_addroftmp(tmp), rp, INT);
			lp  = pccfmt_binop(CALL_PCCOP, lp, rp, INT );
			p = pccfmt_binop(COMOP_PCCOP, lp, 
						pccfmt_addroftmp(tmp), (PCCPTR|STRTY));
			tfree(tmp);
		} else {
			fname[1] = 'c';
			rp = pcc_iropt_node(tp->left);
			lp = pccfmt_icon(0, fname, (DOUBLE|PCCFUN));
			p  = pccfmt_binop(CALL_PCCOP, lp, rp, DOUBLE );
		}
	}
	return p;
}

static CNODE *
pcc_intr_call(tp)
TRIPLE *tp;
{
LEAF *func;
TRIPLE *param;
CNODE *lp, *p;
int pcc_op;

	param = (TRIPLE*) tp->right;
	lp = pcc_iropt_node(param->left);
	func = (LEAF*) tp->left;
	pcc_op = is_intrinsic(func->val.const.c.cp+1);
	if(pcc_op == -1) {
		quita("pcc_intr_call: >%s<", func->val.const.c.cp);
	}
	p = pccfmt_unop(pcc_op, lp, pcc_type(param->left->operand.type.tword));
	return p;
}

stmtprof_eof()
{
char buff[100];

	pccfmt_pass ("	.data");
	pccfmt_pass ("	.lcomm	___bb,4");
	pccfmt_pass ("	.lcomm	___bb_init,4");
	pccfmt_pass ("	.even");
	pccfmt_pass ("___count:");
	sprintf (buff, "	.long	%d", bbnum );
	pccfmt_pass (buff);
}
