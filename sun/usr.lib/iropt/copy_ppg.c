#ifndef lint
static	char sccsid[] = "@(#)copy_ppg.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

# include "iropt.h"
# include <stdio.h>

extern BOOLEAN partial_opt;
LISTQ *copy_lq;
extern LISTQ *df_lq;
SET_PTR copy_kill, copy_gen, copy_in, copy_out;
int copy_set_wordsize;

void
do_local_ppg()
{ /* do copy propagation locally -- within the basic block */

	register BLOCK *block_ptr;
	register LIST *list_ptr, *lp;
	register TRIPLE *tp1, *tp2;
	void replace();
	
	if(copy_lq == NULL) copy_lq = new_listq();

	for( block_ptr = entry_block; block_ptr; block_ptr = block_ptr->next )
	{
		TFOR( tp1, block_ptr->last_triple->tnext )
		{
			if( tp1->op == ASSIGN && interesting_copy( tp1 ) )
			{
				LFOR(lp, tp1->canreach )
				{
					tp2 = LCAST(lp, EXPR_REF)->site.tp;
					if( tp2->op == IMPLICITUSE ||
						tp2->op == EXITUSE )
						continue;

					if( no_change(block_ptr, tp1->tnext,
							tp2, tp1->right ) )
						replace( tp2, tp1 );
				}
			}
		}
	}
}

void
do_global_ppg()
{
	void global_replace(), remove_triple();
	LIST **lpp, *lp1, *lp2, *lp3, *can_reach, *copy_list();
	int copyno;
	LEAF *right;
	COPY *cp;
	TRIPLE *tp, *triple;
	BLOCK *bp;
	BOOLEAN bit_test(), both_local_var();

	if(copy_lq == NULL) copy_lq = new_listq();

	/* compute the data flow infomation */
	entable_copies();
	if( ncopies == 0 ) /* no interested copy */
		return;
	copy_set_wordsize = ( roundup(ncopies,BPW) ) / BPW;
	AUTO_SET(copy_gen,nblocks, ncopies);
	AUTO_SET(copy_kill,nblocks, ncopies);
	AUTO_SET(copy_in,nblocks, ncopies);
	AUTO_SET(copy_out,nblocks, ncopies);
	build_copy_kill_gen();
	build_copy_in_out();


	for( lpp = copy_hash_tab; lpp < &copy_hash_tab[COPY_HASH_SIZE]; lpp++ )
	{ /* for all the interested copies */
		LFOR(lp1, *lpp)
		{ /* for each interested copy */

			cp = (COPY *)lp1->datap;
			cp->visited = TRUE;	/* been processed */
			copyno = cp->copyno;
			right = cp->right;
			LFOR(lp2, cp->define_at)
			{ /* for each occurs */
				tp = LCAST(lp2, COPY_REF)->site.tp;
				can_reach = copy_list( tp->canreach, copy_lq );
				LFOR(lp3, can_reach)
				{ /* for all the triple it can reach */
					triple = LCAST(lp3, EXPR_REF)->site.tp;
					if( triple->op == IMPLICITUSE ||
						triple->op == EXITUSE )
						continue;

					bp = LCAST(lp3, EXPR_REF)->site.bp;
					if(bit_test(copy_in, bp->blockno, copyno) )
					{ /* in the IN[] SET */
						if( no_change(bp,
								bp->last_triple->tnext,
								triple,	right))
						{ /* no local change */
							global_replace( bp,
									triple, tp,
									lp3, cp );
						}
					}
				}
			}
		}
	}

	for( bp = entry_block; bp; bp = bp->next )
	{ /* for each basic block */
		if( bp->last_triple )
		{
			TFOR( triple,  bp->last_triple->tnext )
			{ /* clean up */
				if( triple->canreach == LNULL && triple->op == ASSIGN &&
						triple->left->operand.tag == ISLEAF &&
						triple->right->operand.tag == ISLEAF &&
						can_delete( triple ) ) {
					if( ! partial_opt || 
						both_local_var(triple->left,triple->right)){
						remove_triple( triple, bp );
					}
				} else {
					if( triple->op == ASSIGN &&
							triple->left == triple->right ) {
					/* FIX THE CANREACH FIRST */
						LFOR( lp1, triple->reachdef2 ) {
							tp = LCAST(lp1,VAR_REF)->site.tp;
							lp2 = copy_list(triple->canreach, df_lq);
							tp->canreach = merge_lists(tp->canreach,lp2);
						}
						remove_triple( triple, bp );
					}
				}
			}
		}
	}
	empty_listq(copy_lq);
}

void
replace( tp, by_triple ) register TRIPLE *tp, *by_triple;
{
	if( tp->left == by_triple->left )
		tp->left = by_triple->right;
	if( tp->right == by_triple->left )
		tp->right = by_triple->right;
}

void
global_replace(bp, triple, tp, lp, by_copy ) 
BLOCK *bp; 
TRIPLE *triple; /* triple->will be modified */
TRIPLE *tp; /* tp->one of the defination point of the copy; must be an ASSIGN */
LIST *lp;
COPY *by_copy;
{
	TRIPLE *tp1;
	LIST *lp1, *lp2, *reachdef, *copy_list();
	void fix_copy_in(), delete_define_at();
	BLOCK *block_ptr;

	if( triple->left == (NODE *)by_copy->left )
	{
		triple->left = (NODE *)by_copy->right;
		reachdef = copy_list( triple->reachdef1, copy_lq );
		triple->reachdef1 = (LIST *) NULL;
		
		/* modify the canreach and reachdef */

		LFOR( lp1, tp->reachdef2 )
		{
			tp1 = LCAST( lp1, VAR_REF )->site.tp;
			lp2 = NEWLIST( df_lq );
			lp2->datap = lp->datap;
			LAPPEND(tp1->canreach, lp2);
			lp2 = NEWLIST( df_lq );
			lp2->datap = lp1->datap;
			LAPPEND(triple->reachdef1, lp2);
		}
		fix_reachdef( triple, reachdef, TNULL );
	}

	if( ISOP( triple->op, BIN_OP ) && triple->right == (NODE *)by_copy->left )
	{
		triple->right = (NODE *)by_copy->right;
		reachdef = copy_list(triple->reachdef2, copy_lq);
		triple->reachdef2 = (LIST *)NULL;
		
		/* modify the canreach */
		LFOR( lp1, tp->reachdef2 )
		{
			tp1 = LCAST( lp1, VAR_REF )->site.tp;
			lp2 = NEWLIST( df_lq );
			lp2->datap = lp->datap;
			LAPPEND(tp1->canreach, lp2);
			lp2 = NEWLIST( df_lq );
			lp2->datap = lp1->datap;
			LAPPEND(triple->reachdef2, lp2);
		}
		fix_reachdef( triple, reachdef, TNULL );

		/* replace the right leaf, will require more work */
		/* because triple could be another interested copy */
		
		if( triple->op == ASSIGN && triple->expr && 
					( ! ((COPY*)triple->expr)->visited ))
		{ /* this triple 'generate' a copy
		   * and has not been processed.
		   * delete triple from the copy->define_at list
		   */
		   	delete_define_at( triple, (COPY *)triple->expr );
			
			fix_copy_in( bp, triple->expr, by_copy );
			for( block_ptr = entry_block; 
					block_ptr; 
						block_ptr = block_ptr->next )
			{
				block_ptr->visited = FALSE;
			}
		}
	}
}

void
delete_define_at( tp, cp ) TRIPLE *tp; COPY *cp;
{
	LIST *lp;

	LFOR( lp, cp->define_at ) {
		if( tp == LCAST( lp, COPY_REF)->site.tp ) {
			delete_list( &cp->define_at, lp );
			break;
		}
	}
}

void
fix_copy_in( bp, cp, base_copy)
BLOCK *bp;
COPY *cp, *base_copy;
{ /*
   * Right now, we will only try to fix the IN[] set for the old
   * COPY.  The right way to do it is to re-compute the data flow 
   * information, but for a fast fix, we use the equation as following :
   * for all the successor of bp set the corresponding bit of the old
   * copy iff both bits for the old and the base copies are set.
   */

	LIST *lp;

	bp->visited = TRUE;
	LFOR(lp, bp->succ)
	{
		if( ! (LCAST(lp, BLOCK)->visited ) )
			fix_copy_in( lp->datap, cp, base_copy );
	}

	if( ! bit_test( copy_in, bp->blockno, base_copy->copyno ) )
		bit_reset( copy_in, bp->blockno, cp->copyno );

}

BOOLEAN
both_local_var( leafp1, leafp2 ) LEAF *leafp1, *leafp2;
{ /* return TRUE iff neight leafp1 nor leafp2 is exteranl or HEAP variable. */

	register union leaf_value *rv, *lv;
	struct segdescr_st *ldescr, *rdescr;
	
	lv = &leafp1->val;

	if( lv->addr.seg->descr.external == EXTSTG_SEG ||
				lv->addr.seg->descr.class == HEAP_SEG ) {
		return FALSE;
	}

	if( ! ISCONST( leafp2 ) ) {
		rv = &leafp2->val;
		if( rv->addr.seg->descr.external == EXTSTG_SEG ||
					rv->addr.seg->descr.class == HEAP_SEG ) {
			return FALSE;
		}
	}
	return TRUE;
}
