#ifndef lint
static	char sccsid[] = "@(#)ln.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "iropt.h"
#include "loop.h"
#include <stdio.h>

LOOP_TREE *loop_tree_tab;
LOOP_TREE *loop_tree_root;
int n_tree_nodes;
static tree_depth;

/*
**	the loop tree encodes the loop nesting structure
**	ie loop1 is a child of loop2 if loop1 is entirely contained in loop2
**	the algorithm is to make a tree with all loops as children of a (fake)
**	root and then successively change siblings to children
**	overlapping loops ie neither nested nor disjoint are treated as disjoint
*/
extern LISTQ *loop_lq;

make_loop_tree()
{
register LIST *lp;
LOOP *loop;
register char * child_pt, *cp;
char *children;
LOOP_TREE *loop_tree_pt;

	/*
	**	allocate space for the tree in one swoop since the number of
	**	nodes doesn't change, children are encoded
	**	in a byte vector
	*/

	if(nloops <= 0) return;
	n_tree_nodes = nloops+1;
	loop_tree_tab = (LOOP_TREE*) ckalloc(n_tree_nodes,sizeof(LOOP_TREE));
	child_pt = children = ( char *) ckalloc(n_tree_nodes*n_tree_nodes,sizeof(char));

	/*
	**	root node initialized to no siblings and all nodes other than
	**	itself as children
	*/
	loop_tree_root = &loop_tree_tab[n_tree_nodes-1];
	loop_tree_root->children = child_pt;
	for(cp = child_pt; cp < &child_pt[nloops]; cp++) {
		*cp = 1;
	}
	child_pt += n_tree_nodes;

	/*
	**	all other nodes initialized to no children and all nodes other
	**	than the root as siblings
	*/
	loop_tree_pt = loop_tree_tab;
	LFOR(lp,loops) {
		loop = (LOOP*) lp->datap;
		loop_tree_pt->loop = loop;
		loop_tree_pt->parent = loop_tree_root;
		loop_tree_pt->children = child_pt;
		child_pt += n_tree_nodes;
		loop_tree_pt++;
	}

	visit_loop_tree(loop_tree_root);
	tree_depth = -1; 	/* initialize to -1 to account for the dummy root*/
	assign_loop_weight(loop_tree_root);
}

LOCAL BOOLEAN
is_loop_nested(inner,outer)
register LOOP *inner, *outer;
{
register EDGE *ep;
register LIST *lp;
	/*
	**	if, for all of inner's back edges,both vertices of the edge are in outer
	**	inner is contained in outer;
	*/
	LFOR(lp,inner->back_edges) {
		ep = (EDGE*) lp->datap;
		if( (!INLOOP(ep->from->blockno,outer->loopno))  ||
			(!INLOOP(ep->to->blockno,outer->loopno)) ) {
			return FALSE;
		}	
	}
	return TRUE;
}

visit_loop_tree(ltp)
LOOP_TREE *ltp;
{
register LOOP *loop1, *loop2;
register int i, ii;
register LOOP_TREE *ltp1, *ltp2;
BOOLEAN is_loop_nested();
static level =0;

	for(i=0; i<nloops; i++) if(ltp->children[i]) {
		ltp1 = loop_tree_tab + i;
		loop1 = ltp1->loop;
		for(ii=0; ii<nloops; ii++) if(ltp->children[ii] && i != ii) {
			ltp2 = loop_tree_tab + ii;
			loop2 = ltp2->loop;
			if(	is_loop_nested(loop2,loop1) == TRUE ) {
				ltp2->parent = ltp1;
				ltp->children[ii] = 0;
				ltp1->children[ii] = 1;
			}
		}
	}

	for(i=0; i<nloops; i++) if(ltp->children[i]) {
		visit_loop_tree(loop_tree_tab+i);
	}
}

assign_loop_weight(ltp)
LOOP_TREE *ltp;
{
register LOOP_TREE *ltp2;
register LIST *lp;
register BLOCK *block;
register LOOP *loop;
register char *cp;
register int weight;
register int i, nest_level;
#define MAX_NEST 5
/* weights for loops nested 0-MAX_NEST deep. if deeper,  set to MAX_NEST 
** to avoid overflow 
*/
static int loop_weights[]  = {
	1 , 
	LOOP_WEIGHT_FACTOR, 
	LOOP_WEIGHT_FACTOR * LOOP_WEIGHT_FACTOR , 
	LOOP_WEIGHT_FACTOR * LOOP_WEIGHT_FACTOR * LOOP_WEIGHT_FACTOR , 
	LOOP_WEIGHT_FACTOR * LOOP_WEIGHT_FACTOR * LOOP_WEIGHT_FACTOR * LOOP_WEIGHT_FACTOR , 
	LOOP_WEIGHT_FACTOR * LOOP_WEIGHT_FACTOR * LOOP_WEIGHT_FACTOR * LOOP_WEIGHT_FACTOR * LOOP_WEIGHT_FACTOR 
};


	tree_depth++;
	for(ltp2=loop_tree_tab,cp = ltp->children; 
		cp < &ltp->children[nloops]; cp++,ltp2++) {
		if (*cp) {
			assign_loop_weight(ltp2);
		}
	}
	loop = ltp->loop; 
	nest_level = tree_depth;
	if(nest_level > MAX_NEST ) nest_level = MAX_NEST;
	weight = loop_weights[nest_level];
	if(loop) {
		if(SHOWDF == TRUE) {
			printf("loopno %d depth %d\n",loop->loopno,tree_depth);
		}
		LFOR(lp,loop->blocks) {
			block = (BLOCK*) lp->datap;
			if(block->loop_weight < weight) block->loop_weight = weight;
		}
	}
	tree_depth --;
}
