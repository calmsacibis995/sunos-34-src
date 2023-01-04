#ifndef lint
static	char sccsid[] = "@(#)make_expr.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include <stdio.h>

EXPR *find_expr(), *lookup_hash_expr(), *expr_head, *expr_tail;
BOOLEAN same_expr();
int nexprs;
LISTQ *dependentexpr_lq;

dump_exprs()
{
register LIST *lp;
register EXPR *ep, **exprparr, **epp;
EXPR_REF *erp;

	exprparr = (EXPR**) ckalloca((nexprs+1) * sizeof(char*));
	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr )
		{
			exprparr[ep->exprno] = ep;
		}
	}

	exprparr[nexprs] = (EXPR*) NULL;
	printf("\nEXPRS\n");
	for(ep = *exprparr++; ep ; ep = *exprparr++) {
		if(ep->op == LEAFOP) {
			printf("[%d] leaf %s L[%d] ",ep->exprno, 
				(((LEAF*)ep->left)->pass1_id ? 
						((LEAF*)ep->left)->pass1_id : ""),
				((LEAF*)ep->left)->leafno);
		}else {
			printf("[%d] %s E[%d] ",
				ep->exprno, op_descr[ORD(ep->op)].name,
					ep->left->exprno);
			if(ISOP(ep->op,BIN_OP) && ep->op != SCALL && ep->op != FCALL ){
				printf("E[%d] ", ep->right->exprno);
			}
			if(ep->depends_on) {
				printf("\tdepends_on: ");
				LFOR(lp,ep->depends_on->next){
					printf("L[%d] ", LCAST(lp,LEAF)->leafno);
				}   
			}
			if(ep->references) {
				printf("references: ");
				for(erp = ep->references; erp; erp = erp->next_eref) {
					printf("%d  ", erp->refno);
				}   
			}
		}
		printf("\n");
	}
}

free_exprs() 
{
register LIST **lpp, *hash_link;
register EXPR *ep, **epp;
register LEAF *leafp;

	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		*epp = (EXPR *) NULL;
	}
	new_expr(TRUE);
	nexprs = 0;
}

free_depexpr_lists()
{
register LIST **lpp, *hash_link;
register EXPR *ep, **epp;
LEAF *leafp;

	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			ep->depends_on =  LNULL;
		}
	}
	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(hash_link, *lpp) {
			leafp = (LEAF*) hash_link->datap;
			leafp->dependent_exprs = LNULL;
		}
	}
	empty_listq(dependentexpr_lq);
}

entable_exprs()
{
register LIST *lp;
register BLOCK *bp;
register TRIPLE *tp;

	free_exprs();
	if(dependentexpr_lq == NULL) dependentexpr_lq = new_listq();
	for(bp=entry_block;bp;bp=bp->next) {
		TFOR(tp,bp->last_triple) {
			if( ISOP(tp->op,VALUE_OP)) {
					tp->expr = find_expr(tp);
			} else {
				tp->expr = (EXPR*) NULL;
			}
		}
	}
}

EXPR *
find_expr(np)
NODE *np;
{
struct expr expr;
int hash;
register TRIPLE *tp;

	if(np == (NODE*) NULL) {
		return( (EXPR*) NULL);
	}
	expr.type = np->operand.type;
	switch(np->operand.tag) {
		case ISTRIPLE:
			tp = (TRIPLE*) np;
			if( ! ISOP(tp->op,VALUE_OP)) {
				quita("find_expr: non value triple >%X<",ORD(tp->op));
			}
			expr.op = tp->op;
			if( tp->op == SCALL || tp->op == FCALL ) {
				find_call_expr(&expr,tp);
			} else if(ISOP(tp->op,BIN_OP)) {
				expr.left = find_expr(tp->left);
				expr.right = find_expr(tp->right);
				expr.depends_on = 
					merge_lists(copy_list(expr.left->depends_on, dependentexpr_lq),
						copy_list(expr.right->depends_on,dependentexpr_lq));
			} else if(ISOP(tp->op,UN_OP)) {
				expr.left = find_expr(tp->left);
				expr.right = (EXPR*) NULL;
				if(tp->op == ADDROF) {
					expr.depends_on = LNULL;
				} else if(tp->op == IFETCH) {
					/*
					**	an indirection node depends on can_access
					*/
					expr.depends_on = merge_lists(copy_list(expr.left->depends_on,dependentexpr_lq),
						copy_list(tp->can_access,dependentexpr_lq));
				} else {
					expr.depends_on = copy_list(expr.left->depends_on,dependentexpr_lq);
				}
			}
			hash = (ORD(tp->op) << 16 ) ^ (int) expr.left  ^ (int) expr.right; 
			hash %=  EXPR_HASH_SIZE;
			break;
			
		case ISLEAF:
			expr.op = LEAFOP;
			expr.left = (EXPR*) np; /*leaf's address used as a unique expr */
			expr.right = (EXPR*) NULL;
			expr.depends_on = NEWLIST(dependentexpr_lq);
			(LEAF*) expr.depends_on->datap = (LEAF*) np;
			hash = ((int)np >> 4) % EXPR_HASH_SIZE;
			break;

		default:
			quita("find_expr: bad operand tag >%X<",ORD(np->operand.tag));

		}
	return(lookup_hash_expr(hash,&expr));
}	

find_call_expr(exprp,tp)
register EXPR * exprp;
register TRIPLE *tp;
{
static call_count = 0;

	/* 
	**	for now assume calls never yield the same expr
	**	should be something like if fun type not void and
	**  same fun and same arglist and args not defined and
	**  fun builtin and no side effects THEN same expr FIXME
	*/
	exprp->left = find_expr(tp->left);
	exprp->right = (EXPR*) ++call_count;
	exprp->depends_on = copy_list(exprp->left->depends_on,dependentexpr_lq);
}

EXPR *
lookup_hash_expr(index,exprp)
int index;
EXPR *exprp;
{
register LIST *lp1, *lp2;
register EXPR *ep;

		
		for( ep = expr_hash_tab[index]; ep; ep = ep->next_expr )
		{
			if(same_expr(exprp, ep))
			{
				exprp->depends_on = LNULL;
				return((EXPR*) ep);
			}
		}
		ep = (EXPR*) new_expr(FALSE);
		*ep = *exprp;
		ep->exprno = nexprs++;
		ep->references = ep->ref_tail = (EXPR_REF*) NULL;
		ep->save_temp = (LEAF *) NULL;
		ep->next_expr = expr_hash_tab[index];
		expr_hash_tab[index] = ep;
		/*
		**	now for all leaves on which this expr depends do:
		**  	add the expr to the leaf's dependent exprs list
		*/
		LFOR(lp1,exprp->depends_on) {
			lp2 = NEWLIST(dependentexpr_lq);
			(EXPR*) lp2->datap = ep;
			LAPPEND(LCAST(lp1,LEAF)->dependent_exprs,lp2);
		}
		return( ep );

}

BOOLEAN
same_expr(ep1,ep2)
EXPR *ep1, *ep2;
{
BOOLEAN same_irtype();

		if(ep1->op != ep2->op) return(FALSE);
		if(	ep1->op == LEAFOP || ISOP(ep1->op,UN_OP) ) { 
			if(ep1->left == ep2->left)  {
				if(ep1->op == CONV) {
					if(same_irtype(ep1->type,ep2->type) == TRUE){
						return TRUE;
					}
				} else {
					return TRUE;
				}
			}
		} else if(ISOP(ep1->op,BIN_OP)) {
			if(	ep1->left == ep2->left &&
				ep1->right == ep2->right) return(TRUE);
			if(ISOP(ep1->op,COMMUTE_OP)) {
				if(	ep1->left == ep2->right &&
					ep1->right == ep2->left) return(TRUE);
			}
		}
	return(FALSE);
}
