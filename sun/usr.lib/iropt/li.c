#ifndef lint
static	char sccsid[] = "@(#)li.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include "iropt.h"
#include "loop.h"
#include <stdio.h>

/*
**	code motion transformations :
**	visit each loop, outermost ones first; make a list of all invariant
**	triples in the loop, go through this list and move all root triples
**	to the preheader
*/

void analyze_loop(), scan_loop_tree(), move_trees_backwards(), move_triple(),
adjust_var_refs(), test_exit_dominator();
BOOLEAN isconst_triple(), isconst_call(), isconst_def();
BOOLEAN isconst_operand1(), isconst_operand2();
extern LISTQ *loop_lq;
extern int n_iv, n_rc;
static BLOCK *from_bp;
int preloop_ntriples, preloop_nleaves;

void
scan_loop_tree(node,do_iv)
LOOP_TREE *node;
BOOLEAN do_iv;
{
register LIST *lp;
register LOOP_TREE *ltp2;
register char *cp;

	if( node == (LOOP_TREE *) NULL ) return;
	analyze_loop(node->loop,do_iv);
	for(ltp2=loop_tree_tab,cp = node->children;
		cp < &node->children[nloops]; cp++,ltp2++) {
		if (*cp) {
			scan_loop_tree(ltp2,do_iv);
		}   
	}
}

LOCAL void
analyze_loop(loop, do_iv)
register LOOP *loop;
BOOLEAN do_iv;
{
register LIST *lp, *lp_temp, *invariant_tail;
register TRIPLE *tp;
register BLOCK *bp;

	if (loop == (LOOP*) NULL) return;
	/*
	**	the triple's visited flag is used to mark invariant triples:
	**	if non-zero it record the block the triple is in before being moved
	**	initialize all triples in the region to not constant
	**	the block's visited flag is used
	**	to avoid calculating exit dominators unless necessary
	*/
	LFOR(lp_temp, loop->blocks) {
		bp = (BLOCK*) lp_temp->datap;
		bp->visited = FALSE;
		TFOR(tp,bp->last_triple) {
			tp->visited = FALSE;
		}
	}
	loop->invariant_triples = invariant_tail = LNULL;
	/*
	**	traverse the blocks in the loop in dfo order
	**	ie parents before children. Check that a block is
	**	in the loop and that it has triples; make a list
	**	of the eligible loop constants then move them to the 
	**	preheader
	*/
	if(do_iv == TRUE) iv_init();
	for(bp=entry_block;bp;bp=bp->dfonext) {
		if(INLOOP(bp->blockno,loop->loopno) && bp->last_triple) {
			TFOR(tp,bp->last_triple->tnext) {
				if(isconst_triple(tp,bp,loop) == TRUE) {
					/*if the triple is invariant remember which block it's in */
					tp->visited =  (BOOLEAN) bp;
					lp_temp = NEWLIST(loop_lq);
					(TRIPLE*) lp_temp->datap = tp;
					/*
					 * For a single link list, build the
					 * list in the reverse order of the
					 * LAPPEND.
					 */
					
					if( loop->invariant_triples == LNULL ) {
						loop->invariant_triples = invariant_tail = lp_temp;
					} else {
						lp_temp->next = loop->invariant_triples;
						loop->invariant_triples = invariant_tail->next = lp_temp;
					}
				}
			}
		}
	}
	if(SHOWCOOKED == TRUE) {
		print_loop(loop);
	}
	if(do_iv == TRUE) {
		do_induction_vars(loop);
	} else {
		move_trees_backwards(loop);
	}
/*
 * 	if(SHOWCOOKED == TRUE) {
 *		dump_cooked(loop->loopno);
 *	}
 */
}

/*
**	is a triple invariant in this loop ? a value triple is invariant
**	if it always yields the same value, a mod triple is invariant
**	if the definition is always the same (see isconst_def...)
*/
LOCAL BOOLEAN
isconst_triple(tp,bp,loop)
TRIPLE *tp;
BLOCK *bp;
LOOP *loop;
{
LIST *lp;
VAR_REF *rp;
BOOLEAN const_operand1=TRUE, const_operand2=TRUE;
LEAF *leafp;
TRIPLE *implicit;

	if( ! (ISOP(tp->op, VALUE_OP) || ISOP(tp->op, MOD_OP) )  || 
				tp->tripleno >= preloop_ntriples) {
		return(FALSE);
	}

	switch(tp->op) {
		case IMPLICITDEF:
		case IMPLICITUSE:
		case ENTRYDEF:
		case EXITUSE:
			return FALSE;

		case SCALL:
		case FCALL:
			return isconst_call(tp,loop) ;

		case ADDROF:
			return TRUE;
			break;

	 	case ASSIGN:
			const_operand1 = isconst_def(tp,bp,loop);
			const_operand2 = isconst_operand2(tp,loop);
			break;

	 	case ISTORE:
			/* 
			**	first check that the address through which we are
			**	indirecting is a loop constant - if so check that all
			**	the definitions that can result from the istore are
			**	invariant
			*/
			const_operand1 = isconst_operand1(tp,loop);
			const_operand2 = isconst_operand2(tp,loop);
			if( const_operand1 == FALSE){
				return(FALSE);
			}
			LFOR(lp, (LIST *)tp->implicit_def) {
				implicit = (TRIPLE*) lp->datap;
				if(isconst_def(implicit,bp,loop) == FALSE) {
					return(FALSE);
				}
			}
			break;

		case IFETCH:
			const_operand1 = isconst_operand1(tp,loop);
			if( const_operand1 == FALSE){
				return(FALSE);
			}
			LFOR(lp, (LIST *)tp->implicit_use) {
				implicit = (TRIPLE*) lp->datap;
				if(isconst_operand1(implicit,loop) == FALSE) {
					return(FALSE);
				}
			}
			break;

		default:
			if(ISOP(tp->op, USE1_OP)) {
				const_operand1 = isconst_operand1(tp,loop);
			}
			if(ISOP(tp->op, USE2_OP)) {
				const_operand2 = isconst_operand2(tp,loop);
			} else {
				const_operand2 = TRUE;
			}
			break;

	}
	return ( (BOOLEAN) (const_operand1 == TRUE && const_operand2 == TRUE) );
}

/*
**	is a left operand constant in this loop ?
*/
LOCAL BOOLEAN
isconst_operand1(tp,loop)
register TRIPLE *tp;
register LOOP *loop;
{
register VAR_REF *rp;
register LIST *lp;

	if(tp->left->operand.tag == ISTRIPLE) {
		/*
		**	because triples are a tree in post order
		**	if the operand is a triple and invariant it will have been 
		**	marked invariant
		*/
		return(tp->left->triple.visited ? TRUE : FALSE );
	} else if(tp->left->operand.tag == ISLEAF) {
		if(ISCONST(tp->left)) {
			return(TRUE);
		} else {
			if(tp->left->leaf.leafno >= preloop_nleaves)  {
				/* leaves created in the course of code motion have inaccurate
				** reachdef information - assume the worst
				*/
				return(FALSE);
			} else if(tp->reachdef1 == LNULL) {
				set_rc(tp->left);
				return(TRUE);
			} else if(tp->reachdef1->next == tp->reachdef1) {
				/*
				**	one definition reaches:if it's outside loop the operand
				**	is invariant and a rc otherwise if the def site is invariant
				**	this operand is but it's not an rc
				*/
				rp = (VAR_REF*) tp->reachdef1->datap;
				if(	!(INLOOP(rp->site.bp->blockno,loop->loopno)) ) {
					set_rc(tp->left);
					return(TRUE);
				} else if(rp->site.tp->visited) {
					return(TRUE);
				} else {
					return(FALSE);
				}
			} else {
				/*
				**	if there several definitions reach they must all
				**	come from outside the loop
				*/
				LFOR(lp,tp->reachdef1) {
					rp = (VAR_REF*) lp->datap;
					if(	INLOOP(rp->site.bp->blockno,loop->loopno) ) {
						return(FALSE);
					}
				}
				set_rc(tp->left);
				return(TRUE);
			}
		}
	}
} 

BOOLEAN LOCAL
isconst_operand2(tp,loop)
register TRIPLE *tp;
register LOOP *loop;
{
register VAR_REF *rp;
register LIST *lp;

	if(tp->right->operand.tag == ISTRIPLE) {
		return(tp->right->triple.visited ? TRUE : FALSE );
	} else if(tp->right->operand.tag == ISLEAF) {
		if(ISCONST(tp->right)) {
			return(TRUE);
		} else {
			if(tp->right->leaf.leafno >= preloop_nleaves)  {
				return(FALSE);
			} else if(tp->reachdef2 == LNULL) {
				set_rc(tp->right);
				return(TRUE);
			} else if(tp->reachdef2->next == tp->reachdef2) {
				rp = (VAR_REF*) tp->reachdef2->datap;
				if(	!(INLOOP(rp->site.bp->blockno,loop->loopno)) ) {
					set_rc(tp->right);
					return(TRUE);
				} else if(rp->site.tp->visited) {
					return(TRUE);
				} else {
					return(FALSE);
				}
			} else {
				LFOR(lp,tp->reachdef2) {
					rp = (VAR_REF*) lp->datap;
					if(	INLOOP(rp->site.bp->blockno,loop->loopno) ) {
						return(FALSE);
					}
				}
				set_rc(tp->right);
				return(TRUE);
			}
		}
	}
} 

LOCAL BOOLEAN
isconst_call(tp, loop)
TRIPLE *tp;
LOOP *loop;
{
	/*
	**	should analyze func_descr to decide whether call is movable FIXME
	*/
	return(FALSE);
}

LOCAL BOOLEAN
isconst_def(def_tp, def_bp, loop)
TRIPLE *def_tp;
BLOCK *def_bp;
LOOP *loop;
{
TRIPLE *use_tp;
BLOCK *use_bp;
LIST *lp, *lp2, *reachdef;
LEAF *leafp;
VAR_REF *rp;

	if(def_tp->left->operand.tag != ISLEAF ) {
		quit("isconst_def: lhs of a mod triple not a leaf");
	}
	leafp = (LEAF*) def_tp->left;

	/*	three conditions must be met for an assignment to be moved to the 
	**	preheader - from Dragon Book Section 13.4
	*/
	LFOR(lp,def_tp->canreach) {
		rp = (VAR_REF*) lp->datap;
		use_tp = rp->site.tp;
		use_bp = rp->site.bp;
		if(! INLOOP(use_bp->blockno,loop->loopno)) {
			/*
			**	If def_tp can reach outside the loop then  the
			**	assignment can be moved only if it's in a block
			**	which dominates all exits.
			**	The block's visited flag is used
			**	to avoid re-calculating exit dominators 
			*/
			if(def_bp->visited == FALSE) {
				test_exit_dominator(def_bp,loop);
				def_bp->visited = TRUE;
			}
			if(INLOOP(def_bp->blockno,loop->loopno) 
						== IS_EXIT_DOMINATOR) {
				continue;
			} else {
				return FALSE ;
			}
		}
	}

	rp = leafp->references;
	while(rp) {
		if(	INLOOP(rp->site.bp->blockno, loop->loopno) ) {
			switch (rp->reftype) {
				case VAR_DEF:
				case VAR_AVAIL_DEF:
					if( rp->site.tp != def_tp ) {
						/* condition 2: no more than 1 def in loop */
						return FALSE;
					}
					break;

				case VAR_USE1:
				case VAR_EXP_USE1:
				case VAR_USE2:
				case VAR_EXP_USE2:
					/* condition 3
					**	if the use is inside the loop then def_tp must be the
					**	only triple to reach it
					*/
					use_tp = rp->site.tp;
					if(rp->reftype == VAR_USE1 || rp->reftype == VAR_EXP_USE1) {
						reachdef = use_tp->reachdef1;
					} else {
						reachdef = use_tp->reachdef2;
					}
					if(reachdef && reachdef->next == reachdef && 
							LCAST(reachdef,VAR_REF)->site.tp == def_tp ) {
					} else {
						return FALSE ;
					}
					break;
			}
		}
		rp = rp->next_vref;
	}
	return TRUE ;
}

/*
**	if a block dominates all exits from the loop note this in the
**	loop * block table
*/
LOCAL void
test_exit_dominator(bp,loop)
register BLOCK *bp;
register LOOP *loop;
{
register LIST *lp;
register BLOCK *exit;
register char *loop_block_index;

	loop_block_index = & loop_block_tab[nblocks*loop->loopno];
	LFOR(lp,loop->blocks) {
		exit = (BLOCK*) lp->datap;
		if( loop_block_index[exit->blockno] != IS_EXIT) {
			continue;
		}
		if(dominates(bp->blockno, exit->blockno) == FALSE) {
			return;
		}
	}
	loop_block_index[bp->blockno] = IS_EXIT_DOMINATOR;
}

LOCAL void
mark_tree(parent)
register TRIPLE *parent;
{
	if( ISOP(parent->op, USE1_OP) && parent->left->operand.tag == ISTRIPLE ) {
		mark_tree(parent->left);
	} 
	if(ISOP(parent->op,USE2_OP) && parent->right->operand.tag==ISTRIPLE){
		mark_tree(parent->right);
	}
	(BLOCK*) parent->visited = (BLOCK*) NULL;
}

LOCAL void
move_trees_backwards(loop)
register LOOP *loop;
{
register LIST *lp, *last;
register TRIPLE *tp;
TRIPLE *root, *parent, *find_parent(), *append_triple();
LEAF *tmp, *new_temp();
typedef struct invariant_tree {
	BLOCK *from_bp; 
	TRIPLE *root; 
	struct invariant_tree *next;
} INV_TREE;
INV_TREE *ivtp, *inv_tree_head, *inv_tree_tail;

	if(loop->invariant_triples == LNULL) return;
	inv_tree_head = inv_tree_tail = (INV_TREE*) NULL;
	last = loop->invariant_triples; /* invariant_triple is a "backword" list */
	LFOR( lp, last )
	{
		tp = (TRIPLE*) lp->datap;
		if(ISOP(tp->op,ROOT_OP)) {
			ivtp = (INV_TREE *) ckalloc(1,sizeof(INV_TREE));
			ivtp->from_bp = (BLOCK*) tp->visited;
			ivtp->root = (TRIPLE*) tp;
			ivtp->next = inv_tree_head;
			inv_tree_head = ivtp;
			if(inv_tree_tail ==(INV_TREE*) NULL) {
				inv_tree_tail = ivtp;
			}
			mark_tree(tp);
		}
	}
	last = loop->invariant_triples;
	LFOR( lp, last ) {
		tp = (TRIPLE*) lp->datap;
		if((BLOCK*) tp->visited == (BLOCK*) NULL ) {
			continue;
		}
		parent = find_parent(tp);
		if(tp->expr && tp->expr->save_temp != (LEAF*) NULL ) { 
			/* temp already allocated*/
			tmp = tp->expr->save_temp;
		} else {
			tmp = new_temp(tp);
			if(tp->expr) {
				tp->expr->save_temp = tmp;
			}
		}
	   	if((TRIPLE*)parent->left == tp) {
		   (LEAF*) parent->left = tmp;
	   	} else {
		   (LEAF*) parent->right = tmp;
	   	}
		root = append_triple(tp, ASSIGN, tmp, tp, tp->type);
		ivtp = (INV_TREE *) ckalloc(1,sizeof(INV_TREE));
		ivtp->from_bp = (BLOCK*) tp->visited;
		ivtp->root = root;
		ivtp->next = (INV_TREE*) NULL;

		if(inv_tree_head ==(INV_TREE*) NULL) {
			inv_tree_head = ivtp;
		}
		if(inv_tree_tail != (INV_TREE*) NULL) {
			inv_tree_tail->next = ivtp;
		}
		inv_tree_tail = ivtp;
		mark_tree(root);
	}	
	for(ivtp = inv_tree_head; ivtp ; ivtp = ivtp->next ) {
			from_bp = ivtp->from_bp;
			move_triple(ivtp->root,loop->preheader);
	}
}

/*
**	change the block pointer for all reference records associated with a
**	triple in preparation for moving the triple out of the block it's in
*/
LOCAL void
adjust_var_refs(tp, to_bp)
register TRIPLE *tp;
BLOCK *to_bp;
{
register LIST *lp;
register VAR_REF *vrp;

	if( tp->var_refs ) {
		vrp = tp->var_refs;
		while(vrp && vrp->site.tp == tp) {
			vrp->site.bp = to_bp;
			vrp = vrp->next;
		}
	}
	/* note that the var ref lists are no longer in the
	** same order as the triples within blocks 
	*/
}

LOCAL void
move_triple(tp,to_bp)
TRIPLE *tp;
BLOCK *to_bp;
{
register VAR_REF *vrp;
register int index;
LIST *lp;

	if(tp->left->operand.tag == ISTRIPLE) {
		move_triple(tp->left,to_bp);
	}
	if(ISOP(tp->op,BIN_OP) && tp->right->operand.tag == ISTRIPLE) {
		move_triple(tp->right,to_bp);
	}

	adjust_var_refs(tp,to_bp);

	/* first move any associated implicit uses */
	LFOR(lp,tp->implicit_use) {
		move_triple((TRIPLE*)lp->datap, to_bp);
	}

	/* then the triple */
	tp->tprev->tnext = tp->tnext;
	tp->tnext->tprev = tp->tprev;
	if(from_bp->last_triple == tp) {
		quit("move_triple: block does not terminate with a branch");
	}
	tp->tprev = tp->tnext = tp;
	/*
	**	the last triple in the preheader is the goto to the header
	**	so use tprev
	*/
	TAPPEND(to_bp->last_triple->tprev,tp);

	/* then any implicit defs */
	LFOR(lp,tp->implicit_def) {
		move_triple((TRIPLE*)lp->datap, to_bp);
	}
}
