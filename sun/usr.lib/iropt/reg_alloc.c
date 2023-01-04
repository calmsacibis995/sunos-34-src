#ifndef lint
static	char sccsid[] = "@(#)reg_alloc.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * This file contains the main routine of the doing register
 * allocateion.
 */
#include "iropt.h"
#include <stdio.h>
#include <ctype.h>
#include "reg.h"
LISTQ *reg_lq;
extern LISTQ *tmp_lq;
extern int regmask;

#define ISBRANCHOP( op )	( ( op == CBRANCH) || ( op == SWITCH ) || ( op == REPEAT ) \
					|| ( op == INDIRGOTO ) || ( op == GOTO ) )
#define SET_REGMASK(fpa,b,f81,a,d)	(((FIRST_FPAREG+MAX_FPAREG-fpa) << 16 ) | \
					(( b )<< 12 ) | \
					((FIRST_FREG-MAX_FREG+f81)<<8) | \
					((FIRST_AREG-MAX_AREG+a)<<4) | \
					(FIRST_DREG-MAX_DREG+d))
/*
 * The reference count of each uses of a variable is
 * the difference of the execution cycle (approximetly) between
 * put this variable in the register and put it in memory (assume all
 * using base-displacement mode.
 */


int reg_life_wordsize;
int reg_share_wordsize;
int n_dreg, n_areg, n_freg, max_freg;
extern int nleaves;

do_register_alloc()
{
	void build_du_web(), weigh_du_web(), modify_triples(), 
		compute_interference(), delete_list(),
		find_dominators(), share_registers();
	LIST *lp, *lpa, *lpd, *lpf, *reg_a, *reg_d, *reg_f,
		*cur_a, *cur_d, *cur_f;
	BOOLEAN more_d, more_a, more_f;
	int a_weight, d_weight, f_weight, first_freg;
	SORT *sp;

	reg_init();

	build_du_web();
	if( nwebs <= 1 ) /* no web worth to put into the register */
		return;

	compute_interference();
	weigh_du_web();

	/* Do register allocation according to the information */
	/* gather from the above calls. sort_a_list should points */
	/* to the heaviest a web and so does sort_d_list and
	/* sort_f_list. */

	share_registers();

	/* 
	 * sort_a_list, sort_d_list and sort_f_list is in order 
	 * of the weight -- heaviest is the first one 
	 * in the list and than the ->next
	 */

	n_areg = MAX_AREG;
	n_dreg = MAX_DREG;
	if( usefpa ) {
		max_freg = n_freg = MAX_FPAREG;
		first_freg = FIRST_FPAREG;
	} else {
		if( use68881 ) {
			max_freg = n_freg = MAX_FREG;
			first_freg = FIRST_FREG;
		} else {
			max_freg = n_freg = 0;
		}
	}
	lpa = sort_a_list ? sort_a_list->next : LNULL;
	lpd = sort_d_list ? sort_d_list->next : LNULL;
	lpf = sort_f_list ? sort_f_list->next : LNULL;
	
	more_d = ( lpd && lpd->datap != (LDATA *)NULL ) ?  TRUE : FALSE;
	more_a = ( lpa && lpa->datap != (LDATA *)NULL ) ?  TRUE : FALSE;
	more_f = ( lpf && lpf->datap != (LDATA *)NULL ) ?  TRUE : FALSE;

	cur_d = reg_d = (LIST *)ckalloc(MAX_DREG, sizeof( LIST ) );
	cur_a = reg_a = (LIST *)ckalloc(MAX_AREG, sizeof( LIST ) );
	cur_f = reg_f = (LIST *)ckalloc( n_freg, sizeof( LIST ) );
	
	while( ( n_areg && more_a ) || ( n_dreg && more_d ) || (n_freg && more_f ) )
	{ /* more registers and more webs to allocate */
		
		a_weight = d_weight = f_weight = 0;
		
		while( n_areg && more_a && a_weight == 0 )
		{
			a_weight = LCAST( lpa, SORT )->weight;
			if( a_weight == 0 )
			{ /* skip those webs that have been merged */
				lpa = lpa->next;
				if( lpa->datap == (LDATA *)NULL ) {
					more_a = FALSE;
					lpa = LNULL;
				}
				continue;
			}
		}

		while( n_dreg && more_d && d_weight == 0 )
		{
			d_weight = LCAST( lpd, SORT )->weight;
			if( d_weight == 0 )
			{ /* skip those webs that have been merged */
				lpd = lpd->next;
				if( lpd->datap == (LDATA *)NULL ) {
					more_d = FALSE;
					lpd = LNULL;
				}
				continue;
			}
		}

		while( n_freg && more_f && f_weight == 0 )
		{

			f_weight = LCAST( lpf, SORT )->weight;
			if( f_weight == 0 )
			{ /* skip those webs that have been merged */
				lpf = lpf->next;
				if( lpf->datap == (LDATA *)NULL ) {
					more_f = FALSE;
					lpf = LNULL;
				}
				continue;
			}
		}

		if( ( ( d_weight + INIT_CONT ) <= 0 ) && 
			( ( a_weight + INIT_CONT ) <= 0 ) &&
			( ( f_weight + FINIT_CONT) <= 0 ) )/* DONE */
			break;

		if( ( a_weight > f_weight ) && ( ( a_weight > d_weight ) ||
			( ( a_weight == d_weight ) && ISPTR( LCAST(lpa, SORT)->web->leaf->type.tword ) ) ) )
		{ /* allocate a A register for lpa or for fpa_base */
			if( usefpa && fpa_base_weight > a_weight && fpa_base_reg == -1 ) {
				fpa_base_reg = FIRST_AREG - MAX_AREG + n_areg--;
				continue;
			}
			cur_a->datap = lpa->datap;
			LCAST(cur_a, SORT)->regno = FIRST_AREG - MAX_AREG + n_areg--;
			if( lpd && LCAST(lpd, SORT)->web == LCAST(lpa, SORT)->web ) {
				lpd = lpd->next;
				if( lpd->datap == (LDATA *) NULL ) {
					more_d = FALSE;
					lpd = LNULL;
				}
			}
			if( lpf && LCAST(lpf, SORT)->web == LCAST(lpa, SORT)->web ) {
				lpf = lpf->next;
				if( lpf->datap == (LDATA *) NULL ) {
					more_f = FALSE;
					lpf = LNULL;
				}
			}

			if( LCAST(lpa, SORT)->web->sort_d ) { /* in D list too */
				lp = LCAST(lpa, SORT)->web->sort_d->lp;
				delete_list( &sort_d_list, lp );
			}

			if( LCAST(lpa, SORT)->web->sort_f ) { /* in F list too */
				lp = LCAST(lpa, SORT)->web->sort_f->lp;
				delete_list( &sort_f_list, lp );
			}

			lpa = lpa->next;
			if( lpa->datap == (LDATA *)NULL ) {
				more_a = FALSE;
				lpa = LNULL;
			} else {
				cur_a++;
			}
			continue;
		}
		
		if( d_weight >= a_weight && d_weight > f_weight )
		{ /* allocate a D register for lpd */
			cur_d->datap = lpd->datap;
			LCAST(cur_d, SORT)->regno = FIRST_DREG - MAX_DREG + n_dreg--;
			if( lpa && LCAST(lpa, SORT)->web == LCAST(lpd, SORT)->web )
			{ /* lpa will be deleted, so advance it first */
				lpa = lpa->next;
				if( lpa->datap == (LDATA *)NULL ) {
					more_a = FALSE;
					lpa = LNULL;
				}
			}
			if( lpf && LCAST(lpf, SORT)->web == LCAST(lpd, SORT)->web ) {
				lpf = lpf->next;
				if( lpf->datap == (LDATA *) NULL ) {
					more_f = FALSE;
					lpf = LNULL;
				}
			}

			if( LCAST(lpd, SORT)->web->sort_a ) { /* in A list too */
				lp = LCAST(lpd, SORT)->web->sort_a->lp;
				delete_list( &sort_a_list, lp );
			}
		
			if( LCAST(lpd, SORT)->web->sort_f ) { /* in F list too */
				lp = LCAST(lpd, SORT)->web->sort_f->lp;
				delete_list( &sort_f_list, lp );
			}
			
			lpd = lpd->next;
			if( lpd->datap == (LDATA *) NULL ) {
				more_d = FALSE;
				lpd = LNULL;
			} else {
				cur_d++;
			}
			continue;
		}

		if( f_weight >= a_weight && f_weight >= d_weight )
		{ /* allocate a F register for lpf */
			cur_f->datap = lpf->datap;
			if( use68881 ) {
				LCAST(cur_f, SORT)->regno = first_freg - max_freg + n_freg--;
			} else if( usefpa ) { /* start at fpa4 ends with fpa15 */
				LCAST(cur_f, SORT)->regno = first_freg + max_freg - n_freg--;
			}
			
			if( lpa && LCAST(lpa, SORT)->web == LCAST(lpf, SORT)->web )
			{ /* lpa will be deleted, so advance it first */
				lpa = lpa->next;
				if( lpa->datap == (LDATA *)NULL ) {
					more_a = FALSE;
					lpa = LNULL;
				}
			}
			if( lpd && LCAST(lpd, SORT)->web == LCAST(lpf, SORT)->web )
			{
				lpd = lpd->next;
				if( lpd->datap == (LDATA *) NULL ) {
					more_d = FALSE;
					lpd = LNULL;
				}
			}

			if( LCAST(lpf, SORT)->web->sort_a ) /* in A list too */
			{ /* take it out of the A list */
				lp = LCAST(lpf, SORT)->web->sort_a->lp;
				delete_list( &sort_a_list, lp );
			}
		
			if( LCAST(lpf, SORT)->web->sort_d ) /* in F list too */
			{ /* take it out of the D list */
				lp = LCAST(lpf, SORT)->web->sort_d->lp;
				delete_list( &sort_d_list, lp );
			}
			
			lpf = lpf->next;
			if( lpf->datap == (LDATA *) NULL ) {
				more_f = FALSE;
				lpf = LNULL;
			} else {
				cur_f++;
			}
			continue;
		}
	}

	/*
	 * set up regmask
	 */
	if( usefpa ) {
		if( fpa_base_reg == -1 ) {
			regmask = SET_REGMASK( n_freg, 0, 0, n_areg, n_dreg );
		} else {
			regmask = SET_REGMASK( n_freg, fpa_base_reg, 0, n_areg, n_dreg );
		}
	} else {
		regmask = SET_REGMASK( 0, 0, n_freg, n_areg, n_dreg );
	}
	
	/* the dominator information is not correct */
	/* for those new created pre_header blocks */
	find_dominators();

	modify_triples( reg_a, reg_d, reg_f );	

	if(SHOWCOOKED==TRUE)
	{
		dump_webs();
	}
	alloc_new_web(TRUE);
	empty_listq(reg_lq);
}

static int sequence;

void
resequence_triples()
{ /* put the tripleno of the triples that in a basic */
  /* block back into the lexical oreder */

	register BLOCK *bp;
	register TRIPLE *tp, *tp1;
	register LIST *lp, *waiting_list;      /* waiting_list is a list of triples
					        * that can not be resequenced
						* right now.
						*/
	fpa_base_weight = sequence = 0;
		
	for( bp = entry_block; bp; bp = bp->next ) {
		waiting_list = LNULL;
		TFOR( tp, bp->last_triple->tnext ) {
			if(tp->op == IMPLICITUSE || tp->op == ENTRYDEF || tp->op == EXITUSE) { 
				tp->tripleno = sequence++;
				continue;
			}

			if( tp->op == IMPLICITDEF ) { /* see if it has to waiting_list */
				if( ( (TRIPLE *)tp->right)->op != FCALL ) {
					tp->tripleno = sequence++;
					continue;
				} else {
					lp = NEWLIST( tmp_lq );
					(TRIPLE *)lp->datap = tp;
					LAPPEND( waiting_list, lp );
					continue;
				}
			}

			if( tp->op == SCALL ) {
				TFOR( tp1, (TRIPLE *)tp->right ) {
					resequence( tp1, bp );
				}
				resequence( tp->left, bp );
				tp->tripleno = sequence++;
				LFOR( lp, waiting_list ) {
					resequence( lp->datap, bp );
				}
				waiting_list = LNULL;
				continue;
			}

			if( ISOP(tp->op, ROOT_OP) ) {
				resequence( tp->left , bp);
						
				if( ISOP( tp->op, BIN_OP ) && 
						tp->right->operand.tag == ISTRIPLE ) {
					resequence( tp->right, bp );
				}
				if( ISBRANCHOP( tp->op ) ) { /* numbering the implicit_def first. */
					LFOR( lp, waiting_list ) {
						resequence( lp->datap, bp );
					}
					tp->tripleno = sequence++;
				} else {
					tp->tripleno = sequence++;
					LFOR( lp, waiting_list ) {
						resequence( lp->datap, bp );
					}
				}
				waiting_list = LNULL;
			}
		}
	}
	empty_listq( tmp_lq );
}

resequence( np, bp ) NODE *np; BLOCK *bp;
{
	register TRIPLE *tp, *tp1;

	if( np == (NODE *)NULL || np->operand.tag != ISTRIPLE )
		return;

	tp = (TRIPLE *)np;
	if( tp->op == FCALL ) {
		TFOR( tp1, (TRIPLE *)tp->right ) {
			resequence( tp1, bp );
		}
		resequence( tp->left, bp );
	}
	else {
		resequence( tp->left, bp );
		if( ISOP( tp->op, BIN_OP ) ) {
			resequence( tp->right, bp );
		}
	}
	if( usefpa ) {
		if( ISFLOAT(tp->type.tword ) ) {/* every reference counts for one cycle */
		
			fpa_base_weight += bp->loop_weight;
		} else {
			if( ISDOUBLE(tp->type.tword ) ) {
				fpa_base_weight + = bp->loop_weight << 1;
			}
		}
	}
	tp->tripleno = sequence++;
}
	
void 
build_du_web()
{ /*
   * Algorithm:
   *	Scan throught all the leaves, first, find out all the variables
   *	that can be a register variable.  For each interested variable,
   *	for each defination build a du-web and then merege those webs
   *	that have to be allocated in the same register or memory location.
   */

	LIST *lp1, *lp2, **lpp, *lp3;
	LEAF *leafp;
	VAR_REF *var_ref;
	BOOLEAN can_in_reg();
	void new_web(), delete_implicit();
	int old_nwebs, count;
	TRIPLE *tp;
	struct segdescr_st descr;

	reg_life_wordsize = ((unsigned)roundup(nblocks, BPW)) / BPW;
	/*
	 * Make a dummy list at the end of the web list.
	 */
	lp1 = NEWLIST(reg_lq);
	lp1->datap = (LDATA *)NULL;
	web_head = web_tail = lp1;
	nwebs = 1;

	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) 
	{ /* for each hash entry */
		LFOR( lp1, *lpp )
		{ /* for each leaf */
			
			leafp = (LEAF *) lp1->datap;
			if( ISCONST(leafp) || leafp->no_reg || leafp->overlap )
				continue;

			descr = leafp->val.addr.seg->descr;
			if( descr.class == HEAP_SEG ) /* dummy leaf */
				continue;

			if( !can_in_reg( leafp ) )
				continue;

			if( leafp->references == (VAR_REF*) NULL )
			/* never used */
				continue;

			old_nwebs = nwebs;
			for(var_ref = leafp->references; var_ref;
                              var_ref = var_ref->next_vref)
			{ /* make a web for each definition */
				if( var_ref->reftype == VAR_AVAIL_DEF ||
						var_ref->reftype == VAR_DEF )
					new_web( leafp, var_ref );
			}

			count = nwebs - old_nwebs;
			if( count > 1 )
				merge_same_var( count );
		}
	}
	web_head = web_tail; /* points to the dummpy tail element */
}

void
compute_interference()
{
	register LIST *lp1, *lp2;
	register WEB *web1, *web2;
	int reg_share_bitsize;
	BOOLEAN short_intf();
	void set_bit();

	reg_share_bitsize = roundup( nwebs, BPW );
	reg_share_wordsize = (unsigned)reg_share_bitsize / BPW;

	for( lp1 = web_head->next; lp1->datap != (LDATA *) NULL; lp1 = lp1->next )
	{
		web1 = LCAST(lp1, WEB);
		web1->can_share = (SET)ckalloc( reg_share_wordsize, sizeof(int) );

	}
	
	for( lp1 = web_head->next; lp1->datap != (LDATA *) NULL; lp1 = lp1->next )
	{
		web1 = LCAST(lp1, WEB);
		for( lp2 = web_head->next; lp2->datap != (LDATA *) NULL; lp2 = lp2->next )
		{
			if( lp1 >= lp2 ) /* same web or do it later*/
				continue;

			web2 = LCAST(lp2, WEB);
			if( web1->leaf == web2->leaf )
			/* two webs for the same leaf */
			/* should never interference to each other */
				continue;

			if( intfr(web1->life_span, web2->life_span ) )
			{
				if( web1->short_life && web2->short_life )
				{ /* do detail anaylyse */
					if( ! short_intf( web1, web2 ) )
						continue;
				}

				set_bit( web1->can_share, web2->web_no );
				set_bit( web2->can_share, web1->web_no );
			}
		}
	}

	for( lp1 = web_head->next; lp1->datap != (LDATA *) NULL; lp1 = lp1->next )
	{ /* two webs can share a register iff they are not */
	  /* interference to each other */
		reverse_can_share( (LCAST(lp1, WEB))->can_share  );
	}
}

void
weigh_du_web()
{ /* weight the webs and sort into two list*/
  /*  -- one for A one for D registers */

	LIST *lp1, *lp2;
	WEB *web;
	void weigh_web(), sort_webs();

	for( lp1 = web_head->next; lp1->datap != (LDATA *) NULL; lp1 = lp1->next )
	{
		web = LCAST(lp1,WEB);

		LFOR( lp2, web->define_at )
		{
			weigh_web( web, LCAST(lp2, VAR_REF) );
		}

		LFOR( lp2, web->use )
		{
			weigh_web( web, LCAST(lp2, VAR_REF) );
		}
	}
	sort_webs(); /* sort into a binary tree than put into a list */
}

void
modify_triples( reg_a, reg_d, reg_f ) LIST *reg_a, *reg_d, *reg_f;
{
	LIST *lp;
	WEB *web;
	LEAF *new_leaf, *old_leaf, *reg_leaf();
	int i, regno;
	void replace_leaves();

	for( i = 0, lp = reg_d; i < MAX_DREG && lp->datap; i++, lp++ )
	{ /* for all webs that have been allocated in D register. */
		for( web = LCAST( lp, SORT )->web; web; web = web->same_reg )
		{ /* for all the web in that D register */
			old_leaf = web->leaf;
			new_leaf = reg_leaf( old_leaf, DREG_SEGNO,
					LCAST( lp, SORT )->regno  );
			replace_leaves( web, new_leaf, old_leaf );
		}
	}

	for( i = 0, lp = reg_a; i < MAX_AREG && lp->datap; i++, lp++ )
	{ /* for all webs that be allocated in A register. */
		for( web = LCAST( lp, SORT )->web; web; web = web->same_reg )
		{ /* for all the web in that A register */
			old_leaf = web->leaf;
			new_leaf = reg_leaf( old_leaf, AREG_SEGNO,
						LCAST( lp, SORT)->regno );
			replace_leaves( web, new_leaf, old_leaf );
		}
	}

	for( i = 0, lp = reg_f; i < max_freg && lp->datap; i++, lp++ )
	{ /* for all webs that have been allocated in F register. */
		for( web = LCAST( lp, SORT )->web; web; web = web->same_reg )
		{ /* for all the web in that F register */
			old_leaf = web->leaf;
			new_leaf = reg_leaf( old_leaf, FREG_SEGNO,
					LCAST( lp, SORT )->regno );
			replace_leaves( web, new_leaf, old_leaf );
		}
	}
}

void
replace_leaves( web, new_leaf, old_leaf )
WEB *web;
LEAF *new_leaf, *old_leaf;
{
	LIST *lp;
	TRIPLE *tp, *triple, *append_triple();
	BLOCK *bp, *best_bp, *find_df_block();
	LEAF *tmp_leaf, *new_temp();
	
	LFOR( lp, web->define_at )
	{
		tp = LCAST( lp, VAR_REF )->site.tp;
		bp = LCAST( lp, VAR_REF )->site.bp;

		if( tp->op == ENTRYDEF)
		{ /* do pre-loading */
			best_bp = find_df_block( web );/* find best block to load */
			if( best_bp != bp && best_bp != (BLOCK *)NULL )
			{ /* other place than the ENTRYDEF */
			  /* do loading after first triple */
				tp = best_bp->last_triple->tnext;
			}
			triple = append_triple( tp, ASSIGN, new_leaf, old_leaf,
							old_leaf->type );
			if( bp->last_triple == tp )
				bp->last_triple = triple;
			continue;
		}

		/* else */
		if( tp->op == IMPLICITDEF )
		{
			if( ( (TRIPLE *)tp->right)->op == FCALL )
			{
				for( tp=(TRIPLE *)tp->right; ;tp=tp->tnext )
				{ /* find the next ROOT triple */
					if( (ISOP(tp->op,ROOT_OP) ) )
						break;
				}
				if( ISBRANCHOP( tp->op ) )
				{ /* assign the expression to a tmp */
					tmp_leaf = new_temp( tp->left );
					triple = append_triple( tp->tprev, ASSIGN, tmp_leaf,
								tp->left, tmp_leaf->type );
					tp->left = (NODE *)tmp_leaf;
					tp = triple;
				}
					
			}

			triple = append_triple( tp, ASSIGN, new_leaf, 
						old_leaf, old_leaf->type );
			if( bp->last_triple == tp )
				bp->last_triple = triple;
			continue;
		}
		/* else */
		if( tp->left == (NODE *)old_leaf )
			tp->left = (NODE *)new_leaf;
		if( tp->right == (NODE *)old_leaf )
			tp->right = (NODE *)new_leaf;
	}

	LFOR( lp, web->use )
	{
		tp = LCAST(lp, VAR_REF)->site.tp;
		bp = LCAST(lp, VAR_REF)->site.bp;

		if( tp->op == IMPLICITUSE || tp->op == EXITUSE )
		{ /* store it back into memory */
			triple = append_triple( tp, 
						ASSIGN, old_leaf, new_leaf,
							old_leaf->type );
			if( bp->last_triple == tp )
				bp->last_triple = triple;
			continue;
		}

		/* else */
		if( tp->left == (NODE *)old_leaf )
			tp->left = (NODE *)new_leaf;
		if( tp->right == (NODE *)old_leaf )
			tp->right = (NODE *)new_leaf;
	}
}

void
share_registers()
{ /*
   * merge those webs that can share the same registers.
   */

	BOOLEAN merge_sorts();
	void re_order();
	LIST *lpa, *lpd, *lpf, *lp_next;
	SORT *sp;
	int a_weight, d_weight, f_weight;
	
	lpa = sort_a_list ? sort_a_list->next : LNULL;
	lpd = sort_d_list ? sort_d_list->next : LNULL;
	lpf = sort_f_list ? sort_f_list->next : LNULL;

	while( (lpa && lpa->datap) || (lpd && lpd->datap ) || (lpf && lpf->datap) )
	{
		f_weight = a_weight = d_weight = 0;
		
		while( lpa && lpa->datap && a_weight == 0 ) /* more webs in reg A list */
		{
			sp = LCAST(lpa, SORT);
			if( sp->weight == 0 )
			{ /* visited */
				lpa = lpa->next;
				continue;
			}
			a_weight = sp->weight;
		}

		while( lpd && lpd->datap && d_weight == 0 ) /* more webs in D register list */
		{
			sp = LCAST(lpd, SORT);
			if( sp->weight == 0 ) 
			{ /* already visited */
				lpd = lpd->next;
				continue;
			}
			d_weight = sp->weight;
		}
		
		while( lpf && lpf->datap && f_weight == 0 ) /* more webs in F register list */
		{
			sp = LCAST(lpf, SORT);
			if( sp->weight == 0 ) 
			{ /* already visited */
				lpf = lpf->next;
				continue;
			}
			f_weight = sp->weight;
		}
		
		if( a_weight == 0 && d_weight == 0 && f_weight == 0 ) {/* done */
			break;
		}
			
		if( a_weight > d_weight && a_weight > f_weight )
		{ /* merge A list until no more sharing can be done */
			while( merge_sorts( lpa ) ){};

			/* fix the order list. */
			lp_next = lpa->next;
			re_order( lpa, &sort_a_list );
			lpa = lp_next;
		}
		else
		{
			if( d_weight > f_weight && d_weight >= a_weight )
			{ /* merge D list until no more sharing can be done */
				while( merge_sorts( lpd ) ){};			

				lp_next = lpd->next;
				re_order( lpd, &sort_d_list );
				lpd = lp_next;
			}
			else
			{ /* merge F list */
				while( merge_sorts( lpf ) ) {};

				lp_next = lpf->next;
				re_order( lpf, &sort_f_list);
				lpf = lp_next;
			}
		}
	}
}

