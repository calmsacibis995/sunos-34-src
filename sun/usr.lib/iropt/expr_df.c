#ifndef lint
static	char sccsid[] = "@(#)expr_df.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include <stdio.h>

extern SET_PTR expr_gen,expr_kill, expr_in, expr_out;
extern SET_PTR exprdef_kill, exprdef_gen, exprdef_in,  exprdef_out;
extern SET_PTR exprdef_mask, all_exprdef_mask;
extern int exprdef_set_bitsize , exprdef_set_wordsize;
extern int expr_set_bitsize, expr_set_wordsize;
extern LISTQ *df_lq, *tmp_lq;

void bit_set();
BOOLEAN bit_test();
LIST *blowup_exprdef_in_bits();
static	int expr_avail_defno;

void
build_expr_sets(setbit_flag)
BOOLEAN setbit_flag;
{
/* This routine will build expr_gen, expr_kill, exprdef_gen and the */
/* exprdef_mask.  After the expr_kill for each block and the exprdef_mask */
/* have been built, it will call build_exprdef_kill to build exprdef_kill*/

register EXPR_REF *rp, *first_rp;
register EXPR *ep, **epp;
LIST *gen_list, *new_gen;
EXPR_KILLREF *last_kill;
int this_block, blockno;
TRIPLE *tp;
BOOLEAN *killed_in, killed;
register int exprno, i;
int tab_size;
int mask_index;


	tab_size = nblocks*exprdef_set_wordsize;
	if(setbit_flag == TRUE) {
		killed_in = (BOOLEAN*) ckalloca(nblocks*sizeof(BOOLEAN));
	}
	if(entry_block->blockno != 0) quit("build_exprsets: FIXME");
	expr_avail_defno = 0;
	gen_list = LNULL;
	last_kill = (EXPR_KILLREF*) NULL;
	/*
	**	examine all expressions
	*/
	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) 
	{
		for( ep = *epp; ep; ep = ep->next_expr )
		{
			if(ep->op == LEAFOP || ep->references == (EXPR_REF*) NULL) 
			{
				continue;
			}


			if( setbit_flag == TRUE )
			{
				for(i=0; i < nblocks; i++)
				{
					killed_in[i] = FALSE;
				}
			}

			exprno = ep->exprno;
			first_rp = ep->references;

			/*
			**	for each expression examine all references
			*/
			while(first_rp)
			{ /* for this expression */
				killed = FALSE;
				if(first_rp->reftype == EXPR_KILL) {
					this_block = ((EXPR_KILLREF*)first_rp)->blockno;
				} else {
					this_block =  first_rp->site.bp->blockno;
				}
				gen_list = LNULL;
				last_kill = (EXPR_KILLREF*) NULL;
				empty_listq(tmp_lq);
				rp = first_rp;

				do
				{ /* for this basic block */
					switch(rp->reftype) 
					{
						case EXPR_GEN:
							insert_list(&gen_list,rp, tmp_lq);
							last_kill = (EXPR_KILLREF*) NULL;
							break;

						case EXPR_KILL:
							killed = TRUE;
							/*
							** kill all definitions
							*/
							last_kill = (EXPR_KILLREF*) rp;
							gen_list = LNULL;
							break;

						case EXPR_USE1:
							tp = rp->site.tp;
							/*
							**	put all live definitions
							**	on the reachdef list
							*/
							tp->reachdef1 = 
							    copy_list(gen_list,df_lq);

							if( ! killed )
							{
								rp->reftype = 
								  EXPR_EXP_USE1;
							}
							break;

						case EXPR_USE2:
							tp = rp->site.tp;
							/*
							**	put all live definitions
							**	on the reachdef list
							*/
							tp->reachdef2 = 
							    copy_list(gen_list,df_lq);
	
							if( ! killed )
							{
								rp->reftype = 
								  EXPR_EXP_USE2;
							}
							break;
					}
					rp = rp->next_eref;
					if(rp) {
						if(rp->reftype == EXPR_KILL) {
							blockno = ((EXPR_KILLREF*)rp)->blockno;
						} else {
							blockno =  rp->site.bp->blockno;
						}
					} else {
						blockno = -1;
					}
				} while( blockno == this_block) ;
				first_rp = rp;
				/*
				** if any kills of this expr were found 
				** in this block note so
				*/
				if( setbit_flag == TRUE )
				{
					if( killed == TRUE )
					{
						killed_in[this_block] = TRUE;
					}

					if( gen_list )
					{ /* mark the expr_gen, expr_def_gen */
						build_expr_gen( gen_list, exprno);
					}
					else
					{
						if( last_kill )
						{ /*	there is a lasting kill of this expr */
						  /*	 mark it in the expr_kill set */
							bit_set( expr_kill, 
									last_kill->blockno, ep->exprno );
						}
					}
				}
			}
			if( setbit_flag == TRUE )
				build_exprdef_kill( killed_in, exprno );
		}
	}
	empty_listq(tmp_lq);
	free_expr_killref_space();
}

LOCAL
build_expr_gen( gen_list , exprno ) 
LIST *gen_list;
register int exprno;
{
	register LIST *lp;
	register EXPR_REF *rp;

	rp = LCAST( gen_list, EXPR_REF );
	bit_set( expr_gen, rp->site.bp->blockno, exprno );

	LFOR( lp, gen_list )
	{
		rp = LCAST( lp, EXPR_REF );
		rp->reftype = EXPR_AVAIL_GEN;
		rp->defno = expr_avail_defno++;
		bit_set( exprdef_gen, rp->site.bp->blockno, rp->defno);
		bit_set( exprdef_mask, exprno, rp->defno );
	}
}

LOCAL
build_exprdef_kill(killed_in, exprno)
BOOLEAN *killed_in;
int exprno;
{
register int bx, i;
register SET exprdef_kill_index;
register SET exprdef_mask_index;

	exprdef_mask_index = exprdef_mask->bits + ((short)exprno*(short)exprdef_set_wordsize);
	for(bx=0; bx< nblocks; bx++) 
	{ /* for each block */
		/*
		** if a kill of this expr occurred in this block 
		** set exprdef_kill to all definitions correspond to
		** this expression
		*/
		if(*killed_in++ == TRUE) 
		{
			exprdef_kill_index = exprdef_kill->bits + 
							((short)bx*(short)exprdef_set_wordsize);
			for(i = 0; i < exprdef_set_wordsize; i++ )
				*exprdef_kill_index++ |= exprdef_mask_index[i];
		} 
	}
}

/* compute IN and OUT for available expressions problem */
compute_avail_expr()
{
					/* point to a sets for a specific block*/
register BIT_INDEX in_index, out_index, kill_index, gen_index, mask_index;
BIT_INDEX newin, in, out, gen, kill;

register LIST *lp;
register BLOCK *bp;		/* steps through blocks */
BOOLEAN	change;			/* set if any changes propagated */
register int i;			/* loop control */
register int tab_size;
SET_PTR expr_newin, exprdef_newin;	/* temp sets for detecting changes */
int expr_block_offset;
int expr_passn = 0;
int exprdef_passn = 0;
int pred;
int j;
void build_exprdef_mask();

	/* get space for newin */
	AUTO_SET(expr_newin,1, nexprs);
	in  = expr_in->bits;
	out = expr_out->bits;
	gen = expr_gen->bits;
	kill = expr_kill->bits;
	newin = expr_newin->bits;

	/* initialize OUT[b] to GEN[b] */
	
	for (i=0; i<expr_set_wordsize; i++) 
	{
		in[i]=0;
		out[i]=gen[i];
	}

	tab_size = nblocks*expr_set_wordsize;
	for (i=expr_set_wordsize; i<tab_size; i++) 
	{
		in[i] = ~0;
		out[i] = ~kill[i];
	}

	change = TRUE;
	while (change == TRUE) 
	{
		expr_passn++;
		if(SHOWDF) 
		{
			printf("avail_exprs() PASS %d\ni in[i],out[i]\n",expr_passn);
		}
		change = FALSE;
		for (bp=entry_block->next;bp;bp=bp->next) 
		{

			expr_block_offset = (short)bp->blockno*(short)expr_set_wordsize;
			in_index = &in[expr_block_offset];
			kill_index = &kill[expr_block_offset];
			gen_index = &gen[expr_block_offset];
			for (i=0; i<expr_set_wordsize; i++) 
			{
				newin[i] = ~0;
			}

			/* for each of block bs predecessors */
			LFOR(lp,bp->pred) 
			{ /* newin gets OUT of the predecessor */
				out_index = &out[(short)LCAST(lp,BLOCK)->blockno*(short)expr_set_wordsize];
				for (i=0; i<expr_set_wordsize; i++) 
				{
					newin[i] &= out_index[i];
				}
			}

			/* set change according to whether IN[b] has changed */
			if(change == FALSE) 
			{
				for (i=0; i<expr_set_wordsize; i++) 
				{
					if (in_index[i] != newin[i]) 
					{
						change = TRUE;
						break;
					}
				}
			}

			out_index = &out[expr_block_offset];
			/* set IN[b] to NEWIN[b] */
			/* and OUT[b]=IN[b] - KILL[b] U GEN[b] */
			for (i=0; i<expr_set_wordsize; i++) 
			{

				in_index[i] = newin[i];
				out_index[i] =(in_index[i] & ~kill_index[i]) | gen_index[i];

				if(SHOWDF==TRUE) 
				{
					printf("e %d\t%X\t%X\n",bp->blockno,in_index[i],out_index[i]);
				}
			}
		}
	}
	build_exprdef_mask();

	/* NOW COMPUTE THE EXPR_DEF */

	AUTO_SET(exprdef_newin, 1, nexprdefs);
	in  = exprdef_in->bits;
	out = exprdef_out->bits;
	gen = exprdef_gen->bits;
	kill = exprdef_kill->bits;
	newin = exprdef_newin->bits;

	/* initialize OUT[b] to GEN[b] */
	
	tab_size = (nblocks)*exprdef_set_wordsize;
	for (i=0; i<tab_size; i++) 
	{
		in[i] = 0;
		out[i] = gen[i];
	}

	change = TRUE;
	while (change == TRUE) 
	{
		exprdef_passn++;
		if(SHOWDF) 
		{
			printf("avail_exprdefs() PASS %d\ni in[i],out[i]\n",exprdef_passn);
		}
		change = FALSE;
		for (bp=entry_block->next;bp;bp=bp->next) 
		{

			expr_block_offset = (short)bp->blockno*(short)exprdef_set_wordsize;
			in_index = &in[expr_block_offset];
			kill_index = &kill[expr_block_offset];
			gen_index = &gen[expr_block_offset];
			for (i=0; i<exprdef_set_wordsize; i++) 
			{
				newin[i] = 0;
			}

			/* for each of block bs predecessors */
			LFOR(lp,bp->pred) 
			{ /* newin gets OUT of the predecessor */
				out_index = &out[(short)LCAST(lp,BLOCK)->blockno*(short)exprdef_set_wordsize];
				for (i=0; i<exprdef_set_wordsize; i++) 
				{
					newin[i] |= out_index[i];
				}
			}

			mask_index = (all_exprdef_mask->bits  + expr_block_offset);
			for( i = 0; i < exprdef_set_wordsize; ++i )
			{
				newin[i] &= mask_index[i];
			}

			/* set change according to whether IN[b] has changed */
			if(change == FALSE) 
			{
				for (i=0; i<exprdef_set_wordsize; i++) 
				{
					if (in_index[i] != newin[i]) 
					{
						change = TRUE;
						break;
					}
				}
			}

			out_index = &out[expr_block_offset];
			/* set IN[b] to NEWIN[b] */
			/* and OUT[b]=IN[b] - KILL[b] U GEN[b] */
			for (i=0; i<exprdef_set_wordsize; i++) 
			{

				in_index[i] = newin[i];
				out_index[i] =(in_index[i] & ~kill_index[i]) | gen_index[i];

				if(SHOWDF==TRUE) 
				{
					printf("e %d\t%X\t%X\n",bp->blockno,in_index[i],out_index[i]);
				}
			}
		}
	}
}

/*
**	go through all exprs
**	and for each examine all references block, by block
**	set exposed uses to exprdef_forward_in[b] for that block
*/

expr_reachdefs()
{
register EXPR *ep, **epp;
register EXPR_REF *rp, *first_rp;
LIST *in_list;
BLOCK *this_block;

	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr )
		{
			if(ep->op == LEAFOP || ep->references == (EXPR_REF*) NULL) {
				continue;
			}
			first_rp = ep->references;
			while(first_rp) {
				this_block = first_rp->site.bp;
				rp = first_rp;
				in_list = blowup_exprdef_in_bits(ep,this_block);
				do {
					if(in_list) {
						if(rp->reftype == EXPR_EXP_USE1) {
							if(rp->site.tp->reachdef1) {
								rp->site.tp->reachdef1 = merge_lists(rp->site.tp->reachdef1,copy_list(in_list,df_lq));
							} else {
								rp->site.tp->reachdef1 = copy_list(in_list,
																	df_lq);
							}
						} else if(rp->reftype == EXPR_EXP_USE2){
							if(rp->site.tp->reachdef2) {
								rp->site.tp->reachdef2 = merge_lists(rp->site.tp->reachdef2,copy_list(in_list,df_lq));
							} else {
								rp->site.tp->reachdef2 = copy_list(in_list,
																df_lq);
							}
						}
					}
					rp = rp->next_eref;
				} while(rp && rp->site.bp == this_block);
				first_rp = rp;
			}
			empty_listq(tmp_lq);
		}
	}
}

LIST *
blowup_exprdef_in_bits(ep,bp)  
EXPR *ep;
BLOCK *bp;
{
LIST *list;
register EXPR_REF *rp;

	list = LNULL;
	for(rp = ep->references; rp ; rp = rp->next_eref) {
		if(rp->reftype == EXPR_AVAIL_GEN && 
					bit_test(exprdef_in,bp->blockno,rp->defno)==TRUE){
			insert_list(&list, rp, tmp_lq);
		}
	}
	return(list);
}

void
build_exprdef_mask()
{
/* An exprdef will be alived on an entry of a basic block, only if */
/* the correspond expression is alived on that entry */
/* all_exprdef_mask[block_no] is the set of the maximum possible */
/* exprdef can be alived on the entry that block */

	register BLOCK *bp;
	register int i, j;
	register BIT_INDEX mask_index;
	register SET exprdef_index;

	for( bp = entry_block->next; bp; bp = bp->next )
	{
		mask_index = all_exprdef_mask->bits + ((short)bp->blockno*
						(short)exprdef_set_wordsize);

		for( i = 0; i < nexprs; ++i )
		{
			if( bit_test( expr_in, bp->blockno, i ) )
			{
				exprdef_index = exprdef_mask->bits + ((short)i*
						(short)exprdef_set_wordsize);
				for( j = 0; j < exprdef_set_wordsize; ++j )
				{
					mask_index[j] |= exprdef_index[j];
				}
			}
		}
	}
}
