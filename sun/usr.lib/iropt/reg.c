#ifndef lint
static	char sccsid[] = "@(#)reg.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * This file contains the unitily routines for doing register allocation.
 */

#include "iropt.h"
#include <stdio.h>
#include <ctype.h>
#include "reg.h"
#define TWOTO31_1	2147483647;	/*  2 ** 31 - 1 */

int nwebs = 0;
int n_dsort = 0;
int n_asort = 0;
int n_fsort = 0;
int alloc_web= 0;
int fpa_base_reg = -1;
int fpa_base_weight = 0;
LIST *web_head = LNULL;
LIST *web_tail = LNULL;
LIST *sort_d_list = LNULL;
LIST *sort_a_list = LNULL;
LIST *sort_f_list = LNULL;
WEB *free_web;
extern int nleaves;
extern LISTQ *reg_lq;
extern BOOLEAN partial_opt;

char *dreg_name[] = {
	"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7"
};

char *areg_name[] = {
	"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7"
};

char *freg_name[] = {
	"F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7"
};

char *fpareg_name[] = {
	"FPA0", "FPA1", "FPA2", "FPA3", "FPA4", "FPA5", "FPA6", "FPA7",
	"FPA8", "FPA9", "FPA10", "FPA11", "FPA12", "FPA13", "FPA14", "FPA15"
};

reg_init()
{
	nwebs = n_dsort = n_asort = n_fsort = alloc_web = 0;
	fpa_base_reg = -1;
	sort_d_list = sort_a_list = sort_f_list = web_head = web_tail = LNULL;
	if(reg_lq == NULL) reg_lq = new_listq();

}

BOOLEAN
can_in_reg( leafp ) LEAF *leafp;
{
	int size, type_size();
	struct segdescr_st descr;

	if( (ISFTN( leafp->type.tword ) ) || (ISARY( leafp->type.tword ) ) )
	/* no function or array */
		return( FALSE );


	if( partial_opt ) {
		descr = leafp->val.addr.seg->descr;
		if( descr.external == EXTSTG_SEG || descr.class == HEAP_SEG ) {
			return FALSE;
		}
	}
	
	if( ( use68881 || usefpa ) && ( ISREAL( leafp->type.tword ) ) )
		return( TRUE );

	size = type_size( leafp->type );
	if( size > 4 || size <= 1 )
	/* no double or complex or character */
		return( FALSE );

	/* else */
	return ( TRUE );
}

void
new_web(leafp, rp) LEAF *leafp; VAR_REF *rp;
{ /* This routine will build a new web but it will
   * do the checking see if it is a potential
   * register web or not
   */

	WEB *web, *alloc_new_web();
	register LIST *lp;
	register TRIPLE *tp1, *tp2;
	void compute_life_span();

	tp1 = rp->site.tp;
	if( tp1->op == ENTRYDEF || tp1->op == IMPLICITDEF )
	{
		LFOR( lp, tp1->canreach )
		{
			tp2 = LCAST( lp, VAR_REF )->site.tp;
			if( tp2->op != EXITUSE && tp2->op != IMPLICITUSE )
				goto do_it;
			/*  else */
			if( tp2->reachdef1 != tp2->reachdef1->next )
			/* there are some other definition can reach here */
				goto do_it;
		}
		/* do NOT even creat web for it */
		return;
	}

do_it:
	web = alloc_new_web(FALSE);
	lp = NEWLIST(reg_lq);
	(WEB *)lp->datap = web;
	/*
	 * For a single link list, build the
	 * list in the reverse order of the
	 * LAPPEND.
	 */
	
	lp->next = web_head;
	web_head = web_tail->next = lp;

	web->web_no = nwebs++;
	web->leaf = leafp;
	web->define_at = NEWLIST(reg_lq);
	(VAR_REF *)web->define_at->datap = rp;
	web->use = tp1->canreach;
	web->d_cont = web->a_cont = web->f_cont = 0;
	web->same_reg = (WEB *)NULL;
	web->sort_d = web->sort_a = web->sort_f = (SORT *)NULL;
	web->import = ( tp1->op == ENTRYDEF ) ? TRUE : FALSE;
	web->short_life = TRUE;

	compute_life_span( web );
}

void
compute_life_span( web ) WEB *web;
{
	LIST *lp1,*lp2, *reachdef;
	int used_in_block;
	BLOCK *bp;
	TRIPLE *tp;
	void set_bit(), set_pred();

	web->life_span = (SET)ckalloc( 1, reg_life_wordsize*sizeof(int) );

	bp = ((VAR_REF *)web->define_at->datap)->site.bp;
	set_bit( web->life_span, bp->blockno );
	LFOR(lp1, web->use)
	{
		tp = LCAST(lp1,VAR_REF)->site.tp;
		if( tp->left == (NODE *)web->leaf )
		/* used on the lift hand side */
			reachdef = tp->reachdef1;
		else
			reachdef = tp->reachdef2;

		if( reachdef && reachdef == reachdef->next &&
				LCAST(lp1, VAR_REF)->site.bp == bp )
		{ /* live in one block only */
			continue;
		}

		LFOR( lp2, reachdef )
		{ /* for each definition can reaches here */
			set_bit( web->life_span,
				LCAST(lp2, VAR_REF)->site.bp->blockno );
		}

		used_in_block = LCAST(lp1, VAR_REF)->site.bp->blockno;
		set_bit( web->life_span, used_in_block);
		set_pred( web->life_span, LCAST(lp1,VAR_REF)->site.bp );
		web->short_life = FALSE;
	}
}

/*
 * replaced by asm routine in bit_util.s
 *
 * void
 * set_bit( set, bitno ) register SET set; register int bitno;
 * {
 *	register int word_index, bit_offset;
 *
 *	word_index = (unsigned)bitno / BPW;
 *	bit_offset = (unsigned)bitno % BPW;
 *	set[word_index] |= (1<<bit_offset);
 * }
 *
 *
 * BOOLEAN
 * test_bit( set, bitno ) register SET set; register int bitno;
 * {
 *	register int word_index, bit_offset;
 *
 *	word_index = (unsigned)bitno / BPW;
 *	bit_offset = (unsigned)bitno % BPW;
 *	if( set[word_index] & (1<<bit_offset) )
 *		return( TRUE );
 *	else
 *		return( FALSE );
 * }
 */

void
set_pred( set,  bp ) SET set; BLOCK *bp;
{
	register LIST *lp, *lp1;
	register int bitno;
	void set_bit();
	BOOLEAN test_bit();

	if( bp == (BLOCK *)NULL ) /* pred of entry block */
		return;

	lp1 = bp->pred;
	
	LFOR( lp, lp1 )
	{
		bitno = LCAST(lp, BLOCK)->blockno;
		if( test_bit( set, bitno ) ) /* already visited */
			continue;

		set_bit( set, bitno );
		set_pred( set, lp->datap );
	}
}

void
merge_same_var( count ) int count;
{ /* 
   * Merge the last COUNT of webs if possible.
   * The last COUNT of webs represent the webs
   * for the same variable.
   */

	register LIST *lp1, *lp2;
	register int i,j;
	register LIST *tmp_l;
	BOOLEAN change, must_merge(), intfr();
	void merge_webs();

	for( i = 0,lp1 = web_head; i < count; lp1 = lp1->next,i++ ) {
		if( LCAST(lp1, WEB)->short_life )
			continue;
		do {
			change = FALSE;
			for(j = i+1,lp2 = lp1->next; j < count; lp2 = lp2->next, j++) {
				if( LCAST(lp2, WEB)->short_life )
					continue;

				if( intfr(LCAST(lp1, WEB)->life_span, LCAST(lp2, WEB)->life_span) &&
						must_merge(LCAST(lp1, WEB), LCAST(lp2, WEB)) )
				{
					merge_webs( lp1, lp2 ); /* delete lp2 */
					--count;
					change = TRUE;
					break;
				}
			}
		} while( change );
	}
}

BOOLEAN
must_merge( web1, web2 ) WEB *web1, *web2;
{ /*
   * This routine test to see if web1 and web2 have to be
   * allocated for same resource.  It will return true iff
   * anyone of the usage of of web1 can be reached by the
   * web2"s definition (or vis versa).
   */
	register LIST *lp0, *lp1, *lp2, *lp3;
	register LDATA *define;

	LFOR( lp0, web2->define_at )
	{
		define = lp0->datap;
	   	LFOR( lp1, web1->use )
		{
			if( LCAST(lp1, VAR_REF)->reftype == VAR_EXP_USE1 ||
					LCAST(lp1, VAR_REF)->reftype == VAR_USE1 )
				lp2 = LCAST(lp1, VAR_REF)->site.tp->reachdef1;
			else
				lp2 = LCAST(lp1, VAR_REF)->site.tp->reachdef2;
			
			LFOR( lp3, lp2 )
			{
				if( lp3->datap == define ) /* define canreachs web1s use */
					return TRUE;
			}
		}
	}
	return FALSE;
}

BOOLEAN
intfr( set1, set2 ) register SET set1, set2;
{ /*
   * see if two webs point by lp1 and lp2 interference each other.
   */
	register int i;

	for( i = reg_life_wordsize; i; i-- )
	{
		if( *set1++ & *set2++ )
			return( TRUE );
	}
	
	return( FALSE );		
}

void
merge_webs( lp1, lp2 ) LIST *lp1, *lp2;
{ /* merge two webs point by lp1 and lp2 */

	WEB *web1, *web2;
	register int i;
	SET set1,set2;
	LIST *merge_lists(), *order_list();
	void delete_list();

	web1 = (WEB *)lp1->datap;
	web2 = (WEB *)lp2->datap;
	LAPPEND( web1->define_at, web2->define_at );
	web1->use = merge_lists( order_list( web1->use, reg_lq ), 
					order_list( web2->use, reg_lq ) );
	web1->short_life = FALSE;
	if( web2->import )
		web1->import = TRUE;
	set1 = web1->life_span;
	set2 = web2->life_span;
	for( i = reg_life_wordsize; i; i-- )
	{
		*set1++ |= *set2++;
	}
	if( lp2 == web_head ) {
		web_head = web_head->next;
	}
	delete_list( &web_head, lp2 );
}

void
dump_webs() {
	register LIST *lp;
	void print_web();

	if( n_dsort ) {
		printf( "\nDUMP WEBS for register d: use %d D REGISTERS\n",
					MAX_DREG - n_dreg );
		for( lp =  sort_d_list->next; lp->datap != (LDATA *)NULL; lp = lp->next ) {
			print_web( lp, D_TREE );
		}
	}
	
	if( n_asort ) {
		printf( "\nDUMP WEBS for register a: use %d A REGISTERS\n",
					MAX_AREG - n_areg );
		for( lp = sort_a_list->next; lp->datap != (LDATA *)NULL; lp = lp->next ) {
			print_web( lp, A_TREE );
		}

		if( usefpa && fpa_base_reg != -1 ) {
			printf( "\n\t**** FPA_BASE_REG %s *** with %d LB",
					areg_name[fpa_base_reg], fpa_base_weight );
		}
	}

	if( n_fsort ) {
		printf( "\nDUMP WEBS for register f: use %d F REGISTERS\n",
					max_freg - n_freg );
		for( lp = sort_f_list->next; lp->datap != (LDATA *)NULL; lp = lp->next ) {
			print_web( lp, F_TREE );
		}
	}
}

void
print_web( lp, which_tree ) LIST *lp; TREE which_tree;
{
	WEB *web, *same_web;
	LIST *lp1, *lp2;
	int i;
	SORT *sp;

	sp = LCAST(lp, SORT);

	if( sp->weight == 0 )
	/* not a independent web anymore */
		return;

	printf( "\n" );
	printf( "\tMERGED_WEBS: ******weight %d ", sp->weight );
	if( sp->regno ) {
		if( which_tree == D_TREE ) {
			printf( ": IN REG %s***********\n", dreg_name[sp->regno] );
		} else {
			if( which_tree == A_TREE ) {
				printf( ": IN REG %s***********\n", areg_name[sp->regno] );
			} else {
				printf( ": IN REG %s***********\n",
					usefpa ?
					fpareg_name[sp->regno] : freg_name[sp->regno] );
			}
		}
	}
	else
		printf( ": NOT IN REGISTER******\n" );
	for(web = LCAST(lp, SORT)->web; web; web = web->same_reg)
	{
		if( web->short_life )
			printf( "\t\tSHORT" );
		printf( "\t\tWEB[%d]: for leaf[%d] %s\n", web->web_no, 
						web->leaf->leafno,
				web->leaf->pass1_id ? web->leaf->pass1_id : "\"\"" );
		printf( "\t\t\tDEFINE: " );
		if( web->import )
			printf( "IMPORT" );
		else
		{
			LFOR( lp2, web->define_at )
			{
				printf( "B[%d] T[%d],",
					LCAST(lp2,VAR_REF)->site.bp->blockno,
					LCAST(lp2,VAR_REF)->site.tp->tripleno);
			}
		}
		printf( "\n\t\t\tUSE: " );
		LFOR( lp2, web->use )
		{
			printf( "B[%d] T[%d],",
					LCAST(lp2,VAR_REF)->site.bp->blockno,
					LCAST(lp2,VAR_REF)->site.tp->tripleno);
		}
		printf( "\n\t\t\tLIFE_SPAN: " );
		for( i = 0; i < nblocks; i++ )
		{
			if( test_bit( web->life_span, i ) )
			{
				printf( "B[%d], ", i );
			}
		}
		printf( "\n\t\t\tWEIGHT: " );
		printf( "for A register %dLB, for D register %dLB, for F register %dLB",
					web->a_cont, web->d_cont, web->f_cont );
		if( web->sort_a )
			printf( "\n\t\t\tIN A LIST" );
		else
			printf( "\n\t\t\t**NOT** IN A LIST" );

		if( web->sort_d )
			printf( "\n\t\t\tIN D LIST" );
		else
			printf( "\n\t\t\t**NOT** IN D LIST" );

		if( web->sort_f )
			printf( "\n\t\t\tIN F LIST" );
		else
			printf( "\n\t\t\t**NOT** IN F LIST" );

		printf( "\n\t\t\tCAN SAHRE WITH: " );
		for( i = 0; i < nwebs-1; i++ )
		{
			if( test_bit( web->can_share, i ) )
			{
				printf( "WEB[%d], ", i );
			}
		}
		printf( "\n\n" );
	}
	printf( "\n\t\**********************************\n" );
}

BOOLEAN
short_intf( web1, web2 ) register WEB *web1, *web2;
{ /* 
   * both web1 and web2 are short_life webs, that means
   * all the definitions and uses are in the same single basic block
   * they are interference with each other if the life spans are
   * cross to each other.  This routine is assuming the web->use
   * is point to the LAST usage. ( depend on the order of doing CANREACH
   * and the order of doing COPY_LIST ).  Since, the triplenos are sorted
   * in lexical order, so we can tell the order by look at the triple no.
   * For a short_life web, the tripleno of the def alway <= use.
   */

	register int df1, use1, df2, use2;

	/* all the short_life web have only one definition point */
	df1 = LCAST(web1->define_at, VAR_REF)->site.tp->tripleno;
	df2 = LCAST(web2->define_at, VAR_REF)->site.tp->tripleno;

	if( web1->use == LNULL )
	{
		if( web2->use == LNULL )
		{ /* no use at all */
			return( FALSE );
		}

		/* else */
		use2 = LCAST(web2->use, VAR_REF)->site.tp->tripleno;

		if( df1 >= df2 && df1 < use2 )
			return( TRUE );

		else
			return( FALSE );
	}

	/* else, web->use1 != LNULL */
	use1 = LCAST(web1->use, VAR_REF)->site.tp->tripleno;
	if( web2->use == LNULL )
	{
		if( df2 >= df1 && df2 < use1 )
			return( TRUE );

		else
			return( FALSE );
	}

	/* web1->use1 != LNULL && web2->use2 != LNULL */
	use2 = LCAST(web2->use, VAR_REF)->site.tp->tripleno;
	if( df1 > use2 )
	{ /* make sure df1 is not in the way of use2 and his root triple */
		register TRIPLE *tp;

		tp = LCAST(web2->use, VAR_REF)->site.tp;
		if( tp->op == PARAM )
			tp = (TRIPLE *)tp->right;

		/* find the root triples */
		for( ; ! ISOP( tp->op, ROOT_OP ) || tp->tripleno <= use2; tp = tp->tnext ) {}

		if( df1 > tp->tripleno )
			return ( FALSE );
		else
			return ( TRUE );
	}

	if( df2 > use1 )
	{ /* make sure df2 is not in the way of use1 and his root triple */
		register TRIPLE *tp;

		tp = LCAST(web1->use, VAR_REF)->site.tp;
		if( tp->op == PARAM )
			tp = (TRIPLE *)tp->right;
		/* find the root triples */
		for( ; ! ISOP( tp->op, ROOT_OP ) || tp->tripleno <= use1 ; tp = tp->tnext ) {}

		if( df2 > tp->tripleno )
			return ( FALSE );
		else
			return ( TRUE );
	}

	return( TRUE );
		
}

reverse_can_share( can_share ) SET can_share;
{
	register int i;

	for( i = reg_share_wordsize; i; i-- )
	{
		*can_share++ = ~(*can_share);
	}
}

void
weigh_web( web, vp ) WEB *web; VAR_REF *vp;
{
	register TRIPLE *tp;
	register BLOCK *bp;

	tp = vp->site.tp;
	bp = vp->site.bp;
	
	if( ( ISREAL( web->leaf->type.tword ) ) && ( use68881 || usefpa ) ) {
		web->f_cont += bp->loop_weight*(op_descr[((int)tp->op)].f_weight);
		web->a_cont = web->d_cont = 0;
	} else {
		web->f_cont = 0;
		if( vp->reftype == VAR_USE2 || vp->reftype == VAR_EXP_USE2 ) { 
			web->d_cont += bp->loop_weight*(op_descr[((int)tp->op)].right.d_weight);
			web->a_cont += bp->loop_weight*(op_descr[((int)tp->op)].right.a_weight);
		} else { /* use on left handside or define. */
			web->d_cont += bp->loop_weight*(op_descr[((int)tp->op)].left.d_weight);
			web->a_cont += bp->loop_weight*(op_descr[((int)tp->op)].left.a_weight);
		}
	}
}

void 
sort_webs()
{ /* using the binary tree to sort the webs */
  /* base on the wight for A and D register */

	LIST *lp;
	WEB *web;
	SORT *sort_a, *sort_d, *sort_f, *sort_d_root, *sort_a_root, *sort_f_root;
	void insert_tree(), listed();

	sort_d_root = sort_a_root = sort_f_root = (SORT *)NULL;

	for( lp = web_head->next; lp->datap != (LDATA *) NULL; lp = lp->next ) {
		web = LCAST(lp, WEB);
		if( web->d_cont > 0 ) {
			++n_dsort;
			web->sort_d = sort_d = (SORT *)ckalloc(1, sizeof(SORT));
			sort_d->weight = web->d_cont;
			sort_d->web = web;
			sort_d->left = sort_d->right = (SORT *)NULL;
			insert_tree( &sort_d_root, sort_d );
		} else {
			web->sort_d = (SORT *)NULL;
		}

		if( web->a_cont > 0 ) {
			++n_asort;
			web->sort_a = sort_a = (SORT *)ckalloc(1, sizeof(SORT));
			sort_a->weight = web->a_cont;
			sort_a->web = web;
			sort_a->left = sort_a->right = (SORT *)NULL;
			insert_tree( &sort_a_root, sort_a );
		} else {
			web->sort_a = (SORT *)NULL;
		}

		if( web->f_cont > 0 ) {
			++n_fsort;
			web->sort_f = sort_f = (SORT *)ckalloc(1, sizeof(SORT));
			sort_f->weight = web->f_cont;
			sort_f->web = web;
			sort_f->left = sort_f->right = (SORT *)NULL;
			insert_tree( &sort_f_root, sort_f );
		} else {
			web->sort_f = (SORT *)NULL;
		}
	}

	/* put the tree into list in the order of */
	/* the heaviest first */

	if( n_dsort )
	{
		listed( sort_d_root, &sort_d_list );
		/*
		 * At this point, sort_d_list points to the lightest
		 * web in the a list. Append a dummy list at the end
		 * to mark the end and let the sort_list points to it.
		 */
		lp = NEWLIST( reg_lq );
		lp->datap = (LDATA *)NULL;
		LAPPEND( sort_d_list, lp );
	}

	if( n_asort )
	{
		listed( sort_a_root, &sort_a_list );
		lp = NEWLIST( reg_lq );
		lp->datap = (LDATA *)NULL;
		LAPPEND( sort_a_list, lp );
	}

	if( n_fsort )
	{
		listed( sort_f_root, &sort_f_list );
		lp = NEWLIST( reg_lq );
		lp->datap = (LDATA *)NULL;
		LAPPEND( sort_f_list, lp );
	}
}

void
insert_tree( head, sort_el ) 
SORT **head, *sort_el;
{ /* put the sort_el into the tree pointed by head at right position */


	register SORT *cur_p, *parent_p;

	if( *head == (SORT *)NULL )
	{
		*head = sort_el;
		return;
	}

	for( cur_p= *head; cur_p; ) {
		parent_p = cur_p;
		if( cur_p->weight > sort_el->weight )
			cur_p = cur_p->left;
		else
			cur_p = cur_p->right;
	}
	/* found it */

	if( parent_p->weight > sort_el->weight )
		parent_p->left = sort_el;
	else
		parent_p->right = sort_el;
}

void
listed( tree, sort_list ) SORT *tree; LIST **sort_list;
{
	register LIST *lp;

	if( tree == (SORT *)NULL ) {
		return;
	}
	
	listed( tree->right, sort_list );

	/* put the tree into the list*/
	lp = NEWLIST(reg_lq);
	lp->datap = (LDATA *) tree;
	tree->lp = lp;
	LAPPEND( *sort_list, lp );
	listed( tree->left, sort_list );
}

BOOLEAN
merge_sorts( lp ) LIST *lp;
{
	BOOLEAN test_bit(), merge_can_share();
	WEB *base_web, *cur_web;
	LIST *tmp_lp;
	SORT *sp;

	base_web = LCAST(lp,SORT)->web;
	
	/* search for can share starting at next web */
	for( tmp_lp = lp->next; tmp_lp->datap; tmp_lp = tmp_lp->next)
	{
		if( (LCAST(tmp_lp, SORT))->weight == 0 )
			continue;

		cur_web = (LCAST(tmp_lp, SORT))->web;
		if( test_bit( base_web->can_share, cur_web->web_no ) )
		{ /* can share same register */
			if( sp = base_web->sort_a ) {
				sp->weight += cur_web->a_cont;
				if( sp->weight <= 0 ) { /* OVERFLOW */
					sp->weight = TWOTO31_1;
				}
			}

			if( sp = base_web->sort_d ) {
				sp->weight += cur_web->d_cont;
				if( sp->weight <= 0 ) { /* OVERFLOW */
					sp->weight = TWOTO31_1;
				}
			}

			if( sp = base_web->sort_f ) {
				sp->weight += cur_web->f_cont;
				if( sp->weight <= 0 ) { /* OVERFLOW */
					sp->weight = TWOTO31_1;
				}
			}

			if( cur_web->sort_a ) /* mark vistied */
				cur_web->sort_a->weight = 0;
			if( cur_web->sort_d )
				cur_web->sort_d->weight = 0;
			if( cur_web->sort_f )
				cur_web->sort_f->weight = 0;

			cur_web->same_reg = base_web->same_reg;
			base_web->same_reg = cur_web;
			base_web->short_life = cur_web->short_life = FALSE;
			return( merge_can_share( base_web->can_share, 
							cur_web->can_share ) );
		}
	}
	/* can NOT share with any web */
	return( FALSE );
}

BOOLEAN
merge_can_share( base_can_share, web_can_share ) 
	register SET base_can_share, web_can_share;
{
	register int i;
	register BOOLEAN more = FALSE;

	for( i = reg_share_wordsize; i; i-- )
	{
		if( *(base_can_share++) &= *(web_can_share++ ))
			more = TRUE;
	}

	return( more );
}

void
re_order( lp, lpp ) LIST *lp, **lpp;
{ /* after merge, the weight will only be increase. */
  /* so the lp can only go up in the list but never go down */

	register LIST *search_lp, *search_lp_prev;
	register SORT *cur_sp, *search_sp;
	register int cur_weight, search_weight;
	LIST *sort_list;

	cur_sp = LCAST(lp, SORT);
	if( lp == (*lpp)->next )
		return;

	sort_list = *lpp;
	cur_weight = cur_sp->weight;
	
	search_lp_prev = sort_list;
	
	for( search_lp = sort_list->next; search_lp != lp;
			search_lp_prev = search_lp, search_lp = search_lp->next )
	{
		search_sp = LCAST(search_lp, SORT);
		if( search_sp->weight == 0 ) {
			continue;
		}
		search_weight = search_sp->weight;
		if( search_weight < cur_weight )
		{ /* found the right spot for lp */
		/* take lp out of the sort list first. */
			delete_list( lpp, lp );
			lp->next = lp;
			LAPPEND( search_lp_prev, lp );
			return;
		}
	}
}

BLOCK *
find_df_block( web ) WEB *web;
{ /*
   * find the "best" block to do the initial loading
   * for this import web.  The "correct" blocks to do
   * the initial loading are the blocks that dominate
   * all the definition and usage points.  The "best"
   * block to do it is the "nearest" one amount all
   * the "correct" blocks and the loop_weight is 1.
   * Since, this web is IMPORT, so there is at least
   * the entry block will be the "best" block for it.
   * return NULL in MULTIPLE-ENTRY case.
   */

	LIST *lp;
	BLOCK *bp, *df_block;
	BOOLEAN test_bit(), dominates();

	df_block = (BLOCK *)NULL;
	for( bp = entry_block; bp; bp = bp->next )
	{
		if( bp->loop_weight > 1 )
		{ /* dont do the initial loading inside a loop */
			continue;
		}

		if( !test_bit( web->life_span, bp->blockno ) )
		{ /* this block is not in the webs life span */
			continue;
		}

		LFOR( lp, web->define_at )
		{
			if( LCAST( lp, VAR_REF )->site.tp->op == ENTRYDEF )
			/* import dummy definition */
				continue;

			if( !dominates(bp->blockno,LCAST(lp,VAR_REF)->site.bp->blockno) )
				goto next;
		}

		LFOR( lp, web->use )
		{
			if( !dominates(bp->blockno, LCAST(lp,VAR_REF)->site.bp->blockno) )
				goto next;
		}

		/* bp dominates all the occurences of the web */

		if( df_block == (BLOCK *)NULL )
			df_block = bp;

		else
		{ /* both bp and df_block are "correct" block */
		  /* see which one is better */
			if( dominates(df_block->blockno, bp->blockno ) )
			{ /* bp is better */
				df_block = bp;
			}
			/* else df_block is a better fit */
		}
	next: /* continue */;
	}
	return( df_block );
}

