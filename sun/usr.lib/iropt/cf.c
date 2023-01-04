
#ifndef lint
static  char sccsid[] = "@(#)cf.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include "iropt.h"
#include <stdio.h>
#include <ctype.h>

#define REF_SITE 0
#define DEF_SITE 1

LIST *copy_list();
BLOCK *new_block();
TRIPLE *append_triple();
BLOCK *exit_block; 
extern BLOCK *entry_tab, *entry_top;
extern LIST *ext_entries;

struct labelref {
	int refdef,labelno;
	TRIPLE *triple;
}; 

LIST *label_list;
extern int n_label_defs;
extern BOOLEAN optimflag[];
extern LISTQ *tmp_lq;

/*
**	add to a list of label refs & defs such that label number is increasing
**	and the def (if any) precedes the ref(s) (if any)
**  as with all other lists, label_list points to the last item
**  and label_list->next to the first
*/
add_labelref(tp)
TRIPLE *tp;
{
int refdef,labelno;
register struct labelref *new_labp, *labp;
register LIST *lp0, *lp1, *lp0_prev, *lp1_prev;

	refdef = (tp->op == LABELDEF ? DEF_SITE : REF_SITE);
	labelno = tp->left->leaf.val.const.c.i;

	new_labp = (struct labelref*) ckalloc(1,sizeof(struct labelref));
	new_labp->refdef = refdef;
	new_labp->labelno = labelno;
	new_labp->triple = tp;

	lp1 = NEWLIST(proc_lq);
	(struct labelref *) lp1->datap = new_labp;
	if(label_list == (LIST*) NULL) {
		LAPPEND(label_list,lp1);
	} else {
			lp0_prev = label_list;
			LFOR(lp0,label_list->next) {
				labp = (struct labelref*) lp0->datap;

				if( labp->labelno < labelno) {
					lp0_prev = lp0;
					continue;
				}

				if( labp->labelno > labelno) {
					LAPPEND(lp0_prev,lp1);
					return;
				}

				if(refdef == DEF_SITE ) {
					if( labp->refdef == DEF_SITE ) 
						quita("label %d is multiply defined",labelno);
					LAPPEND(lp0_prev,lp1);
				} else {
					lp1_prev = lp0;
					LAPPEND(lp0,lp1);
					if(lp1_prev == label_list) label_list = lp1;
				}
				return;
			}
			LAPPEND(label_list,lp1);
	}
}

connect_labelrefs()
{
register LIST *lp,*lp2;		/* point to two adjacent items in the label_list*/
register struct labelref 
	*labp, *labp2;
register TRIPLE *def_pt;	/* points to the label triple */
register TRIPLE *ref_pt;	/* points to triples that reference  that label  */
int workingon;				/* number of the label being worked on */
BLOCK *bp;
register LIST *lp3;			/* cursor used in fixing labelno constants */
LEAF *leafp;

	if(label_list == NULL) return;
	label_list = label_list->next;
	lp = label_list;

	do {
			labp = (struct labelref*) lp->datap;
			if( labp->labelno == -1)  {
				(BLOCK*)labp->triple->left = (BLOCK*) NULL;
				lp = lp->next;
			} else { 
				if(labp->refdef != DEF_SITE ) {
					quita("connect_labelref: label %d is undefined",
											labp->labelno);
				}
				workingon = labp->labelno;
				def_pt = LCAST(lp,struct labelref)->triple;
		
				lp2 = lp->next;
				labp2 = (struct labelref*) lp2->datap;
				if(labp2->labelno == workingon && labp2->refdef == REF_SITE){
					/* 
					**	this label is referenced, hence mark it as a "leader"
					*/
					def_pt->visited = TRUE;
					for(bp=entry_block; bp; bp = bp->next) {
						if(bp->labelno == workingon) break;
					}
					if(!bp) {
						bp=new_block();
					}
					bp->labelno = new_label();
					LFOR(lp3,labelno_consts) {
						 leafp = (LEAF*) lp3->datap; 
						 if(leafp->val.const.c.i == workingon) {
						  	leafp->val.const.c.i = bp->labelno;
							break;
						  }
					}

					/*
						now turn references to label "workingon" 
						from integers into block pointers
		
					*/
					do {
						ref_pt = labp2->triple;
						(BLOCK*) ref_pt->left = bp;
						lp2=lp2->next;
						if(lp2) labp2 = (struct labelref*)lp2->datap;
					} while(lp2 && labp2->labelno == workingon);
					/* have the label triple point to the block */
					def_pt->left = (NODE*) bp;
				}
			lp=lp2;
		}
	} while(lp != label_list);
	label_list = LNULL;
}

form_bb() 
{
register BLOCK *bp;

	if(ext_entries->next != ext_entries) { /* then scan the "dummy" entry_block*/
		if(entry_block->last_triple) {
			partition_entry_triples(entry_block);
		}
	}
	for(bp=entry_tab; bp < entry_top ; bp ++ ) {
		if(bp->last_triple) {
			partition_entry_triples(bp);
		} 
	}

	make_pred_list();
	if(optimflag[0]==TRUE) {
		move_value_triples();
	}
}

/*	
	pass through all triples and append them to their leaders
	use the references for the last triple in a block to build the succ list
*/

/* 
** returns the next triple in the list and sets branch flags
*/
# 	define NEXT_TRIPLE(tp) \
	( 	(lastwasbranch = isbranch),\
		((tp) = ((tp) == list_last ? TNULL : (tp)->tnext)),\
		(isbranch = (tp == TNULL ? 0 : ISOP(tp->op,BRANCH_OP)))\
	)

#	define SAME_BLOCK(tp) (tp && tp->visited != TRUE && lastwasbranch == 0)

#	define	PATCH_TRIPLES(bp,first_triple,next_block) \
		first_triple->tprev=next_block->tprev;\
		next_block->tprev->tnext = first_triple;\
		bp->last_triple=next_block->tprev;\
		next_block->tprev = list_last; \
		list_last->tnext = next_block; \
		make_succ_list(bp,next_block);\
		prev_block = bp;
	
LOCAL
partition_entry_triples(entry)
BLOCK *entry;
{
register TRIPLE *first;
register TRIPLE *tp;
TRIPLE  *list_last, *move_comments();
register BLOCK *bp;
int isbranch = 0, lastwasbranch = 0;
BLOCK *prev_block;

		if(entry->is_ext_entry == TRUE) {
					/* change its labelno since not done in connect_labelrefs */
			entry->labelno = new_label();
					/* and mark its first triple a leader */
			entry->last_triple->tnext->visited = TRUE; 
		}
		list_last = entry->last_triple;
		tp = first = entry->last_triple->tnext;
		if(tp->op != LABELDEF)  {
			quit("partition_entry_triples: first triple is not a label");
		}
		/*
		**	pass over the triples that belong to the entry block
		*/
		do {
			NEXT_TRIPLE(tp);
		} while(SAME_BLOCK(tp));

		if(tp) {
		/*
		**	some triples remain : assign them to the block identified by the 
		**	"leader" label and fix the entry block's triple list
		*/
			PATCH_TRIPLES(entry,first,tp);
			do {
				if(tp->op == PASS) {
					tp = move_comments(tp,&list_last);
				}
				first = tp;
				if(tp->visited != TRUE) {
					/*
					**	put unreachable code in separate blocks identified by 
					**	a -1 labelno - insert them so as to maintain code order
					*/
					bp = new_block();
					bp->labelno = -1;
					move_block(prev_block,bp);
				} else {
					bp = (BLOCK*) tp->left;
				}
				do {
					NEXT_TRIPLE(tp);
				} while(SAME_BLOCK(tp));
				if(tp) {
					PATCH_TRIPLES(bp,first,tp);
				} else {
					first->tprev=list_last;
					list_last->tnext = first;
					bp->last_triple = list_last;
					make_succ_list(bp,(TRIPLE*) NULL);
				}
				if(bp->last_triple->tnext->op != LABELDEF) {
					TYPE type;
					type.tword = UNDEF;
					type.aux.size = 0;
					append_triple(bp->last_triple,LABELDEF,(NODE*)bp,(NODE*)ileaf(0),type);
				} else {
					bp->last_triple->tnext->left = (NODE*) bp;
				}
			} while (tp);
		} else {
			make_succ_list(entry, (TRIPLE*) NULL);
		}

		if(entry->last_triple->tnext->op != LABELDEF) {
			TYPE type;
			type.tword = UNDEF;
			type.aux.size = 0;
			append_triple(entry->last_triple,LABELDEF,(NODE*)entry,(NODE*)ileaf(0),type);
		} else {
			entry->last_triple->tnext->left = (NODE*) entry;
		}

}

LOCAL
move_value_triples()
{
register BLOCK *bp;
register TRIPLE *tp, *first, *tlp;
register int tripleno;

	for(bp=entry_block; bp; bp=bp->next) {
		if(bp->last_triple) first=bp->last_triple->tnext;
		TFOR(tp,first){
			if(	ISOP(tp->op,ROOT_OP) &&
				( ISOP(tp->op,USE1_OP) || ISOP(tp->op,USE2_OP) )
			){
				check_children(tp,tp->tprev);
			}
		}
	}
	tripleno=0;
	for(bp=entry_block; bp; bp=bp->next) {
		if(bp->last_triple) first=bp->last_triple->tnext;
		TFOR(tp,first){
			tp->tripleno = tripleno++;
			if( tp->op == SCALL || tp->op == FCALL ) {/* PARMS */
				TFOR(tlp, (TRIPLE *)tp->right ) {
					tlp->tripleno = tripleno++;
				}
			}
		}
	}
}

LOCAL
check_children(parent, insert_after, this_block)
register TRIPLE *parent, *insert_after;
{
register TRIPLE *child;

	/* we pay no attention to last_triple in any of this on the grounds
	** that only  value_ops are moved and a value_op cannot be the last_triple
	** of either the from or to blocks
	*/
	if(	ISOP(parent->op,USE1_OP)	&&
		parent->left->operand.tag == ISTRIPLE
	){
		child = (TRIPLE*) parent->left;
		if(child != insert_after) {
			child->tprev->tnext = child->tnext;
			child->tnext->tprev = child->tprev;
			child->tprev = child->tnext = child;
			TAPPEND(insert_after,child);
		}
		check_children(child,child->tprev);
	}
	if(	ISOP(parent->op,USE2_OP)	&&
		parent->right->operand.tag == ISTRIPLE
	){
		child = (TRIPLE*) parent->right;
		if(child != insert_after) {
			child->tprev->tnext = child->tnext;
			child->tnext->tprev = child->tprev;
			child->tprev = child->tnext = child;
			TAPPEND(insert_after,child);
		}
		check_children(child,child->tprev);
	}
	if(ISOP(parent->op,NTUPLE_OP)) {
		register TRIPLE *params;
		params = (TRIPLE*) parent->right;
		TFOR(child, params) {
			check_children(child,parent->tprev);
		}
	}
}

LOCAL TRIPLE*
move_comments(comment,last_triple)
TRIPLE *comment, **last_triple;
{
TRIPLE *code;

	if (comment == *last_triple) {
		return comment;
	}
	code = comment;
	while(code != *last_triple && code->op == PASS) {
		code = code->tnext;
	}
	if (code->op == PASS) {
		return comment;
	}
	if (code == *last_triple) {
		*last_triple = code->tprev;
	}
	code->tprev->tnext = code->tnext;
	code->tnext->tprev = code->tprev;

	code->tnext = comment;
	code->tprev = comment->tprev;
	comment->tprev->tnext = code;
	comment->tprev = code;
	return code;
}


make_succ_list(block,next_triple)
BLOCK* block;
TRIPLE *next_triple;
{
register LIST *lp1;
register TRIPLE *label, *last;
TYPE type;
BLOCK *next_block;
TRIPLE *ref_triple;

	last = (TRIPLE*) block->last_triple;
	if(last == (TRIPLE*) NULL) {
		block->succ = LNULL;
	} else if(ISOP(last->op,BRANCH_OP)) {
		switch(last->op) {
			case SWITCH :
			case CBRANCH :
			case INDIRGOTO :
			case REPEAT:
				TFOR(label,(TRIPLE*)last->right) {
					insert_list(&block->succ,(BLOCK*) label->left,proc_lq);
				}
				break;
	
			case GOTO :
				label = (TRIPLE*) last->right;
				if((BLOCK*)label->left != (BLOCK*) NULL) {
					block->succ = NEWLIST(proc_lq);
					(BLOCK*) block->succ->datap = (BLOCK*) label->left;
				} else {
					if(exit_block == (BLOCK*) NULL) {
						exit_block = block;
					} else {
						quit("make_succ_list: multiple exit blocks");
					}
				}
				break;
		}
	} else if(next_triple == (TRIPLE*) NULL) {
			block->succ = LNULL;
	} else {
		if(next_triple->op != LABELDEF || next_triple->visited != TRUE ) {
			quit("make_succ_list: fall through into an unlabeled block");
		}
		next_block = (BLOCK*) next_triple->left;

		type.tword = UNDEF;
		type.aux.size = 0;

		ref_triple = append_triple(LNULL,LABELREF,(NODE*) next_block, (NODE*) ileaf(0), type);
 		block->last_triple = append_triple(block->last_triple, GOTO,
 							(NODE*)NULL, (NODE*)ref_triple, type);

		block->succ = NEWLIST(proc_lq);
		(BLOCK*) block->succ->datap = next_block;
	}
}

make_pred_list()
{
register BLOCK *a, *b;
register LIST *lp1, *lp2;
/* 
**	build the predecessor lists from the succesor lists:
**	all b that are successors of a have a as a predecessor
*/

	for(a=entry_block;a;a=a->next) {
		LFOR(lp1,a->succ) {
			b = (BLOCK*) lp1->datap;
			lp2=NEWLIST(proc_lq);
			(BLOCK*) lp2->datap = a;
			LAPPEND(b->pred,lp2);
		}
	}
}

/*
**	look for edges where the tail is the only predecessor of the head and the
**	head the only succecessor of the tail and move all code in the tail to 
**	the head (nb tail->head)
cleanup_cf()
{
register BLOCK *pretail,*tail, *head, *bp;
register LIST *lp,*lp2;
TRIPLE *head_first, *tail_first;
int nblocks_at_start;

	nblocks_at_start = nblocks;
	tail = entry_block;
	do {
		pretail = tail;
		tail = tail->next;
		if(tail->succ && tail->succ->next == tail->succ) {
			head = (BLOCK*) tail->succ->datap;
			if(head->pred && head->pred->next == head->pred) {
				if(! ISOP(tail->last_triple->op,BRANCH_OP)) {
					quita("cleanup_cf(): single exit/entry blocks connected by op >>%s<<",op_descr[ORD(tail->last_triple->op)].name);
				} else {
					delete_triple(tail->last_triple,tail);
					if(tail->last_triple != TNULL) {
						if(head->last_triple == TNULL) {
							head->last_triple = tail->last_triple;
						} else {
							head_first = head->last_triple->tnext;
							tail_first = tail->last_triple->tnext;
							tail_first->tprev = head->last_triple;
							head->last_triple->tnext = tail_first;
							head_first->tprev = tail->last_triple;
							tail->last_triple->tnext = head_first;
						}
					}
					LFOR(lp,tail->pred) {
						redirect_flow((BLOCK*)lp->datap,tail,head);
					}
					head->pred = copy_list(tail->pred,proc_lq);
					pretail->next = tail->next;
					nblocks_at_start--;
					tail = pretail;
				}
			}
		}
	} while(tail->next);
	if(nblocks != nblocks_at_start) {
		nblocks =0;
		for(bp=entry_block; bp; bp=bp->next) {
			bp->blockno = nblocks++;
		}
	}
}
*/
