#ifndef lint
static	char sccsid[] = "@(#)implicit.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include <stdio.h>
#include <ctype.h>

TRIPLE *append_triple(), *add_implicit_list(), *note_side_eff();
LIST *order_list();
static LIST *externals, *statics, *heap_leaves, *arg_leaves;
extern LISTQ *proc_lq, *tmp_lq;
extern BOOLEAN partial_opt;
extern LIST *ext_entries;

insert_implicit()
{
register LIST *lp;
SEGMENT *sp, *seg_top;
register TRIPLE *tp;
LIST *real_entries;
BLOCK *bp;

	statics = externals = heap_leaves = arg_leaves = LNULL;
	seg_top = &seg_tab[nseg];
	for(sp=seg_tab;sp<seg_top; sp++) {
		if(sp->descr.external == EXTSTG_SEG && sp->descr.class != HEAP_SEG &&
					sp->leaves != LNULL) {
			if( partial_opt ) { /* do NOT trace the external variables. */
				continue;
			}
			lp = (LIST*) copy_list(sp->leaves,tmp_lq);
			LAPPEND(externals,lp);
		} else if((sp->descr.class == BSS_SEG || sp->descr.class == DATA_SEG) &&
							sp->leaves != LNULL) {
			lp = (LIST*) copy_list(sp->leaves,tmp_lq);
			LAPPEND(statics,lp);
		} else if( sp->descr.class == ARG_SEG && sp->leaves != LNULL) {
			/* no exit_use of arguments */
			lp = (LIST*) copy_list(sp->leaves, tmp_lq);
			LAPPEND(arg_leaves,lp);
		} else if(sp->descr.class == HEAP_SEG && sp->leaves != LNULL) {
			if( partial_opt ) {
				continue;
			}
			lp = (LIST*) copy_list(sp->leaves,tmp_lq);
			LAPPEND(heap_leaves,lp);
		}
	}

	/* look for real entry points and insert ENTRYDEFs at each one */
	real_entries = LNULL;
	LFOR(lp, ext_entries) {
		bp = (BLOCK*) lp->datap;
		if(bp->labelno > 0 ) {
			LIST *lp2;

			lp2 = NEWLIST(tmp_lq);
			(BLOCK*) lp2->datap = bp;
			LAPPEND(real_entries,lp2);
		}
	}

	LFOR(lp, real_entries) {
		bp = (BLOCK*) lp->datap;
		tp = bp->last_triple;
		if(ISOP(tp->op,BRANCH_OP)) {
			tp = tp->tprev;
		}
		tp = add_implicit_list(ENTRYDEF, externals, (NODE*) NULL, tp);
		tp = add_implicit_list(ENTRYDEF, heap_leaves, (NODE*) NULL, tp);
		tp = add_implicit_list(ENTRYDEF, statics, (NODE*) NULL, tp);
		tp = add_implicit_list(ENTRYDEF, arg_leaves, (NODE*) NULL, tp);
		if(!ISOP(bp->last_triple->op,BRANCH_OP)) {
			entry_block->last_triple = tp;
		}
	}

	lookfor_implicit();

	tp = exit_block->last_triple;
	if(ISOP(tp->op,BRANCH_OP)) {
		tp = tp->tprev;
	}
	tp = add_implicit_list(EXITUSE, externals, (NODE*) NULL, tp);
	tp = add_implicit_list(EXITUSE, heap_leaves, (NODE*) NULL, tp);
	tp = add_implicit_list(EXITUSE, statics, (NODE*) NULL, tp);
	if(!ISOP(entry_block->last_triple->op,BRANCH_OP)) {
		exit_block->last_triple = tp;
	}
	empty_listq(tmp_lq);
}

/*
**	scan all blocks and insert implicit uses and def triples for
**	leaves that are affected but do not explicitly appear.
**	For calls that are not intrinsic or support functions
**	externals are used and killed
**
** for expressions that contain several calls only 
** insert one set of external uses/defs around
** the surrounding root triples
** in x=f(i)+x the def of x implied by the
** call does not reach its use within the same 
** expression. this is legal because of order of
** evaluation freedom 
**
*/

LOCAL
lookfor_implicit()
{
register BLOCK *bp;
TRIPLE *tp, *last_root, *argp, *before, *after, *first, *call_site;
LIST *lp, *acc_list, *newl;
LEAF *leafp;
TRIPLE *ifetch;
FUNC_DESCR func_descr;
TRIPLE *external_uds = TNULL;	/* call triple to which external uses and defs*/
 								/* are credited*/
struct held_arg_def {
	LEAF *leafp;
	TRIPLE *call_site;
	struct held_arg_def  *next;
} *held_arg_defs = NULL, *adp;
TYPE type;

	type.tword = UNDEF;
	type.aux.size = 0;

	for(bp=entry_block; bp; bp=bp->next)    {
		if(bp->last_triple==TNULL) continue;

		first = bp->last_triple->tnext;
		TFOR(tp,first) {

			if(	tp->op == IMPLICITDEF || tp->op == IMPLICITUSE ||
				tp->op == ENTRYDEF || tp->op == EXITUSE ) {
				continue;
			}

			if( ISOP(tp->op,ROOT_OP ) ){
				/* 	new root: check if we have any pending implicit u/d and 
				**	insert uses after the previous root and defs before this 
				**	root
				*/
				if(external_uds != TNULL) {
					while(	last_root->tnext->op == IMPLICITUSE &&
							last_root->tnext->right == (NODE*)last_root){
						last_root = last_root->tnext;
					}
					add_implicit_list(IMPLICITUSE, externals,  
										external_uds, last_root);
				}

				before = tp->tprev;
				for(adp = held_arg_defs; adp ; adp = adp->next ) {
					call_site = adp->call_site;
					before = append_triple(before, IMPLICITDEF, 
									adp->leafp,(NODE*) call_site,type);
					newl = NEWLIST(proc_lq);
					(TRIPLE*) newl->datap = before;
					LAPPEND(call_site->implicit_def,newl);
				}
				held_arg_defs = NULL;

				if(external_uds != TNULL) {
					add_implicit_list(IMPLICITDEF, externals,  
										external_uds, before);
					external_uds = TNULL;
				}
				last_root = tp;
			}

			before = tp->tprev;
			after = tp;
			switch(tp->op) {

				case SCALL:
					if(tp->left->operand.tag == ISLEAF) {
						func_descr = tp->left->leaf.type.aux.func_descr;
					} else {
						func_descr = EXT_FUNC;
					}
					before = note_side_eff(IMPLICITUSE, tp->left,tp, before);
					TFOR(argp, (TRIPLE*) tp->right) {
						before =note_side_eff(IMPLICITUSE,argp->left,tp,before);
						if(argp->can_access != LNULL) {
							before = add_implicit_list(IMPLICITUSE, 
												argp->can_access, tp, before);
							if(func_descr !=INTR_FUNC){
								after = add_implicit_list(IMPLICITDEF, 
													argp->can_access,tp, after);
								LFOR(lp,argp->can_access) {
									leafp = (LEAF*) lp->datap;
									after = note_side_eff(IMPLICITDEF,leafp,tp,
													after);
								}
							}
						}
					}
					if(func_descr == EXT_FUNC) {
						add_implicit_list(IMPLICITUSE, externals,tp,before);
						add_implicit_list(IMPLICITDEF, externals, tp, after);
					}
					break;

				case FCALL:
					if(tp->left->operand.tag == ISLEAF) {
						func_descr = tp->left->leaf.type.aux.func_descr;
					} else {
						func_descr = EXT_FUNC;
					}
					before = note_side_eff(IMPLICITUSE, tp->left,tp, before);
					TFOR(argp, (TRIPLE*) tp->right) {
						before =note_side_eff(IMPLICITUSE,argp->left,tp,before);
						if(argp->can_access != LNULL) {
							before = add_implicit_list(IMPLICITUSE, 
												argp->can_access, tp, before);
							if(func_descr !=INTR_FUNC){
								LFOR(lp,argp->can_access) {
									adp = (struct held_arg_def *)
										ckalloca(sizeof(struct held_arg_def));
									adp->leafp = (LEAF*) lp->datap;
									adp->call_site = tp;
									adp->next = held_arg_defs;
									held_arg_defs = adp;
								}
							}
						}
					}
					if( tp->left->operand.tag == ISLEAF &&
						tp->left->leaf.class == CONST_LEAF &&
						tp->left->leaf.type.aux.func_descr != EXT_FUNC
					){
						/* then it's a support function */	
					} else {
						/* avoid putting out multiple copies of 
						**	external ud for an expression like 
						**	f1()+f2(). 
						**	The external uds get credited to f2
						*/
						external_uds = tp;
					}
					break;

				case IFETCH:
					before = note_side_eff(IMPLICITUSE, tp->left,tp, 
									before);
					before = add_implicit_list(IMPLICITUSE, tp->can_access, tp,
									before);
					LFOR(lp,tp->can_access) {
						leafp = (LEAF*) lp->datap;
						before = note_side_eff(IMPLICITUSE,leafp,tp,
										before);
					}
					break;

				case ISTORE:
					before = note_side_eff(IMPLICITUSE, tp->left,tp, 
									before);
					before = note_side_eff(IMPLICITUSE, tp->right,tp, 
									before);
					after = add_implicit_list(IMPLICITDEF, tp->can_access, tp,
									after);
					LFOR(lp,tp->can_access) {
						leafp = (LEAF*) lp->datap;
						after = note_side_eff(IMPLICITDEF,leafp,tp,
										after);
					}
					break;

				default:
					if( ISOP(tp->op, USE1_OP)) {
						note_side_eff(IMPLICITUSE, tp->left, tp, 
								before);
					}
					if( ISOP(tp->op, USE2_OP)) {
						note_side_eff(IMPLICITUSE, tp->right, tp, 
								before);
					}
					if( ISOP(tp->op, MOD_OP)) {
						 note_side_eff(IMPLICITDEF, tp->left, 
								tp, after);
					}
					break;

			}
		}
		if(external_uds != TNULL) {
			 quit("lookfor_implicit: function value(s) not used");
		}
	}
}

/* 
** find the set of leaves that can be affected (used or defined) along with
** the operand explicitly named in a triple. If the operand is a leaf then
** the access list could be all leaves statically aliased with it; if its
** an indirection then the access list is the union of the overlap lists
** of all leaves on the indirect nodes's can_access list
*/

LOCAL TRIPLE *
note_side_eff(op, operand, site, after)
IR_OP op;
TRIPLE *site;
TRIPLE *after;
NODE *operand;
{
LIST *acc_list = LNULL;
register LIST *lp, *lp2;
LEAF *leafp;
TRIPLE *tp_temp;
TYPE type;

	type.tword = UNDEF;
	type.aux.size = 0;
	if(operand->operand.tag == ISLEAF && operand->leaf.overlap) {
		acc_list = operand->leaf.overlap = 
					order_list(operand->leaf.overlap,proc_lq);
		LFOR(lp,acc_list) {
			leafp = (LEAF*) lp->datap;
			if(leafp != (LEAF*) operand ) {
				after = append_triple(after, op, leafp, 
							(NODE*) site, type);
				lp2 = NEWLIST(proc_lq);
				(TRIPLE*) lp2->datap = after;
				if( op == IMPLICITDEF ) {
					LAPPEND(site->implicit_def,lp2);
				}
				else if( op == IMPLICITUSE ) {
					LAPPEND(site->implicit_use,lp2);
				}
			}
		}
	}
	return after ;
}

/*
** put out a implicit uses or defs after after for every leaf on list and 
** credit them to site
*/
TRIPLE *
add_implicit_list(op, list, site, after)
IR_OP op;
LIST *list;
TRIPLE *after, *site;
{
register LIST *lp, *lp2, *acc_list;
register LEAF *leafp;
TYPE type,ptr_type;

	type.tword = UNDEF;
	type.aux.size = 0;
	LFOR(lp,list) {
		leafp = (LEAF*) lp->datap;
		after = append_triple( after , op, leafp, site, type);
		if( op == ENTRYDEF )
			leafp->entry_define = after;
		if( op == EXITUSE )
			leafp->exit_use = after;
		if(site != TNULL) {
			lp2 = NEWLIST(proc_lq);
			(TRIPLE*) lp2->datap = after;
			if( op == IMPLICITDEF ) {
				LAPPEND(site->implicit_def,lp2);
			}
			else if( op == IMPLICITUSE ) {
				LAPPEND(site->implicit_use,lp2);
			}
		}
	}
	return(after);
}
