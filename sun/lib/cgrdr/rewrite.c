#ifndef lint
static	char sccsid[] = "@(#)rewrite.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include    "cg_ir.h"
#include    "pcc_defines.h"
#include    <stdio.h>

#define ARG1 arg_v[0]
#define ARG2 arg_v[1]
#define ARG3 arg_v[2]
#define ARG4 arg_v[3]
extern int source_lineno;
LOCAL BOOLEAN rewrite_string_op();

BOOLEAN
rewrite_lib_call(tp, q)
TRIPLE *tp;
CNODE **q;
{
register LEAF *func;
register char *fname;


	func= (LEAF*) tp->left;
	fname = func->val.const.c.cp;
	*q = (CNODE*) NULL;

	if(strncmp(fname, "_s_copy",7) == 0) {
		return rewrite_string_op(tp, ASSIGN_PCCOP, q);
	}

	if(strncmp(fname, "_s_cmp",6) == 0) {
		return rewrite_string_op(tp, MINUS_PCCOP, q);
	}
	return FALSE;
}


LOCAL BOOLEAN
rewrite_string_op(tp,op, q)
TRIPLE *tp;
int op;
CNODE **q;
{
NODE *(arg_v[4]), **arg_vp = arg_v;
TRIPLE *args, *param;
CNODE *p, *rp, *lp, *pcc_iropt_node(), *pccfmt_unop(), *pccfmt_binop();

	args = (TRIPLE*) tp->right;
	TFOR(param,  args->tnext) {
		*arg_vp++ = param->left;
	}
	if(	ARG4->operand.tag == ISLEAF && 
		ARG3->operand.tag == ISLEAF && 
		((LEAF*)ARG4)->class == CONST_LEAF &&
		((LEAF*)ARG3)->class == CONST_LEAF &&
		((LEAF*)ARG4)->val.const.c.i == 1  &&
		((LEAF*)ARG3)->val.const.c.i == 1
	) {
		lp = pcc_iropt_node(ARG1);
		lp =  pccfmt_unop(INDIRECT_PCCOP, lp, CHAR );
		rp = pcc_iropt_node(ARG2);
		rp =  pccfmt_unop(INDIRECT_PCCOP, rp, CHAR );
		p = pccfmt_binop(op, lp, rp, CHAR);
		if(tp->op == SCALL) {
			pccfmt_doexpr(source_lineno,p);
		} else {
			*q = p;
		}
		return TRUE;
	} else {
		return FALSE;
	}
}
