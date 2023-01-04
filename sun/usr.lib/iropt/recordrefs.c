#ifndef lint
static	char sccsid[] = "@(#)recordrefs.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include <stdio.h>

extern LISTQ *tmp_lq;

int nvardefs, nexprdefs;
int nvarrefs, nexprrefs;
VAR_REF *var_ref_head, *new_var_ref();
EXPR_REF *new_expr_ref();
EXPR_KILLREF *new_expr_killref();
static BOOLEAN record_expr_refs;
/*
**	examine each triple and build the corresponding variable and/or expr 
**	reference entries distinguishing definitions, uses and killed exprs. 
**	other routines will later mark available definitions 
**	and exposed uses
*/

record_refs(do_exprs)
BOOLEAN do_exprs;
{
register LIST *tlp;			/* steps through a block's triples */
register TRIPLE *tp, *first;
register LIST *lp;
LIST *param_defs = LNULL;	/* list of rparams to be defined at end of an expr*/
VAR_REF *rp;
BLOCK *bp;
LIST *clp;
TRIPLE *argp, *triple;
LEAF *leafp;
	/* in expressions with imbedded definitions due to function calls 
	** (rparams and implicit defs) these are delayed until all uses are recorded
	*/
BOOLEAN	delay_defs;	
SITE site;

	record_expr_refs = do_exprs;
	free_ref_space();

	for(bp=entry_block; bp; bp=bp->next)	{
		if(bp->last_triple==TNULL) continue;
		first = bp->last_triple->tnext;
		site.bp=bp;
		TFOR(tp, first) {
			if(do_exprs == FALSE) {
				tp->expr = (EXPR*) NULL;
			}
			site.tp = tp;
			if(tp->op == SCALL || tp->op == FCALL) {
				record_use(site,tp->left,1);
				/*
				** the order of uses and defs around call sites is sensitive.
				** the strategy is to mark use of the function name, mark
				** a use of all arguments and then (1) if the call is a
				** subroutine call mark a def of all rparams (2) else
				** save a list of all such defines and put it out after
				** all uses in this rhs have been recorded to
				** ensure the defs do not reach uses in the same
				** rhs ( or kill subexpressions). There is an implicit
				** assumption that arguments to rparam are leaves or
				** indirects of address calculations - other expressions
				** should have been assigned to temporaries in pass1
				*/
				TFOR(argp,(TRIPLE*) tp->right) {
					site.tp = argp;
					record_use(site,argp->left,1);
				}

				if(tp->op == SCALL) { /* it's a subroutine*/
					delay_defs = FALSE;
				} else { /* it's (part of) an expression */
					delay_defs = TRUE;
					/* record a definition of the expression 
					*/
					if(record_expr_refs == TRUE) {
						site.tp = tp;
						record_expr_def(site,tp);
					}
				}
			} else if(tp->op == IMPLICITDEF) {
				if(delay_defs == TRUE) {
					/* 	an implicit def amidst an expression that contains
					**	a function call is treated as a reference parameter
					**	ie the def is a side effect that occurs after the
					**	whole expr has been recorded
					*/
					lp = NEWLIST(tmp_lq);
					(TRIPLE*) lp->datap = tp;
					LAPPEND(param_defs,lp);
				} else {
					site.tp = tp;
					record_var_def(site,tp->left);
				}
			} else {
				if( ISOP(tp->op, USE1_OP) && 
					( tp->left->operand.tag != ISLEAF ||
					  (!ISCONST(tp->left)) ) ){
						record_use(site,tp->left,1);
				}
				if( ISOP(tp->op, USE2_OP) && 
					(tp->right->operand.tag != ISLEAF ||
					  (!ISCONST(tp->right)) ) ){
						record_use(site,tp->right,2);
				}
				if( ISOP(tp->op, VALUE_OP) && record_expr_refs == TRUE ) {
						record_expr_def(site,tp);
				} else {
					if(ISOP(tp->op,ROOT_OP)) {
						/*
						** defs due to rparams are put out at the end 
						** of an expression after all uses are recorded
						** eg for i = b[i]+f(&i) the two uses of i and the
						** use of the + triple are recorded before the
						** definition 
						*/
						if(param_defs != LNULL) {
							LFOR(lp,param_defs) {
								argp = (TRIPLE*) lp->datap;
								site.tp = argp;
								record_var_def(site,argp->left);
							}
							param_defs = LNULL;
						}
						delay_defs = FALSE;
					}
					/*	the modifications due to an istore are described
					**	by implicit def triples and thus are not recorded here
					*/
					if(ISOP(tp->op, MOD_OP) && tp->op != ISTORE) {
						site.tp = tp;
						record_var_def(site,tp->left);
					}
				}
			}
		}
		if(param_defs != LNULL) {
			quit("record_refs: function value(s) not used");
		}
		empty_listq(tmp_lq);
	}
}

/*
** the item being defined is a single leaf - multiple definitions due to
** assignment through an indirect node or assignment to overlapped leaves
** have been expanded into separate implicit def triples
*/

record_var_def(site,item)
SITE site;
LEAF *item;
{
register LIST *lp,*lp2;
register VAR_REF *var_rp;
register EXPR_KILLREF *expr_rp;
EXPR *expr;

	if( ((NODE*) item)->operand.tag != ISLEAF) {
		/* FIXME debug */
		quit("record_var_def: bad definition");
	}
	var_rp = new_var_ref(FALSE);
	var_rp->reftype = VAR_DEF;
	var_rp->site = site;
		/* 
		** defno becomes the bit number of the definition if
		** build_var_sets determines the definition is available on exit 
		** from the block - here it's initialized to an illegal bit number
		*/
	var_rp->defno = -1; 
	var_rp->leafp = item;

		/*
		**  add the mod var reference to the leaf's references list
		**  for all expressions on the leaf's dependent exprs list
		**		if the expr is computed (ie not a leaf)
		**  	make a killed expr reference and
		**  	add this reference to the expr's references list
		*/

	nvardefs++;

	/* build the references IN the ORDER of OCCURS */
	if( item->references == (VAR_REF *)NULL )
		item->ref_tail = item->references = var_rp;
	else
	{
		item->ref_tail->next_vref = var_rp;
		item->ref_tail = var_rp;
	}

	if(site.tp->var_refs == (VAR_REF*) NULL) {
		site.tp->var_refs = var_rp;
	}

	if(record_expr_refs == TRUE) LFOR(lp,item->dependent_exprs) {
		expr = (EXPR*) lp->datap;
		if( expr->op != LEAFOP)  {
			expr_rp = new_expr_killref(FALSE);
			expr_rp->reftype = EXPR_KILL;
			expr_rp->blockno = site.bp->blockno;

			if( expr->ref_tail == (EXPR_REF *)NULL )
				expr->ref_tail = expr->references = (EXPR_REF*) expr_rp;
			else
			{
				expr->ref_tail->next_eref = (EXPR_REF*) expr_rp;
				expr->ref_tail = (EXPR_REF*) expr_rp;
			}

		}
	}

}

record_expr_def(site,item)
SITE site;
TRIPLE *item;
{
register EXPR_REF *expr_rp;
register EXPR *expr;

	expr_rp = new_expr_ref(FALSE);
	expr = item->expr;

	expr_rp->reftype = EXPR_GEN;
	expr_rp->site = site;
	expr_rp->defno = -1; 

	if( expr->ref_tail == (EXPR_REF *)NULL )
		expr->ref_tail = expr->references = expr_rp;
	else
	{
		expr->ref_tail->next_eref = expr_rp;
		expr->ref_tail = expr_rp;
	}

	nexprdefs ++;

}

record_use(site,item, use1or2)
SITE site;
NODE *item;
int use1or2;
{
register LEAF *leafp;
register VAR_REF *var_rp;
register EXPR_REF *expr_rp;
register EXPR *expr;

	if(item->operand.tag == ISLEAF) {
		leafp = (LEAF*) item;
		if(ISCONST(leafp)) return;
		var_rp =  new_var_ref(FALSE);
		var_rp->reftype = ( use1or2  == 1  ? VAR_USE1 : VAR_USE2 );
		var_rp->site = site;
		var_rp->leafp = leafp;
		if( leafp->references == (VAR_REF *)NULL )
			leafp->ref_tail = leafp->references = var_rp;
		else
		{
			leafp->ref_tail->next_vref = var_rp;
			leafp->ref_tail = var_rp;
		}

		if(site.tp->var_refs == (VAR_REF*) NULL) {
			site.tp->var_refs = var_rp;
		}
	} else if(item->operand.tag == ISTRIPLE) {
		if(record_expr_refs != TRUE) return;
		expr = item->triple.expr;
		expr_rp = new_expr_ref(FALSE);
		expr_rp->reftype = ( use1or2 == 1  ? EXPR_USE1 : EXPR_USE2 );
		expr_rp->site = site;

		if( expr->ref_tail == (EXPR_REF *)NULL )
			expr->ref_tail = expr->references = expr_rp;
		else
		{
			expr->ref_tail->next_eref = expr_rp;
			expr->ref_tail = expr_rp;
		}

	} else {
		quit("record_use: bad operand tag >%X<",item->operand.tag); 
	}
}

dump_var_refs()
{
register VAR_REF *rp, **refparr;
register LIST *lp;
register TRIPLE *tp, *triple;
BLOCK *bp;
static char *refname[] = {
	"VAR_DEF", "VAR_AVAIL_DEF","VAR_USE1", 
	"VAR_USE2", "VAR_EXP_USE1", "VAR_EXP_USE2" 
};

	refparr = (VAR_REF**) ckalloca((nvarrefs+1) * sizeof(char*));
	for(rp = var_ref_head; rp; rp = rp->next) {
		refparr[rp->refno] = rp;
	}

	refparr[nvarrefs] = (VAR_REF*) NULL;
	printf("\nVAR_REFS :\n");
	for(rp = *refparr++; rp ; rp = *refparr++) {
		printf("[%d]\t%s",rp->refno,refname[ORD(rp->reftype)]);
		if(!ISUSE(rp->reftype)) printf(" defno %d ",rp->defno);
		printf(" L[%d]",rp->leafp->leafno);
		printf(" at B[%d] T[%d]\n", rp->site.bp->blockno,rp->site.tp->tripleno);
	}
}

dump_expr_refs()
{
EXPR_REF *rp;
TRIPLE *tp, *triple;
EXPR *ep, **epp;
BLOCK *bp;
static char *refname[] = {
	"EXPR_GEN", "EXPR_AVAIL_GEN","EXPR_USE1", 
	"EXPR_USE2", "EXPR_EXP_USE1", "EXPR_EXP_USE2", "EXPR_KILL", "EXPR_AVAIL_KILL" 
};
struct entry { EXPR_REF * rp; int exprno; } *ordered_refs, *entryp;

	if(nexprrefs ==  0) return;
	printf("\nEXPR_REFS :\n");

	ordered_refs = (struct entry *)
						ckalloca((nexprrefs+1) * sizeof(struct entry));

	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			for(rp = ep->references; rp ; rp = rp->next_eref) {
# ifndef DEBUG
				/* refno field is not compiled unless debugging */
				if(rp->reftype == EXPR_KILL) continue;
# endif
				ordered_refs[rp->refno].rp = rp;
				ordered_refs[rp->refno].exprno = ep->exprno;
			}
		}
	}

	ordered_refs[nexprrefs].rp = (EXPR_REF*) NULL;
	for(entryp = ordered_refs; entryp->rp ; entryp++ ) {
		rp = entryp->rp;
		printf("[%d]\t%s of V[%d] at B[%d] T[%d]",
			rp->refno,refname[ORD(rp->reftype)], entryp->exprno,
			rp->site.bp->blockno,rp->site.tp->tripleno);
		if( rp->reftype == EXPR_AVAIL_GEN) printf(" defno %d",rp->defno);
		printf("\n");
	}
}


/*
**	go through all exprs
**	and eliminate kill references from the references list
**	then free the space used for the kill references
*/

free_expr_killref_space()
{
register EXPR *ep, **epp;
register EXPR_REF *rp0, *rp1;

	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			rp0 = ep->references;
			while(	rp0 && 
					(rp0->reftype == EXPR_KILL ||
					rp0->reftype == EXPR_AVAIL_KILL )  ){
				rp0 = rp0->next_eref;
			}
			ep->references = ep->ref_tail = rp0;

			while(rp0) {
				ep->ref_tail = rp0;
				rp1 = rp0->next_eref;
				while(rp1 &&
					(rp1->reftype == EXPR_KILL || 
					rp1->reftype == EXPR_AVAIL_KILL)){
					rp1 = rp1->next_eref;
				}
				rp0->next_eref = rp1;
				rp0 = rp1;
			}
		}
	}
	new_expr_killref(TRUE);
}

free_ref_space()
{
register LIST **lpp, *hash_link;
register LEAF *leafp;
register BLOCK *bp;
register TRIPLE *tp, *triple;
TRIPLE *last, *right;
LIST *head;

	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		head =  *lpp;
		if(head != LNULL) LFOR(hash_link, head) {
			leafp = (LEAF*) hash_link->datap;
			leafp->references = leafp->ref_tail =  (VAR_REF*) NULL;
		}
	}

	for(bp=entry_block; bp; bp=bp->next)	{
		last = bp->last_triple;
		if(last != TNULL) TFOR(tp,last) {
			if( ISOP(tp->op, NTUPLE_OP)) {
				right = (TRIPLE*) tp->right;
				if(right != TNULL) TFOR( triple, right ) {
					triple->reachdef1=triple->reachdef2=triple->canreach = NULL;
					triple->var_refs = (VAR_REF*) NULL;
				}
			}
			tp->reachdef1 = tp->reachdef2 = tp->canreach = NULL;
			tp->var_refs = (VAR_REF*) NULL;
		}
	}

	new_expr_killref(TRUE);
	new_expr_ref(TRUE);
	new_var_ref(TRUE);
	nvardefs = nexprdefs = nvarrefs = nexprrefs = 0;
}
