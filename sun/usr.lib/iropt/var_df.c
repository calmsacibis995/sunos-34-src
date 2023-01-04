#ifndef lint
static	char sccsid[] = "@(#)var_df.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include <stdio.h>

int expr_set_wordsize, exprdef_set_wordsize;
SET_PTR expr_gen,expr_kill,expr_in,expr_out;
SET_PTR exprdef_gen,exprdef_kill,exprdef_in,exprdef_out;
SET_PTR exprdef_mask, all_exprdef_mask;

static int vardef_set_wordsize;	
static SET_PTR var_gen,var_kill,var_in,var_out;
static int var_avail_defno;

extern VAR_REF *var_ref_head;
extern LISTQ *df_lq, *tmp_lq;
BOOLEAN bit_test();
/*
	obtain data flow information for the computation graph:  first
	build a table of variable references; then go through triples
	list building the gen, kill, def and use sets; lastly solve in
	and out for both the forward and backward problems
*/

compute_df(do_vars,do_exprs,do_global) 
BOOLEAN do_vars,do_exprs,do_global;
{
static int passno = 0;

	passno++;

	/* build the expr symbol table then
	** build the var and expr ref tables and 
	** use them to form the references lists for exprs 
	** and leaves 
	*/
	if(do_exprs == TRUE) {
		entable_exprs();
		record_refs(TRUE);
		if(SHOWCOOKED == TRUE) {
			dump_exprs();
			dump_expr_refs();
			dump_var_refs();
		}
		free_depexpr_lists();
	} else {
		record_refs(FALSE);
		if(SHOWCOOKED == TRUE) {
			dump_var_refs();
		}
	}
   /*      the set wordsizes should depend on the number of avail defs
   **      but we don't yet know how many defs will be avail
   */
	vardef_set_wordsize =  ( roundup(nvardefs,BPW) ) / BPW;
	expr_set_wordsize = ( roundup(nexprs,BPW) ) / BPW;
	exprdef_set_wordsize = ( roundup(nexprdefs,BPW) ) / BPW;

	if(do_exprs == TRUE && exprdef_set_wordsize > 0) {
		compute_expr_df(do_global);
	}
	
	if(do_vars == TRUE && vardef_set_wordsize > 0) {
		compute_var_df(do_global);
	}
	
	if(vardef_set_wordsize > 0 || exprdef_set_wordsize > 0) {
		do_canreach();
	}
}

compute_var_df(do_global)
BOOLEAN do_global;
{

	if(do_global == TRUE) {

		AUTO_SET(var_gen,nblocks,nvardefs);
		AUTO_SET(var_kill,nblocks,nvardefs);
		AUTO_SET(var_in,nblocks,nvardefs);
		AUTO_SET(var_out,nblocks,nvardefs);

		build_var_sets(TRUE); 
		compute_var_in_out();
		var_reachdefs();
	} else {
		build_var_sets(FALSE); 
	}
}

compute_expr_df(do_global)
BOOLEAN do_global;
{
	if(do_global == TRUE) {
		AUTO_SET(expr_gen,nblocks,nexprs);
		AUTO_SET(expr_kill,nblocks,nexprs);
		AUTO_SET(expr_in,nblocks,nexprs);
		AUTO_SET(expr_out,nblocks,nexprs);

		AUTO_SET(exprdef_gen,nblocks,nexprdefs);
		AUTO_SET(exprdef_kill,nblocks,nexprdefs);
		AUTO_SET(exprdef_in,nblocks,nexprdefs);
		AUTO_SET(exprdef_out,nblocks,nexprdefs);

		/*for a given expr keeps track of all expr def bits that define it*/
		AUTO_SET(exprdef_mask,nexprs,nexprdefs);
		AUTO_SET(all_exprdef_mask,nblocks,nexprdefs);

		build_expr_sets(TRUE);
		compute_avail_expr();
		expr_reachdefs();
	} else {
		build_expr_sets(FALSE);
	}
}

dump_cooked(point)
int point;
{
register BLOCK *p;
register LIST *lp;
register TRIPLE *tlp;
int i=0;

	dump_segments();
	dump_leaves();
	printf("\n\tPOINT %d\n",point);
	for(p=entry_block;p;p=p->next) {

			printf("\nBLOCK [%d] %s label %d next %d loop_weight %d",p->blockno,
				(p->entryname ? p->entryname : ""), p->labelno,
				(p->next ? p->next->blockno : -1 ), p->loop_weight);
			printf(" pred: ");
			if(p->pred) LFOR(lp,p->pred->next) {
				printf("%d ",  ((BLOCK *)lp->datap)->blockno );
			}
			printf("succ: ");
			if(p->succ) LFOR(lp,p->succ->next) {
				printf("%d ",  ((BLOCK *)lp->datap)->blockno );
			}
			if(p->dfonext) {
				printf("dfonext %d ",  p->dfonext->blockno);
			}
			if(p->dfoprev) {
				printf("dfoprev %d ",  p->dfoprev->blockno);
			}
			printf("\n");
			if(p->last_triple) TFOR(tlp,p->last_triple->tnext) {
				print_triple(tlp,1);
			}
			printf("\n");
	}
}

/*
**	go through the references list of each leaf; for each basic
**	block in the list find the locally available definition (if
**	any) by scanning backwards, and the locally exposed uses by scanning
**	forwards. If a locally available definition is found for basic block b set
**	the corresponding bit in GEN(b) and  set KILL(c) for all bb c != b
*/

build_var_sets(setbit_flag)
BOOLEAN setbit_flag;
{
LIST *all_avail_defs;			/* list of all available definitions of a variable*/
LEAF *leafp;				/* steps through the leaf table */
VAR_REF *rp;				/* steps through the references list */
VAR_REF *def_rp;
BLOCK *this_block;
TRIPLE *tp;
LIST **lpp, *hash_link, *def_list;

	var_avail_defno=0;
	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(hash_link, *lpp) {
			leafp = (LEAF*) hash_link->datap;
			rp = leafp->references;
			all_avail_defs = LNULL;
			empty_listq(tmp_lq);
			while( rp )
			{ /* for this leaf */
				this_block = rp->site.bp;
				def_list = LNULL;
				do 
				{ /* for this block */
					tp = rp->site.tp;
					if(rp->reftype == VAR_USE1) 
					{
						if(def_list == LNULL) 
						{	
							rp->reftype = VAR_EXP_USE1;
						} 
						else 
						{
							tp->reachdef1 = copy_list(def_list,df_lq);
						}
					} 
					else
						if(rp->reftype == VAR_USE2) 
						{
							if(def_list == LNULL) 
							{	
								rp->reftype = VAR_EXP_USE2;
							}
							else 
							{
								tp->reachdef2 = copy_list(def_list,df_lq);
							}
						}
						else 
						{
					/* 
					**	after a definition is encountered any remaining uses are not
					**	exposed
					*/
							if(def_list == LNULL) 
							{
								def_list = NEWLIST(tmp_lq);
							}
							(VAR_REF*) def_list->datap = rp;
						}
				rp = rp->next_vref;
				} while( rp && rp->site.bp == this_block );
				if( def_list )
				{ /* there is a defination in this block */
					def_rp = (VAR_REF *)def_list->datap;
					def_rp->reftype = VAR_AVAIL_DEF;
					def_rp->defno = var_avail_defno++;
					if( setbit_flag == TRUE )
					{
						bit_set(var_gen, this_block->blockno, def_rp->defno);
						LAPPEND(all_avail_defs, def_list);
					}
				}
			}

			if( setbit_flag == TRUE && all_avail_defs != LNULL )
			{
				build_kill(all_avail_defs);
			}
		}
	}
}


build_kill(alldef_list)
register LIST *alldef_list;
{
register LIST *lp, *lp2;
register BLOCK *this_block;

	LFOR(lp,alldef_list) {
		this_block = LCAST(lp,VAR_REF)->site.bp;
		for(lp2 = lp->next; lp2 != lp; lp2 = lp2->next) {
				bit_set(var_kill, this_block->blockno, LCAST(lp2,VAR_REF)->defno);
		}
	}
}

/* compute IN and OUT for forward/union flow problem */
LOCAL
compute_var_in_out()
{
BIT_INDEX in,out,gen,kill,newin;
					/* point to sets for a specific block*/
BIT_INDEX in_index,out_index,kill_index,gen_index;	

LIST *lp;
BLOCK *bp;
BOOLEAN	change, block_change;	/* note if any changes propagated */
SET_PTR var_newin;				/* temp for detecting changes */
int passn = 0;

register int b;					/* start of a block's bits within a set */
register int temp;
register int i;			/* loop control */
register int setsize;

	AUTO_SET(var_newin, 1, nvardefs);
	gen = var_gen->bits;
	kill = var_kill->bits;
	in = var_in->bits;
	out = var_out->bits;
	newin = var_newin->bits;

	/* initialize IN[b] = 0 and OUT[b] = GEN[b] */
	setsize = vardef_set_wordsize;
	temp = nblocks * setsize;
	{ register BIT_INDEX in_ptr = in, out_ptr=out, gen_ptr=gen;
		for (i=0; i<temp; i++) {
			*in_ptr++ = 0;
			*out_ptr++ = *gen_ptr++;
		}
	}

	change = TRUE;
	while (change == TRUE) {
		passn++;
		if(SHOWDF== TRUE) {
			printf("compute_var_in_out PASS %d\ni in[i],out[i]\n",passn);
		}
		change = FALSE;
		for (bp=entry_block;bp;bp=bp->dfonext) {

			b=(short)bp->blockno*(short)setsize;
			in_index = &in[b];
			kill_index = &kill[b];
			gen_index = &gen[b];

			/* compute NEWIN: for each of block b's predecessors 
			** newin is set to the or of the predecessors' OUTs
			*/
			{ register BIT_INDEX newin_ptr = newin;
				for (i=0; i<setsize; i++) {
					*newin_ptr++ = 0;
				}
			}
			if(bp->pred) {
				register LIST *lp, *pred;
				pred = bp->pred;
				LFOR(lp,pred) {
					out_index = &out[(short)LCAST(lp,BLOCK)->blockno*(short)setsize];
					{ register BIT_INDEX newin_ptr = newin,  out_ptr = out_index;
						for (i=0; i<setsize; i++) {
							*newin_ptr++ |= *out_ptr++;
						}
					}
				}
			}

			/* test if IN eq NEWIN */
			block_change = FALSE;
			{ register BIT_INDEX in_ptr = in_index, newin_ptr = newin;
				for (i=0; i<setsize; i++) {
					if(*in_ptr++ != *newin_ptr++) {
						block_change = TRUE;
						break;
					}
				}
			}

			out_index = &out[b];
			if(block_change == TRUE) {
				/* set IN[b] to NEWIN[b] */
				/* and OUT[b]=IN[b] - KILL[b] U GEN[b] */
				{ register BIT_INDEX in_ptr = in_index, newin_ptr = newin,
										out_ptr = out_index, kill_ptr = kill_index,
										gen_ptr = gen_index;
					for (i=0; i<setsize; i++) {
						*in_ptr = *newin_ptr++;
						*out_ptr++ = (*in_ptr++ & ~*kill_ptr++) | *gen_ptr++;
					}
				}
				change = TRUE;
			}
			if(SHOWDF==TRUE) {
				for (i=0; i<setsize; i++) {
					printf("%d\t%X\t%X\n",bp->blockno,in_index[i],out_index[i]);
				}
			}
		}
	}
	if(SHOWDF==TRUE) {
		for(i=0;i<nblocks;i++) {
			printf("%d var_gen %X, var_kill %X, var_in %X, var_out %X\n",
			i,var_gen[i],var_kill[i],var_in[i],var_out[i]);
		}
		printf("compute_var_in_out: %d passes to converge\n",passn);
	}
}

LIST *
blowup_in_bits(leafp,bp)  
LEAF *leafp;
BLOCK *bp;
{
LIST *list, *new_l;
register VAR_REF *rp;

	list = LNULL;
	for(rp = leafp->references; rp ; rp = rp->next_vref ) {
		if(rp->reftype == VAR_AVAIL_DEF && 
					bit_test(var_in,bp->blockno,rp->defno) == TRUE ) {
			new_l = NEWLIST(tmp_lq);
			(VAR_REF *)new_l->datap = rp;
			LAPPEND(list, new_l);
		}
	}
	return(list);
}

var_reachdefs()
{
LEAF *leafp;
BLOCK *this_block;
LIST *in_list;
VAR_REF *rp;
LIST **lpp, *hash_link;

/*
**	go through all leaves
**	and for each list examine all references block, by block
**	set exposed uses to var_in[b] for that block
*/

	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(hash_link, *lpp) {
			leafp = (LEAF*) hash_link->datap;
			rp = leafp->references;
			in_list = LNULL;
			empty_listq(tmp_lq);
			while(rp) {
				this_block = rp->site.bp;
				in_list = blowup_in_bits(leafp,this_block);
				do {
					if(rp->reftype == VAR_EXP_USE1) {
						rp->site.tp->reachdef1 = copy_list(in_list,df_lq);
					} else if(rp->reftype == VAR_EXP_USE2){
						rp->site.tp->reachdef2 = copy_list(in_list,df_lq);
					}
					rp = rp->next_vref;
				} while(rp && rp->site.bp == this_block);
			}
		}
	}
}

do_canreach()
{
EXPR *ep, **epp;
VAR_REF *vsink_rp;
EXPR_REF *esink_rp;
TRIPLE *sink_tp;
register BOOLEAN istriple; 
register LIST *new_l, *sink_lp;
register TRIPLE *source_tp;

	for( vsink_rp = var_ref_head; vsink_rp; vsink_rp = vsink_rp->next ) {
		/* visit the var_ref_tab from the HEAD, so the canreach */
		/* will point to the last usage in the basic block, and */
		/* the register allocator will depend on this order */
		if(ISUSE(vsink_rp->reftype)) { 
			sink_tp = (TRIPLE*) vsink_rp->site.tp;
			if(vsink_rp->reftype==VAR_USE1 || 
						vsink_rp->reftype==VAR_EXP_USE1)
			{ 	register LIST *reachdef1 = sink_tp->reachdef1;

				istriple = 
					(sink_tp->left->operand.tag == ISTRIPLE ? TRUE : FALSE );
				LFOR(sink_lp,reachdef1) {
					if(istriple) {
						source_tp = LCAST(sink_lp,EXPR_REF)->site.tp;
					} else { 	/*sink_tp->left->operand.tag == ISLEAF*/
						source_tp = LCAST(sink_lp,VAR_REF)->site.tp;
					}
					new_l = NEWLIST(df_lq);
					(VAR_REF*) new_l->datap = vsink_rp;
					LAPPEND(source_tp->canreach,new_l);
				}
			} 
			else
			{ 	register LIST *reachdef2 = sink_tp->reachdef2;

				istriple = 
					(sink_tp->right->operand.tag == ISTRIPLE ? TRUE : FALSE );
				LFOR(sink_lp,reachdef2) {
					if(istriple) {
						source_tp = LCAST(sink_lp,EXPR_REF)->site.tp;
					} else {
						source_tp = LCAST(sink_lp,VAR_REF)->site.tp;
					}
					new_l = NEWLIST(df_lq);
					(VAR_REF*) new_l->datap = vsink_rp;
					LAPPEND(source_tp->canreach,new_l);
				}
			}
		}
	}

	if(nexprs)
		for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			for(esink_rp = ep->references; 
								esink_rp ; esink_rp = esink_rp->next_eref) {
				if( ISEUSE(esink_rp->reftype) ) {
					sink_tp = (TRIPLE*) esink_rp->site.tp;
					if(esink_rp->reftype==EXPR_USE1 || 
							esink_rp->reftype==EXPR_EXP_USE1)
					{ 	register LIST *reachdef1 = sink_tp->reachdef1;

						istriple = 
							(sink_tp->left->operand.tag == ISTRIPLE ? TRUE : FALSE );
						LFOR(sink_lp,reachdef1) {
							if(istriple) {
								source_tp = LCAST(sink_lp,EXPR_REF)->site.tp;
							} else {
								source_tp = LCAST(sink_lp,VAR_REF)->site.tp;
							}
							new_l = NEWLIST(df_lq);
							(EXPR_REF*) new_l->datap = esink_rp;
							LAPPEND(source_tp->canreach,new_l);
						}
					} 
					else 
					{ 	register LIST *reachdef2 = sink_tp->reachdef2;

						istriple = 
							(sink_tp->right->operand.tag == ISTRIPLE ? TRUE : FALSE );
						LFOR(sink_lp,reachdef2) {
							if(istriple) {
								source_tp = LCAST(sink_lp,EXPR_REF)->site.tp;
							} else {
								source_tp = LCAST(sink_lp,VAR_REF)->site.tp;
							}
							new_l = NEWLIST(df_lq);
							(EXPR_REF*) new_l->datap = esink_rp;
							LAPPEND(source_tp->canreach,new_l);
						}
					}
				}
			}
		}
	}
}
