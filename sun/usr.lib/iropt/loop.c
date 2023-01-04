#ifndef lint
static	char sccsid[] = "@(#)loop.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "iropt.h"
#include "loop.h"
#include <stdio.h>

char *loop_block_tab;
BLOCK *first_block_dfo;
static BLOCK *dfonext;
SET_PTR dominate_set;
static int *dfnumber1, dfcount1;	/* 0 -> n-1 : order in which visited */
static int *dfnumber2, dfcount2;	/* n-1 -> 0  : how "far" from init node */
static int block_set_wordsize;
LIST *edges, *loops;
int nloops;
void dfo_search();
BOOLEAN bit_test(), insert_list();
BLOCK *new_block();
extern  LISTQ *tmp_lq, *proc_lq;
LISTQ *loop_lq;
extern LIST *indirgoto_targets;
/* we build one loop for each header regardless of how many back edges come
** into it. the loop build goes as follows : recognize a back edge, build
** its natural loop, add this to other loops that share the same header
** (struct loop_list) then build a loop struct for export to other routines
*/
static struct loop_list {
	BLOCK *header, *preheader;
	LIST *blocks, *edges;
	LOOP *loop;
	struct loop_list *next;
} *headers;

void
find_dfo() 
{
register BLOCK *bp, *from, *to;
register LIST *lp;
register EDGE *ep;
int i;

	edges = LNULL;
	for(bp=entry_block;bp;bp=bp->next) {
		bp->visited = FALSE;
	}
	dfnumber1 = (int*) ckalloca(nblocks*sizeof(int));
	dfnumber2 = (int*) ckalloca(nblocks*sizeof(int));
	dfcount1 = 0;
	dfcount2 = nblocks-1;
	entry_block->dfoprev = (BLOCK*) NULL;
	dfonext = (BLOCK*) NULL;

	dfo_search(entry_block);
	/*
	**	distinguish retreating from cross edges
	**	for retreating edges the dfs distance number of
	** 	the from is >= than that of the to, ie to is an ancestor
	** 	of from in the DFST. The remaining edges are cross by elimination
	*/
	LFOR(lp,edges) {
		ep = (EDGE*) lp->datap;
		if(ep->edgetype == RETREATORCROSS) {
			from = ep->from;
			to = ep->to;
			if(dfnumber2[from->blockno] >= dfnumber2[to->blockno]) {
				ep->edgetype = RETREATING;
			} else {
				ep->edgetype = CROSS;
			}
		}
	}
	if(SHOWDF == TRUE) {
		printf(" block visited all_children_visited\n");
		for(i=0;i<nblocks;i++) {
			printf(" %d %d %d\n",i,dfnumber1[i],dfnumber2[i]);
		
		}
	}
}

LOCAL void
dfo_search(bp)
register BLOCK *bp;
{
register LIST *succ, *lp;
register BLOCK *next;
register EDGE *ep;

	bp->visited = TRUE;
	dfnumber1[bp->blockno] = dfcount1;
	dfcount1 += 1;
	LFOR(succ,bp->succ) {
		next = (BLOCK*) succ->datap;
		ep = (EDGE*) ckalloc(1,sizeof(EDGE));
		ep->from = bp;
		ep->to = next;
		lp = NEWLIST(proc_lq);
		(EDGE*) lp->datap = ep;
		LAPPEND(edges,lp);
		if(next->visited == FALSE) {
			/*
			**	all visited to unvisited vertix edges are in the DFST
			*/
			ep->edgetype = ADVANCING_TREE;
			dfo_search(next);
		} else {
			/*
			**	if we've visited this node and its predecessor
			**	dfnumber 1s have been assigned to both vertices
			**	if the from is less than to it's an advancing edge
			**	else it could be a retreating or cross edge
			**	these will be distinguished later
			*/
			if(dfnumber1[next->blockno] > dfnumber1[bp->blockno]) {
				ep->edgetype = ADVANCING;
			} else {
				ep->edgetype = RETREATORCROSS;
			}
		}
	}
	if(dfonext) {
		dfonext->dfoprev = bp;
	} else {
		first_block_dfo = bp;
	}
	dfnumber2[bp->blockno] = dfcount2;
	dfcount2--;
	bp->dfonext = dfonext;
	dfonext = bp;
}

/*
** dragon book algorithm 13.2
*/
find_dominators()
{
register BIT_INDEX dominates_index, mask;
SET_PTR mask_ptr, heap_set();
register BLOCK *pred_bp, *bp;
register LIST *pred;
register int i;
BLOCK *after_entry;
BOOLEAN change;

	block_set_wordsize = ( roundup(nblocks, BPW) ) / BPW;
	dominate_set = heap_set(nblocks,nblocks);
	AUTO_SET(mask_ptr,1,nblocks);
	mask = mask_ptr->bits;

	/*
	**	D(n0) = n0
	*/
	bit_set(mask_ptr,0,entry_block->blockno);
	dominates_index = dominate_set->bits + 
				(entry_block->blockno*block_set_wordsize);
	for(i=0;i<block_set_wordsize;i++) {
		dominates_index[i] = mask[i];
	}

	/*
	**	for n - in N-n0 do D(n) = N
	*/
	after_entry = entry_block->dfonext;
	for(bp=after_entry;bp;bp=bp->next) {
		dominates_index = dominate_set->bits + (bp->blockno*block_set_wordsize);
		for(i=0;i<block_set_wordsize;i++) {
			dominates_index[i] = ~0;
		}
	}
	change = TRUE;

	while (change == TRUE) {
		change = FALSE;
		for(bp=after_entry;bp;bp=bp->dfonext) {
			/*
			**	mask = {n} union (intersection D(p) for all p pred of bp)
			*/
			for(i=0;i<block_set_wordsize;i++) {
				mask[i] = ~0;
			}
			LFOR(pred,bp->pred) {
				pred_bp = (BLOCK*) pred->datap;
				dominates_index = dominate_set->bits + 
						(pred_bp->blockno*block_set_wordsize);
				for(i=0;i<block_set_wordsize;i++) {
					mask[i] &= dominates_index[i];
				}
			}
			bit_set(mask_ptr,0,bp->blockno);

			dominates_index = dominate_set->bits + 
							(bp->blockno*block_set_wordsize);
			for(i=0;i<block_set_wordsize;i++) {
				if(mask[i] != dominates_index[i]) {
					change = TRUE;
				}
				dominates_index[i] = mask[i];
			}
		}
	}
}

find_loops()
{
BLOCK *bp;
int i;
BIT_INDEX dominates_index;
register LIST *lp;
EDGE *ep;

	loops = LNULL;
	nloops = 0;
	headers = (struct loop_list *) NULL;
	if(loop_lq == NULL) loop_lq = new_listq();
	find_dominators();
	if(SHOWDF == TRUE) {
		printf(" DOMINATOR df bits\n");
		for(bp=entry_block;bp;bp=bp->next) {
			printf(" %d\n",bp->blockno);
			dominates_index = dominate_set->bits + bp->blockno;
			for(i=0;i<block_set_wordsize;i++) {
				printf(" %d %X\n",i,dominates_index[i]);
			}
		}
	}

	LFOR(lp,edges) {
		ep = (EDGE*) lp->datap;
		if(ep->edgetype == RETREATING) {
			if(dominates(ep->to->blockno, ep->from->blockno)){
				ep->edgetype = RETREATING_BACK;
				make_loop(ep);
			}
		}
	}
	if(SHOWDF == TRUE) {
		dump_edges();
	}

	if(headers) {
		connect_preheaders();
		/*	now that we know how many new blocks we'll need ... */
		make_loop_tab();
		/*	dfo and dominator information needs to be recomputed
		**	to account for preheader blocks
		*/
		find_dfo();
		find_dominators();
	}
}

LOCAL
new_header(edgep,blocks)
EDGE *edgep;
LIST *blocks;
{
register LIST *new_edge, *lp;
register struct loop_list *hp;
register BLOCK *header = edgep->to;

	LFOR(lp,indirgoto_targets) {
		/* if a header is the target of an indirect goto ignore it -
		** otherwise we would have to change all the places where the headers's
		** label was assigned and this seems more trouble than its worth
		*/
		if(header == (BLOCK*) (LCAST(lp,TRIPLE)->left) ) {
			return;
		}
	}

	for(hp=headers; hp; hp=hp->next) {
		if(hp->header == header) break;
	}
	if(!hp) {
		hp  = (struct loop_list *) ckalloc(1,sizeof(struct loop_list));
		hp->header = header;
		hp->next = headers;
		headers = hp;
	}
	new_edge = NEWLIST(loop_lq);
	(EDGE*) new_edge->datap = edgep;
	LAPPEND(hp->edges,new_edge);
	LFOR(lp,blocks) {
		insert_list(&(hp->blocks),(BLOCK*) lp->datap,loop_lq);
	}
}

/* 
**	tests whether block number 1 dominate block number 2 
** 
*/
BOOLEAN
dominates(bn1,bn2)
int bn1, bn2;
{

	return(bit_test(dominate_set,bn2,bn1));
}

dump_loops()
{
register LIST *lp;
register LOOP *loop;

	printf("LOOPS:\n");
	LFOR(lp,loops) {
		loop = (LOOP*) lp->datap;
		print_loop(loop);
	}
}

print_loop(loop)
LOOP *loop;
{
register LIST *lp, *first;
register TRIPLE *tp;

		printf("loop %d preheader %d ", 
			loop->loopno, loop->preheader->blockno);
		LFOR(lp,loop->back_edges) {
			printf(" %d->%d, ", 
				LCAST(lp,EDGE)->from->blockno, LCAST(lp,EDGE)->to->blockno);
		}
		LFOR(lp,loop->blocks) {
			printf("B[%d]  ", LCAST(lp,BLOCK)->blockno);
		}
		printf("\n\t");
		if(loop->invariant_triples) {
			first = loop->invariant_triples;
			LFOR(lp,first) {
				tp = (TRIPLE*) lp->datap;
				printf("T[%d]%c  ", tp->tripleno, 
									((ISOP(tp->op,ROOT_OP)) ? 'r ' : ' '));
			}
		}
		printf("\n");
}

dump_edges()
{
static char *edgetypes[] = {
	"RETREATING", "RETREATING_BACK", "ADVANCING", "ADVANCING_TREE", "CROSS",
	"RETREATORCROSS"
};
register LIST *lp;
EDGE *ep;

	LFOR(lp,edges) {
		ep = (EDGE*) lp->datap;
		printf("%d -> %d %s\n",
		ep->from->blockno,ep->to->blockno,edgetypes[(int)ep->edgetype]);
	}
}

/* Algorithm 13.1 */
LOCAL 
make_loop(back_edge)
EDGE *back_edge;
{
LIST *loop_blocks = LNULL;
register LIST *lp, *lp2;
BLOCK *block, *m;
LOOP *loop;
LIST *stack;

	stack = LNULL;
	block = back_edge->to;
	block->loop_weight = LOOP_WEIGHT_FACTOR;/*later adjusted for nesting depth*/
	lp = NEWLIST(loop_lq);
	(BLOCK*) lp->datap = block;
	LAPPEND(loop_blocks,lp);
	
	block = back_edge->from; /* INSERT(n) */
	if(insert_list(&loop_blocks,block,loop_lq) == TRUE) {
		block->loop_weight = LOOP_WEIGHT_FACTOR;
		lp = NEWLIST(tmp_lq);
		(BLOCK*) lp->datap = block;
		LAPPEND(stack,lp);
	}

	while(stack != LNULL) {
		m = (BLOCK*) stack->datap;
		delete_list(&stack,stack);
		LFOR(lp2,m->pred) { /* foreach pred p do INSERT(p) */
			block = (BLOCK*) lp2->datap;
			if(insert_list(&loop_blocks,block,loop_lq) == TRUE) {
				block->loop_weight = LOOP_WEIGHT_FACTOR;
				lp = NEWLIST(tmp_lq);
				(BLOCK*) lp->datap = block;
				LAPPEND(stack,lp);
			}
		}
	} 
	
	new_header(back_edge,loop_blocks);

	empty_listq(tmp_lq);
}

LOCAL
make_loop_tab()
{
BLOCK *header, *preheader;
register BLOCK *bp, *succ;
register LIST *lp, *lp2, *lp3, *tmp_lp;
register char *loop_tab_index;
LOOP *loop, **loop_vec, *loop1, *loop2;
BLOCK *header1, *preheader1;
register int i;

	loop_vec = (LOOP**) ckalloca(sizeof(LOOP*)*nloops);
	loop_block_tab = (char*) ckalloc(nblocks*nloops,sizeof(char));
	LFOR(lp,loops) {
		loop = (LOOP*) lp->datap;
		loop_tab_index = &loop_block_tab[nblocks*loop->loopno];
		loop_vec[loop->loopno] = loop;

		LFOR(lp2,loop->blocks) {
			bp = (BLOCK*) lp2->datap;
			loop_tab_index[bp->blockno] = IS_IN;
		}
	}


	LFOR(lp,loops) {
		loop = (LOOP*) lp->datap;
		/*
		**	mark those blocks in the loop that are exits, ie have a successor
		**	outside the loop
		*/
		loop_tab_index = &loop_block_tab[nblocks*loop->loopno];
		LFOR(lp2,loop->blocks) {
			bp = (BLOCK*) lp2->datap;
			LFOR(lp3,bp->succ) {
				succ = (BLOCK*) lp3->datap;
				if( !INLOOP(succ->blockno,loop->loopno)) {
					loop_tab_index[bp->blockno] = IS_EXIT;
					break;
				}
			}
		}
	}

	/* decide what loops a preheader block belongs to .. */
	LFOR(lp,loops) {
		loop1 = (LOOP*) lp->datap;
		preheader1 = loop1->preheader;
		header1 = LCAST(loop1->back_edges,EDGE)->to;
		/* preheader1 belongs to all the loops header1 belongs to except
		** loops whose header is header1 (...because preheader1 is the immediate
		** dominator of header1)
		*/
		loop_tab_index = &loop_block_tab[header1->blockno];
		for(i=0; i < nloops; i++, loop_tab_index += nblocks) {
			if( *loop_tab_index  == IS_NOTIN)  /* header1 not in loop2 */
				continue;
			loop2 = loop_vec[i];
			if( LCAST(loop2->back_edges,EDGE)->to == header1 ) continue;
			loop_block_tab[(nblocks*loop2->loopno)+preheader1->blockno] = IS_IN;
			tmp_lp = NEWLIST(loop_lq);
			(BLOCK*) tmp_lp->datap = preheader1;
			LAPPEND(loop2->blocks,tmp_lp);
		}
	}
	if(SHOWDF) {
		dump_byte_tab("LOOP*BLOCK TAB",loop_block_tab,nloops,nblocks);
	}
}

LOCAL
connect_preheaders()
{
TRIPLE *ref_triple, *append_triple();
TYPE type;
LIST *lp, *lp2, *lp3, *pred_list;
LOOP *loop;
BLOCK *bp, *header, *preheader;
struct loop_list *hp;
EDGE *ep;

	/* for each header allocate a loop then */
	/* connect preheaders to headers and vice versa */
	for(hp = headers; hp; hp = hp->next ) {
		hp->preheader = preheader = new_block();
		preheader->labelno = new_label();
		loop = (LOOP*)	ckalloc(1,sizeof(LOOP));
		hp->loop = loop;
		loop->back_edges = hp->edges;
		loop->blocks = hp->blocks;
		loop->loopno = nloops++;
		loop->preheader = preheader;
		lp = NEWLIST(loop_lq);
		(LOOP*) lp->datap = loop;
		LAPPEND(loops,lp);

		header = hp->header;
		lp2 = NEWLIST(proc_lq);
		(BLOCK*) lp2->datap = header;
		LAPPEND(preheader->succ,lp2);

		lp2 = NEWLIST(proc_lq);
		(BLOCK*) lp2->datap = preheader;
		LAPPEND(header->pred,lp2);

		type.tword = UNDEF;
		type.aux.size = 0;
		preheader->last_triple = append_triple(preheader->last_triple,LABELDEF,
								(NODE*)preheader, (NODE*) NULL,type);
		ref_triple = append_triple(LNULL,LABELREF,(NODE*) header,
								(NODE*) ileaf(0), type);
		preheader->last_triple = append_triple(preheader->last_triple, GOTO,
							(NODE*)NULL, (NODE*)ref_triple, type);
	}

	/*
	**	then look at all predecessors of the header and make those not in
	**	the loop (other than the preheader) connect to the preheader
	*/
	for(hp = headers; hp; hp = hp->next ) {
		header = hp->header;
		preheader = hp->preheader;
		pred_list = LNULL; /* build a new list of predecessors */
		LFOR(lp,header->pred) {
			bp = (BLOCK*) lp->datap;
			if(bp == preheader) { /* the preheader is always on the pred_list*/
				lp3 = NEWLIST(proc_lq);
				(BLOCK*) lp3->datap = bp;
				LAPPEND(pred_list,lp3);
				goto next_pred;	
			} else { /* test if it's a back edge */
				loop = hp->loop;
				LFOR(lp2,loop->back_edges) {
					ep = (EDGE*) lp2->datap;
					if(bp == ep->from)  {
						/* if this edge is one of the back edges coming into the
						** loop leave it alone
						*/
						lp3 = NEWLIST(proc_lq);
						(BLOCK*) lp3->datap = bp;
						LAPPEND(pred_list,lp3);
						goto next_pred;	
					}
				}
				/* it's an edge to be redirected */
				redirect_flow(bp,header,preheader);
			}
		next_pred: ;
		}
		header->pred = pred_list;
	}
}

LOCAL
redirect_flow(from_bp,old_to_bp,new_to_bp)
BLOCK *from_bp, *old_to_bp, *new_to_bp;
{
LIST *lp, *lp2;
TRIPLE *labelref, *last;
BLOCK *bp;

	LFOR(lp,from_bp->succ) {
		bp = (BLOCK*) lp->datap;
		if(bp == old_to_bp)  {
			(BLOCK*) lp->datap = new_to_bp;
			last = from_bp->last_triple;
			if(ISOP(last->op,BRANCH_OP)) {
				switch(last->op) {
					case SWITCH :
					case CBRANCH :
					case INDIRGOTO :
					case REPEAT:
						TFOR(labelref,(TRIPLE*)last->right) {
							if( (BLOCK*)labelref->left == old_to_bp) {
								(BLOCK*)labelref->left = new_to_bp;
							}
						}
						break;
			
					case GOTO :
						labelref = (TRIPLE*) last->right;
						if( (BLOCK*)labelref->left == old_to_bp) {
							(BLOCK*)labelref->left = new_to_bp;
						} else {
							quit("redirect_flow(): bad labelref triple");
						}
						break;
				}
			} else {
				quita("redirect_flow(): connected by op >>%s<<",
					op_descr[ORD(last->op)].name);
			}
			lp2 = NEWLIST(proc_lq);
			(BLOCK*) lp2->datap =  from_bp;
			LAPPEND(new_to_bp->pred,lp2);
		}
	}
}
