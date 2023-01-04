#ifndef lint
static	char sccsid[] = "@(#)misc.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include <stdio.h>

TRIPLE *free_triple_lifo;
extern BLOCK * last_block;
extern LISTQ *proc_lq;

union leaf_location{
	ADDRESS *ap;
	struct constant *constp;
};
extern LEAF *leaf_top;

LIST *copy_list();
BOOLEAN same_irtype();
void unlist();

LOCAL void
unlist_triple(tp)
register TRIPLE *tp;
{
	void fix_del_reachdef(), fix_canreach();

	if( tp == TNULL)
		return;
	if( tp->tprev )
		tp->tprev->tnext = tp->tnext;
	if( tp->tnext )
		tp->tnext->tprev = tp->tprev;
	if( tp->canreach )
	{
		fix_canreach( tp, tp->canreach );
		tp->canreach  = LNULL;
	}
	if( tp->reachdef1 )
		fix_del_reachdef( tp, tp->reachdef1, TNULL );
	if( tp->reachdef2 )
		fix_del_reachdef( tp, tp->reachdef2, TNULL );
	tp->left = (NODE *)free_triple_lifo;
	free_triple_lifo = tp;

}	

TRIPLE *
append_triple(after,op,arg1,arg2,type)
TRIPLE *after;
IR_OP op;
NODE *arg1, *arg2;
TYPE type;
{
register TRIPLE *tp;
register LIST *lp;



	if(! free_triple_lifo) {
		tp = (TRIPLE*) new_triple(FALSE);
	} else {
		tp=free_triple_lifo;
		free_triple_lifo = (TRIPLE*) tp->left;
	}
	bzero(tp,sizeof(TRIPLE));

	tp->tprev = tp->tnext = tp;
	tp->tag = ISTRIPLE;
	tp->op = op;
	tp->left =  arg1;
	tp->right =  arg2;
	tp->type = type;
	tp->tripleno = ntriples++;
	if(after == TNULL) {
		tp->tprev = tp->tnext = tp;
	} else {
		TAPPEND(after,tp);
	}
	return(tp);
}

new_label()
{
static label_counter = 77000;
	return(++label_counter);
}

void
move_block(after,block)
BLOCK *after,*block;
{
	/*
	 ** ensure that a block appears in a given place in "next", ie code
	 ** generation, order. Done to keep unreachable code where
	 **	people expect it to be
	 */
	register BLOCK *bp = NULL;
	
	if(block == (BLOCK*) NULL || after == (BLOCK*) NULL) return;

	for(bp=entry_block;bp;bp = bp->next) { /* block cannot be entry_block */
		if(bp->next == block) break;
	}
	bp->next = block->next;
	block->next = after->next;
	after->next = block;
	if(bp->next == (BLOCK*) NULL) {
		last_block = bp;
	}
}

BLOCK *
new_block()
{
	BLOCK *bp;
	
	bp = (BLOCK*) ckalloc(1,sizeof(BLOCK));
	bp->blockno = nblocks++;
	/* is this the first block request for this ir_file ? */
	if(last_block != (BLOCK*) NULL) {		
		last_block->next = bp;
	}
	last_block = bp;
	bp->tag=ISBLOCK;
	bp->loop_weight = 1;
	return(bp);
}

LIST *
copy_list(tail,lqp) 
register LIST *tail;
register LISTQ *lqp;
{ /* Because the external pointer always points to the */
  /* "tail", so do the copy list and return the tail pointer */

register LIST *lp1, *lp2, *last;

		if( tail == LNULL )
			return( LNULL );

		last = LNULL;
		LFOR(lp1,tail->next) {
			lp2=NEWLIST(lqp);
			lp2->datap = lp1->datap;
			LAPPEND(last,lp2);
		}
		return(last);
}


LIST *
order_list(tail,lqp) 
register LIST *tail;
LISTQ *lqp;
{ /*  a variant of copy list that ensures the new list is
  **  sorted in increasing order and duplicates are deleted
  */

register LIST *lp1, *lp2;
LIST *last;
BOOLEAN insert_list();

	if( tail == LNULL )
		return( LNULL );

	last = LNULL;
	LFOR(lp1,tail->next) {
		if( last == LNULL ) {
			last = NEWLIST(lqp);
			last->datap = lp1->datap;
		} else if( last->datap < lp1->datap ) {
			lp2=NEWLIST(lqp);
			lp2->datap = lp1->datap;
			LAPPEND(last, lp2);
		} else {
			(void) insert_list(&last, lp1->datap, lqp);
		}
	}
	return(last);
}

/* 
**	merge two lists sorted by datap value; the args are assumed to point to the
**	largest (last) elements, and the largest element of the resulting merged list
**	is the value returned. both argument lists are distroyed.
*/
LIST *
merge_lists(list1,list2)
register LIST *list1,*list2;
{
		
/* cursors*/
register LIST *lp1 = LNULL, *lp2 = LNULL, *select = LNULL, *merged = LNULL; 

		/* the usual klutziness about advancing to smallest*/
	if(list1) {
		lp1 = list1->next;
	} else {
		return( list2 );
	}

	if(list2) {
		lp2 = list2->next;
	} else {
		return( list1 );
	}
	
	while(lp1 || lp2) {
		if(lp1 == LNULL) {
			select = lp2;
			lp2 = ( lp2->next == list2->next ? LNULL : lp2->next);
		} else if(lp2 == LNULL) {
			select = lp1;
			lp1 = ( lp1->next == list1->next ? LNULL : lp1->next);
		} else if(lp1->datap < lp2->datap) {
			select = lp1;
			lp1 = ( lp1->next == list1->next ? LNULL : lp1->next);
		} else if(lp1->datap > lp2->datap) {
			select = lp2;
			lp2 = ( lp2->next == list2->next ? LNULL : lp2->next);
		} else {
			select = lp2;
			lp1 = ( lp1->next == list1->next ? LNULL : lp1->next);
			lp2 = ( lp2->next == list2->next ? LNULL : lp2->next);
		}
		/* take the lp out of the original list */
		select->next = select;
		LAPPEND(merged,select);
	}
	return(merged);
	
}

/*
** insert an item in a list maintained in increasing datap order
** as usual the argument list references the largest (last) element
*/
BOOLEAN
insert_list(lastp,item,lqp)
register LIST **lastp;
register unsigned item;
LISTQ *lqp;
{
register LIST *new_l, *lp, *last, *lp_prev;

	if(*lastp == (LIST*) NULL) {
		new_l = NEWLIST(lqp);
		(unsigned) new_l->datap = item;
		LAPPEND(*lastp,new_l);
		return(TRUE);
	} else {
		lp_prev = last = *lastp;
		LFOR(lp,last->next) {
			if((unsigned) lp->datap == item) return(FALSE);
			else if( item < (unsigned) lp->datap) {
				new_l = NEWLIST(lqp);
				(unsigned) new_l->datap = item;
				LAPPEND(lp_prev,new_l);
				return(TRUE);
			}
			lp_prev = lp;
		}
		new_l = NEWLIST(lqp);
		(unsigned) new_l->datap = item;
		LAPPEND(last,new_l);
		*lastp = last;
		return(TRUE);
	}
}

quita(str,arg)
char *str,*arg;
{
char buf[132];
	sprintf(buf,str,arg);
	quit(buf);
}

quit(msg)
char *msg;
{

	fflush(stdout);
	fflush(stderr);
	if(strlen(msg) == 0 ) exit  (0);
	else {
		fprintf(stderr,"compiler(iropt) error:\t%s \n",msg);
		exit(1);
	}
}

int
hash_leaf(class,location)
LEAF_CLASS class;
union leaf_location location;
{
register unsigned key;
register char *cp;
register SEGMENT *sp;
register ADDRESS *ap;
register struct constant *constp;

key = 0;

if (class == VAR_LEAF || class == ADDR_CONST_LEAF)  {
	ap = location.ap;
	sp = ap->seg;
	if(sp->descr.builtin == BUILTIN_SEG) {
		key = ( ( (int) sp->descr.class << 16) | (ap->offset) );
	} else {
		for(cp=sp->name;*cp!='\0';cp++) key += *cp;
		key = ( (key << 16) | (ap->offset) );
	}
} else { /* class == CONST_LEAF */
	constp = location.constp;
	key = (unsigned) constp->c.i; 
}

key %= LEAF_HASH_SIZE;
return( (int) key);
}

/* 
	don't really need this for vars leafno ios sufficient FIXME
*/

LEAF *
leaf_lookup(class,type,location)
LEAF_CLASS class;
TYPE type;
union leaf_location location;
{
register LEAF *leafp;
register LIST *hash_listp;
register SEGMENT *sp;
register ADDRESS *ap,*ap2;
register struct constant *constp; 
int index;
LIST *new_l, *lp;

	if (class == VAR_LEAF || class == ADDR_CONST_LEAF)  {
		ap = location.ap;
		sp = ap->seg;
	} else {
		constp = location.constp;
	}
	index = hash_leaf(class,location);

	LFOR(hash_listp,leaf_hash_tab[index]) {
		if(hash_listp && LCAST(hash_listp,LEAF)->class == class )  {
			leafp = (LEAF*)hash_listp->datap;
			if(same_irtype(leafp->type,type)==FALSE) continue;
			if(class == VAR_LEAF || class == ADDR_CONST_LEAF) {
				ap2 = &leafp->val.addr;
				if(ap2->seg == ap->seg &&
			   		ap2->offset == ap->offset) 
			   		return(leafp);
				else continue;
			} else {
				if ( B_ISINT(type.tword) || B_ISBOOL(type.tword)) {
					if(leafp->val.const.c.i == constp->c.i) return(leafp);
					continue;
				} else if ( B_ISCHAR(type.tword) ) {
					if(strcmp(leafp->val.const.c.cp,constp->c.cp) == 0) return(leafp);
					continue;
				}
				printf(" no literal pool for tword %X FIXME",type.tword);
				continue;
			}
		}
	}

	leafp = (LEAF*) ckalloc(1, sizeof(LEAF));
	leafp->tag = ISLEAF;
	leafp->leafno = nleaves++;
	if(leaf_top) {
		leaf_top->next_leaf = leafp;
	}
	leaf_top = leafp;
	leafp->next_leaf = (LEAF*) NULL;
	leafp->type = type;
	leafp->class=class;
	if(leafp->class == VAR_LEAF || leafp->class == ADDR_CONST_LEAF ) {
		leafp->val.addr = *ap;
		if(leafp->class == VAR_LEAF) {
			lp = NEWLIST(proc_lq);
			(LEAF*) lp->datap = leafp;
			LAPPEND(sp->leaves,lp);
		}
	} else {
		leafp->val.const = *constp;
	}
	new_l = NEWLIST(proc_lq);
	(LEAF*) new_l->datap = leafp;
	LAPPEND(leaf_hash_tab[index], new_l);
	return(leafp);
}

LEAF *
ileaf(i)
int i;
{
struct constant const;
union const_u c;
TYPE t;

	t.tword = INT;
	t.aux.size = 0;
	const.isbinary = FALSE;
	const.c.i=i;
	return( leaf_lookup(CONST_LEAF,t,&const));
}

LEAF*
reg_leaf(leafp,segno,regno)
LEAF *leafp;
int segno,regno;
{
ADDRESS addr, *ap;
register SEGMENT *segp;
register LEAF *new_leaf;
register LIST *lp;
register int index;

	bzero(&addr, sizeof(ADDRESS));
	new_leaf = (LEAF*) ckalloc(1, sizeof(LEAF));
	segp = new_leaf->val.addr.seg = &seg_tab[segno];
	lp = NEWLIST(proc_lq);
	(LEAF*) lp->datap = leafp;
	LAPPEND(segp->leaves,lp);
	new_leaf->val.addr.offset = regno;
	new_leaf->val.addr.labelno = 0;
	/* always create a new leaf for it, */
	/* because same register var could have different */
	/* depends list */
	
	new_leaf->tag = ISLEAF;
	new_leaf->leafno = nleaves++;
	if(leaf_top) {
		leaf_top->next_leaf = new_leaf;
	}
	leaf_top = new_leaf;
	new_leaf->next_leaf = (LEAF*) NULL;
	new_leaf->type = leafp->type;
	new_leaf->class=VAR_LEAF;
	index = hash_leaf( VAR_LEAF, &new_leaf->val.addr );
	lp = NEWLIST(proc_lq);
	(LEAF*) lp->datap = new_leaf;
	LAPPEND(leaf_hash_tab[index], lp);
	return(new_leaf);
}

LEAF *
new_temp( tp )
TRIPLE *tp;
{
ADDRESS addr;
register SEGMENT *segp;
LEAF *leafp;
 
	bzero(&addr,sizeof(ADDRESS));
	segp = addr.seg = &seg_tab[AUTO_SEGNO];
	segp->len = roundup(segp->len,type_align(tp->type));
	segp->len += type_size(tp->type);
	addr.offset = -segp->len;
	addr.labelno = 0;
	leafp = leaf_lookup(VAR_LEAF,tp->type,&addr);
	return(leafp);
}

void
fix_del_reachdef( triple, reachdef, base )
	LIST *reachdef; TRIPLE *triple, *base;
{ /* same as fix_reachdef except it has all the expr_df information, */
  /* so do the recursive deletion. */

	LIST *lp, *list_ptr;
	TRIPLE *tp;
	void delete_triple();
	void delete_list();
	BOOLEAN can_delete();

	LFOR(lp, reachdef)
	{
		tp = LCAST(lp, EXPR_REF)->site.tp;
		LFOR(list_ptr, tp->canreach)
		{
			if( LCAST(list_ptr, EXPR_REF)->site.tp == triple )
			{
				if( list_ptr->next == list_ptr /* only can reaches here */ &&
								tp != base )
				{ 
					tp->canreach  = LNULL;
					if( can_delete( tp ) )
						delete_triple( tp,
						LCAST(lp, EXPR_REF)->site.bp );
				}
				else
				{
					delete_list( &tp->canreach, list_ptr );
					if( tp != base && tp->op == ASSIGN &&
							tp->right->operand.tag == ISTRIPLE &&
							tp->canreach &&
							tp->canreach == tp->canreach->next &&
							LCAST( tp->canreach, EXPR_REF)->site.tp == (TRIPLE *)tp->right  &&
							((TRIPLE *)tp->right)->canreach &&
							((TRIPLE *)tp->right)->canreach ==
							((TRIPLE *)tp->right)->canreach->next )
					{
						if( can_delete( tp ) )
						{
							delete_triple( tp,
								LCAST(lp, EXPR_REF)->site.bp);
						}
					}
				}
				break;
			}
		}
	}
}

void
fix_reachdef( triple, reachdef, base )
	LIST *reachdef; TRIPLE *triple, *base;
{
	LIST *lp, *list_ptr;
	TRIPLE *tp;
	void delete_triple();
	void delete_list();
	BOOLEAN can_delete();

	LFOR(lp, reachdef)
	{
		tp = LCAST(lp, EXPR_REF)->site.tp;
		LFOR(list_ptr, tp->canreach)
		{
			if( LCAST(list_ptr, EXPR_REF)->site.tp == triple )
			{
				if( list_ptr->next == list_ptr /* only can reach here */ &&
					tp != base )
				{ 
					tp->canreach  = LNULL;
					if( can_delete( tp ) )
						delete_triple( tp,
						LCAST(lp, EXPR_REF)->site.bp );
				}
				else
					delete_list( &tp->canreach, list_ptr );
				break;
			}
		}
	}
}

void
fix_canreach( triple, can_reach ) LIST *can_reach; TRIPLE *triple;
{
	LIST *lp, *list_ptr;
	TRIPLE *tp;

	LFOR(lp, can_reach)
	{ /* update the reachdef */
		tp = LCAST( lp, EXPR_REF )->site.tp;
		if( tp->left && tp->left->operand.tag == ISTRIPLE &&
			((TRIPLE *)tp->left)->expr == triple->expr )
		{
			LFOR( list_ptr, tp->reachdef1 )
			{
				if( LCAST(list_ptr, EXPR_REF )->site.tp ==
						triple )
				{ /* no longer can reachs here */
					delete_list( &tp->reachdef1,list_ptr );
					break;
				}
			}
			continue;
		}

		if( ISOP( tp->op, BIN_OP) && tp->right && tp->right->operand.tag == ISTRIPLE &&
			((TRIPLE *)tp->right)->expr == triple->expr )
		{
			LFOR( list_ptr, tp->reachdef2 )
			{
				if( LCAST(list_ptr, EXPR_REF )->site.tp ==
						triple )
				{ /* no longer can reachs here */
					delete_list( &tp->reachdef2,list_ptr );
					break;
				}
			}
		}
	}

}

type_align(type)
TYPE type;
{
static int type_alignment[NTYPES] = {
	
	/*UNDEF*/	0,
	/*FARG*/	4,
	/*CHAR*/	1,
	/*SHORT*/	2,
	/*INT*/		4,
	/*LONG*/	4,
	/*FLOAT*/	4,
	/*DOUBLE*/	4,
	/*STRTY*/	4,
	/*UNIONTY*/	4,
	/*ENUMTY*/	4,
	/*MOETY*/	0,
	/*UCHAR*/	1,
	/*USHORT*/	2,
	/*UNSIGNED*/	4,
	/*ULONG*/	4,
	
	/*BOOL*/	4,
	/*EXTENDEDF*/	4,
	/*COMPLEX*/	4,
	/*DCOMPLEX*/ 	4,
	/*STRING*/	1,
	/*VOID*/	0 
};
	if(ISPTR(type.tword)) {
		return(type_alignment[ULONG]);
	} else if (BTYPE(type.tword) != type.tword) {
		quita("type_align: alignment of constructed type >%X<",type.tword);
	} else {
		return(type_alignment[type.tword]);
	}
}

type_size(type)
TYPE type;
{
int size;
static int type_sizes[NTYPES] = {
	
	/*UNDEF*/	0,
	/*FARG*/	0,
	/*CHAR*/	1,
	/*SHORT*/	2,
	/*INT*/		4,
	/*LONG*/	4,
	/*FLOAT*/	4,
	/*DOUBLE*/	8,
	/*STRTY*/	0,
	/*UNIONTY*/	0,
	/*ENUMTY*/	0,
	/*MOETY*/	0,
	/*UCHAR*/	1,
	/*USHORT*/	2,
	/*UNSIGNED*/4,
	/*ULONG*/	4,
	
	/*BOOL*/	4,
	/*EXTENDEDF*/	0,
	/*COMPLEX*/	8,
	/*DCOMPLEX*/ 16,
	/*STRING*/	0,
	/*VOID*/	0 
};

	if(ISPTR(type.tword)) {
		size = type_sizes[ULONG];
	} else if (BTYPE(type.tword) != type.tword) {
		quita("type_size: size of constructed type >%X<",type.tword);
	} else {
		size = type_sizes[type.tword];
		if(size == 0) {
			if(type.tword == STRTY || type.tword == STRING ) {
				size = type.aux.size;
			} else {
				quita("type_size: don't know the size of type >%X<",type.tword);
			}
		}
	}
	return(size);
}

BOOLEAN
same_irtype(p1,p2)
TYPE p1,p2;
{
	if(p1.tword == p2.tword ) {
		if(BTYPE(p1.tword) == STRTY ) {
			if(p1.aux.size != p2.aux.size) return(FALSE);
		} else if( ISFTN(p1.tword) ){
			if(p1.aux.func_descr != p2.aux.func_descr) return(FALSE);
		} else {
			return(TRUE);
		}
	}
	return(FALSE);
}

void
delete_triple( tp, bp ) TRIPLE *tp; BLOCK *bp;
{
	register LIST *lp;

	LFOR( lp, tp->implicit_use )
	{ /* delete the implicit use with the triple */
		delete_triple( lp->datap, bp );
	}

	if( tp == tp->tnext && tp == tp->tprev )
	{ /* the only triple in this block */
		bp->last_triple = TNULL;
	}
	else
		if( tp == bp->last_triple ) /* deleting the last triple */
			bp->last_triple = tp->tprev;

	unlist_triple( tp );
	tp = bp->last_triple;
	if(tp && tp->tprev == tp && tp->op == LABELDEF) {
		unlist_triple(tp);
		bp->last_triple = TNULL;
	}
}

void
remove_triple( tp, bp ) TRIPLE *tp; BLOCK *bp;
{ /* this is almost like the delete triple, except theat */
  /* this routine won"t do the recursive deletion, becasue */
  /* there is no expr information. */

	register LIST *lp1, *lp2;
	register TRIPLE *triple;

	LFOR( lp1, tp->implicit_use )
	{ /* delete the implicit use with the triple */
		remove_triple( lp1->datap, bp );
	}

	if( tp == tp->tnext && tp == tp->tprev )
	{ /* the only triple in this block */
		bp->last_triple = TNULL;
	}
	else
	{
		if( tp == bp->last_triple ) /* deleting the last triple */
			bp->last_triple = tp->tprev;
	}
	/*  fix reachdef and canreach */

	if( tp->left->operand.tag == ISTRIPLE )
	{ /* at the time this routine be called,
	   * all triples must be in the TREE format
	   */
		if( can_delete( tp->left ) )
			remove_triple( tp->left, bp );
	}
	else
	{ /* it is a leaf, and we do compute the var_df */
		LFOR( lp1, tp->reachdef1 )
		{
			triple = LCAST( lp1, VAR_REF )->site.tp;
			LFOR( lp2, triple->canreach )
			{
				if( LCAST(lp2, VAR_REF)->site.tp == tp )
				{ BLOCK *bp;
					bp = LCAST(lp2, VAR_REF)->site.bp;
					delete_list( &triple->canreach, lp2 );
					if( triple->canreach == LNULL && can_delete( triple ))
						remove_triple( triple, bp );
					break;
				}
			}
		}
	}

	if( ISOP( tp->op, BIN_OP ) )
	{
		if( tp->right->operand.tag == ISTRIPLE )
		{
			if( can_delete( tp->right ) )
				remove_triple( tp->right, bp );
		}
		else
		{ /* a LEAF */
			LFOR( lp1, tp->reachdef2 )
			{
				triple = LCAST( lp1, VAR_REF )->site.tp;
				LFOR( lp2, triple->canreach )
				{
					if( LCAST(lp2, VAR_REF)->site.tp == tp )
					{ BLOCK *bp;
						bp = LCAST(lp2, VAR_REF)->site.bp;
						delete_list( &triple->canreach, lp2 );
						if( triple->canreach == LNULL &&
								can_delete( triple ) )
							remove_triple( triple, bp );
						break;
					}
				}
			}
		}
	}
	
	/* unlist triple */

	tp->tprev->tnext = tp->tnext;
	tp->tnext->tprev = tp->tprev;

	tp->left = (NODE *)free_triple_lifo;
	free_triple_lifo = tp;
}

void
delete_list( tail, lp ) register LIST **tail, *lp;
{
	LIST *lp1_prev, *lp1;
	
	if( lp == LNULL )
		return;
	if( lp->next == lp) {
		*tail = LNULL;
		return;
	}

	/* else, search the *tail list for lp */
	lp1_prev = *tail;
	
	LFOR( lp1, (*tail)->next ) {
		if( lp1 == lp ) {
			lp1_prev->next = lp->next;
			if( *tail == lp ) { /* deleting the tail */
				*tail = lp1_prev;
			}
			return;
		} else {
			lp1_prev = lp1;
		}
	}
	/*  can NOT find lp in *tail list */
	quit("delete_list: can not find the item >%X< in the list", lp );
}

BOOLEAN
can_delete( tp ) register TRIPLE *tp;
{ /* 
   *This routine test see if tp can be deleted.
   * Right now, we assume that we can not delete
   * any triple which assign a value to a STATIC or
   * EXTERN variable savely.  Do not delete the triple
   * that has implicit definations.
   */
	register union leaf_value *lv;
	register LEAF *func;

	if( tp->op == FCALL )
	{ /* if it"s INTR_FUNC then delete it else change */
	  /* to a SCALL */
		func = (LEAF *)tp->left;
		if( func->class == CONST_LEAF &&
				func->type.aux.func_descr == INTR_FUNC )
			return( TRUE );
		else {
			tp->op = SCALL;
			return( FALSE );
		}
	}

	if( tp->implicit_def )
		return( FALSE );
				 
	if( ISOP( tp->op, VALUE_OP ) )
		return( TRUE );

	if( tp->op != ASSIGN && tp->op != ISTORE )
		return( FALSE );

	return( TRUE );
}

BOOLEAN
no_change( bp, from, to, var ) 
BLOCK *bp; 
TRIPLE *from, *to; 
LEAF *var;
{ /* 
   * This routine test see if var has been changed or not
   * between triple from(included) and to(excluded)
   * here from and to MUST point to two triples that
   * in the SAME basic block and from is preceding the to
   * in the lexical order.
   */

	register VAR_REF *rp;
	register LIST *lp;
	register TRIPLE *tp;

	if(ISCONST(var))
	/* it is a constant */
		return( TRUE );
	TFOR(tp,from) {
		if( tp == to) break;
		for(rp = tp->var_refs; rp && rp->site.tp == tp ; rp = rp->next) {
			if( rp->reftype != VAR_AVAIL_DEF &&
					rp->reftype != VAR_DEF )
				continue;
			if( rp->leafp == var ) /* defines var */
				return( FALSE );
		}

	}
	/* NO CHANGE */

	return( TRUE );
}

/*
**	binary constants are usually treated as byte strings - this is
**	for when it is necessary to deal with them as aligned data
*/
l_align(cp1)
register char *cp1;
{
union {
	char c[4];
	int w;
} word;
register char *cp2 = word.c;
register int i;

	for(i=0;i<4;i++){
		*cp2++ = *cp1++;
	}
	return(word.w);
}

 
SET_PTR
heap_set(n_rows, n_bits)
int n_rows, n_bits;
{
register int bit_rowsize, word_rowsize, byte_setsize;
register SET_PTR set;

	bit_rowsize = roundup(n_bits,BPW); 
	word_rowsize = bit_rowsize / BPW; 
	byte_setsize = word_rowsize * n_rows * sizeof(unsigned); 
	set = (SET_PTR) ckalloc(1,byte_setsize+sizeof(struct set_description));
	set->nrows = n_rows; 
	set->bit_rowsize = bit_rowsize; 
	set->word_rowsize = word_rowsize; 
	set->bits = (unsigned *) ((char*)set + sizeof(struct set_description));
	return set;
}

roundup(i1,i2)
{
	return (i2*((i1+i2-1)/i2));
}

void
delete_implicit()
{ /* do NOT do this if it is a mutiple entry procedure */
  /* because the leaf->entry_define and exit_use are NOT correct */

	LEAF *leafp;
	LIST *lp, **lpp;
	struct segdescr_st descr;
	void delete_entry();
	
	if( ! entry_block->is_ext_entry ) {
		return;
	}
	
	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) 
	{ /* for each hash entry */
		LFOR( lp, *lpp )
		{ /* for each leaf */
			
			leafp = (LEAF *) lp->datap;
			if( ISCONST(leafp) || leafp->references == (VAR_REF *) NULL )
				continue;
				
			descr = leafp->val.addr.seg->descr;
			if( descr.class == BSS_SEG || descr.class == DATA_SEG ||
				descr.class == ARG_SEG ||
				descr.external == EXTSTG_SEG )
			{ /*delete unnecessary entry defination and exit use*/ 
				delete_entry( leafp );
			}
		}
	}
}

void
delete_entry( leafp ) LEAF *leafp;
{
	TRIPLE *tp;
	struct segdescr_st descr;
	void remove_triple(), delete_list();
	LIST *lp;

	tp = leafp->entry_define;
	if( tp == TNULL ) return;	/* partial_opt == TRUE */
	
	if( ( tp->canreach == LNULL ) ||
			(tp->canreach == tp->canreach->next &&
			 LCAST( tp->canreach, VAR_REF)->site.tp == 
							leafp->exit_use ) )
	{ /* do NOT need this entry defination */
		descr = leafp->val.addr.seg->descr;
		if( descr.external == EXTSTG_SEG )
		{
			tp = leafp->exit_use;
			/* at least one def can reach exit use */
			/* except when the end stmt is NOT reachable */
			if( tp && tp->reachdef1 && tp->reachdef1 == tp->reachdef1->next && 
				LCAST(tp->reachdef1, VAR_REF)->site.tp == 
							leafp->entry_define )
			{ /* delete both entry define and exit use */
				remove_triple(tp, exit_block); /* both are gone */
				leafp->exit_use=TNULL;
			}
			/* delete the entry define */
			remove_triple(leafp->entry_define,entry_block);
			leafp->entry_define = TNULL;
		}
		else
		{ /* bss or data seg or arg seg */
			remove_triple(tp,entry_block);
			if( leafp->exit_use != TNULL )
			{
				remove_triple( leafp->exit_use, exit_block );
			}
			leafp->entry_define = leafp->exit_use = TNULL;
		}
	}
	else
	{
		tp = leafp->exit_use;
		if( tp && tp->reachdef1 && tp->reachdef1 == tp->reachdef1->next && 
				LCAST(tp->reachdef1, VAR_REF)->site.tp == 
							leafp->entry_define )
		{ /* no body modify it, so delete the exit use */
			remove_triple(tp, exit_block); /* both are gone */
			leafp->exit_use=TNULL;
		}
	}
}

