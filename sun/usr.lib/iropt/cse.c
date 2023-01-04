#ifndef lint
static  char sccsid[] = "@(#)cse.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include <stdio.h>
#include <ctype.h>

LISTQ *cse_lq;
LISTQ *df_lq;
/* 
 * This file contains the major function of doing common subexpression
 * elimination.  Right now, it will do the CSE locally (within the basic block)
 * first and then, after the global data flow information has been re-computed,
 * it will do the CSE globally (cross the basic block).
 */

cse_init()
{
register BLOCK *bp;
register TRIPLE *tp, *last;

	if(cse_lq == NULL) cse_lq = new_listq();
	for(bp=entry_block; bp; bp=bp->next) {
		last = bp->last_triple;
		TFOR(tp,last) {
			(TRIPLE*) tp->visited = TNULL;
		}
	}
}

do_local_cse()
{ /* 
   * This procedure will be called before the global data flow information
   * has been computed.  It will detect ALL the local common subexpression
   * and then call to the eliminate_local_cs.
   */

	register BLOCK *bp;
	register TRIPLE *tp, *tlp;
	extern eliminate_local_cs();

	for( bp = entry_block; bp; bp = bp->next )
	{ /* walk the triple backward, so it can catch the */
	  /* largest common supexpression first. */
		tp = bp->last_triple;
		tlp = TNULL;
		
		do
		{
			if( tp->reachdef1 /* can be reached */ && 
				tp->left->operand.tag == ISTRIPLE &&
				tp->reachdef1->next != tp->reachdef1 )
				eliminate_local_cs( bp, tp->reachdef1 );

			if( tp->reachdef2 /* can be reached */ && 
				tp->right->operand.tag == ISTRIPLE &&
				tp->reachdef2->next != tp->reachdef2 )
				eliminate_local_cs( bp, tp->reachdef2 );

			if( tlp ) { /* we are scaning parameter list */
				tp = tp->tnext;
				if( tp = (TRIPLE *)tlp->right ) {/* done with PARM */
					tp = tlp;
					tlp = TNULL;
				}
			} else { 
				tp = tp->tprev;
				if( ( tp->op == SCALL || tp->op == FCALL ) && ( tp->right ) ) {
					tlp = tp;
					tp = (TRIPLE *) tlp->right;
				}
			}
		}while( tp != bp->last_triple );
	}
}

eliminate_local_cs( bp, reachdef ) 
	BLOCK *bp;
	LIST *reachdef;
{
	TRIPLE *base_triple, *find_local_base(), *tp;
	LIST *lp, *can_reach;

	base_triple = find_local_base( reachdef ); /* base_triple is the one */
					           /* that going to be saved */
						   /* and re-used */
	can_reach = copy_list(base_triple->canreach,cse_lq);
	LFOR( lp, can_reach )
	{
		tp = LCAST(lp, EXPR_REF)->site.tp;
		if( ((TRIPLE *)tp->left)->expr == base_triple->expr )
		{
			tp->left = (NODE *)base_triple;
			fix_reachdef( tp, tp->reachdef1, base_triple );
			tp->reachdef1 = LNULL;
		}

		if( ISOP( tp->op, BIN_OP ) &&
			((TRIPLE *)tp->right)->expr == base_triple->expr )
		{
			tp->right = (NODE *)base_triple;
			fix_reachdef( tp, tp->reachdef2, base_triple );
			tp->reachdef2 = LNULL;
		}
	}
}

TRIPLE *
find_local_base( reachdef ) register LIST *reachdef;
{ /* Find the base for the local common subexpression.
   * For those common subexpressions in a single basic block, the base
   * triple is one that has the longest canreach chain.
   */
	register LIST *lp, *list_ptr;
	register TRIPLE *tp, *base_triple;
	register max_count = 0;
	register count;

	LFOR( lp, reachdef )
	{
		tp = LCAST(lp, EXPR_REF)->site.tp;
		count = 0;
		LFOR(list_ptr, tp->canreach)
		{
			++count;
		}
		if( count > max_count )
		{
			max_count = count;
			base_triple = tp;
		}
	}
	if( max_count == 0 )
			quit("can not find local base: find_local_base");

	return( base_triple );
}

do_global_cse()
{
	BLOCK *bp;
	TRIPLE *tp, *tp1, *tlp, *append_triple();
	LIST *lp, *lp1;
	extern eliminate_global_cs();
	BOOLEAN can_delete();
	LEAF *new_leaf, *new_temp();
	VAR_REF *var_rp, *new_var_ref();

	for( bp = entry_block; bp; bp = bp->next )
	{ /* for each basic block */
		tp = bp->last_triple;
		tlp = TNULL;
		
		do
		{ /* for each triple */
			if( tp->reachdef1 /* can be reached */ && 
				tp->left->operand.tag == ISTRIPLE &&
				tp->reachdef1->next != tp->reachdef1 )
				eliminate_global_cs( bp, tp, tp->left, tp->reachdef1 );

			if( tp->reachdef2 /* can be reached */ && 
				tp->right->operand.tag == ISTRIPLE &&
				tp->reachdef2->next != tp->reachdef2 )
				eliminate_global_cs( bp, tp, tp->right, tp->reachdef2 );

			if( tlp ) { /* we are scaning parameter list */
				tp = tp->tnext;
				if( tp = (TRIPLE *)tlp->right ) {/* done with PARM */
					tp = tlp;
					tlp = TNULL;
				}
			} else { 
				tp = tp->tprev;
				if( ( tp->op == SCALL || tp->op == FCALL ) && ( tp->right ) ) {
					tlp = tp;
					tp = (TRIPLE *) tlp->right;
				}
			}
		}while( tp != bp->last_triple );
	}
	for( bp = entry_block; bp; bp = bp->next )
	{ /* for each basic block */
		tlp = TNULL;
		
		TFOR( tp, bp->last_triple->tnext )
		{ /* for each triple */
			if( tlp /* scaning parameter list */ && tp == (TRIPLE *)tlp->right ) {
				/* done with parameter list, go back to nomal list */
				tp = tlp;
				tlp = TNULL;
			} else {
				if( ( tp->op == SCALL || tp->op == FCALL ) && tp->right ) {
					tlp = tp;
					tp = (TRIPLE *)tlp->right;
				}
			}
			if( tp->op == ASSIGN )
			{
				if( tp->canreach == LNULL )
				{
					if( can_delete( tp ) )
					{
						delete_triple( tp, bp );
					}
					continue;
				}

				if( tp->canreach == tp->canreach->next )
				{ /* one usage of this assignment */
					if( ( TRIPLE *)tp->right ==
						LCAST( tp->canreach, EXPR_REF )->site.tp )
					{ /* only usage is to re-define itself */
						if( can_delete( tp ) )
						{
							delete_triple( tp, bp );
						}
					}
				}
				continue;
			}	
	
			if( ! ISOP(tp->op, VALUE_OP) )
				continue;

			if( tp->canreach )
			{
				if( ( ( tp->canreach != tp->canreach->next ) ||
					( LCAST(tp->canreach, EXPR_REF)->site.bp != bp ) ) &&
							! tp->visited )
				{ /* be used more than one place or be used in another
				  /* block and has not been saved yet. */
					if( tp->expr && tp->expr->save_temp ) 
					{ /* already allocated */
						new_leaf = (LEAF *)tp->expr->save_temp;
					}
					else
					{
						new_leaf = new_temp(tp);
						if(tp->expr)
						{
							tp->expr->save_temp = new_leaf;
						}
					}
					(TRIPLE*)tp->visited = append_triple(
						tp, ASSIGN, new_leaf,
						tp, tp->type);
					/* build canreach chain */
					var_rp = new_var_ref(FALSE);
					var_rp->reftype = VAR_DEF;
					var_rp->site.tp = (TRIPLE *)tp->visited;
					var_rp->site.bp = bp;
					
					((TRIPLE*)tp->visited)->canreach =
						copy_list( tp->canreach, df_lq );

					tp->canreach = NEWLIST( df_lq );
					(VAR_REF *)tp->canreach->datap = var_rp;
					
					LFOR(lp, ((TRIPLE *)tp->visited)->canreach )
					{ /* replace with the new leaf */
						tp1 = LCAST(lp,EXPR_REF)->site.tp;
						if( tp1->left == (NODE *)tp )
						{
							tp1->left = (NODE *)new_leaf;
							LFOR( lp1, tp1->reachdef1 )
							{
								if(LCAST(lp1,EXPR_REF)->site.tp == tp )
								{
									(VAR_REF *)lp1->datap = var_rp;
									break;
								}
							}
						}
						if( ISOP(tp1->op, BIN_OP) && 
								( tp1->right == (NODE *)tp ) )
						{
							tp1->right = (NODE *)new_leaf;
							LFOR(lp1, tp1->reachdef2)
							{
								if( LCAST(lp1,EXPR_REF)->site.tp == tp )
								{
									(VAR_REF *)lp1->datap = var_rp;
									break;
								}
							}
						}
					}
				}
			}
			else
			{ /* VALUE_OP but reaches nowhere */
				if( can_delete( tp ) )
					delete_triple( tp, bp );
			}
		}
	}
	empty_listq(cse_lq);
}

eliminate_global_cs( bp, triple, cs_triple, reachdef )
BLOCK *bp;
TRIPLE *triple, *cs_triple;
LIST *reachdef;
{
	TRIPLE *base_triple, *tp, *append_triple();
	LIST *can_reach, *lp1, *lp2, *reach_def, *copy_list();

	reach_def = copy_list(reachdef,cse_lq);

	/* replace the expression reference in the triple by the base_triple */
	LFOR( lp1, reach_def )
	{
		if( ( base_triple = LCAST(lp1, EXPR_REF)->site.tp ) ==
								cs_triple )
			continue;

		if( LCAST(lp1,EXPR_REF)->site.bp == bp )
		/* assume local CSE has been done */
			continue;

		/* found a non-local base_triple */
		/* pick up any non-local base_triple should work */
		can_reach = copy_list( cs_triple->canreach ,cse_lq);

		/* delete the near by one */
		delete_triple( cs_triple, bp );

		LFOR(lp2, can_reach )
		{
			tp = LCAST(lp2, EXPR_REF)->site.tp;

			if( ( (TRIPLE *)tp->left ) == cs_triple )
			{
				tp->left = (NODE *)base_triple;

				/* make the base_triple be the only one on */
				/* tp"s reachdef1 chain, so the cse won"t over do it */
				tp->reachdef1 = NEWLIST( df_lq );
				tp->reachdef1->datap = lp1->datap;
			}

			if( ( ISOP( tp->op, BIN_OP ) ) && ((TRIPLE *)tp->right == cs_triple ))
			{
				tp->right = (NODE *)base_triple;
				tp->reachdef2 = NEWLIST( df_lq );
				tp->reachdef2->datap = lp1->datap;
			}
		}
		break;
	}
}

