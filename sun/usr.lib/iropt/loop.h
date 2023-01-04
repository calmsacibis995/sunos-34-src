/*	@(#)loop.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

typedef enum { RETREATING, RETREATING_BACK, ADVANCING, ADVANCING_TREE, 
CROSS, RETREATORCROSS } EDGETYPE;

#	define INLOOP(blockno,loopno) (loop_block_tab[(nblocks)*(loopno)+(blockno)])
#	define IS_IN  1
#	define IS_EXIT  2
#	define IS_EXIT_DOMINATOR  3
#	define IS_NOTIN 0

#	define LOOP_WEIGHT_FACTOR 8

typedef struct edge {
	EDGETYPE edgetype;
	BLOCK *from, *to;
} EDGE;

typedef struct loop {
	LIST *back_edges;
	int 	loopno;
	BLOCK	*preheader;
	LIST	*blocks;
	LIST	*invariant_triples;
} LOOP;

typedef struct loop_tree {
	struct loop_tree *parent;
	char *children;
	LOOP *loop;
} LOOP_TREE;

extern char *loop_block_tab;
extern LIST *loops, *edeges;
extern int nloops, n_tree_nodes;
extern LOOP_TREE *loop_tree_tab, *loop_tree_root;
BOOLEAN dominates();

typedef struct iv_info {
	BOOLEAN	is_rc;
	BOOLEAN	is_iv;
	int indx;
	LEAF *leafp;			/* x, the var in question */
	LIST * clist;		/* list of constants for which T(x*c) must be mantained */
	char	*afct;		/* the ivs and rcs which can affect the value of x */
	LIST * plus_temps;		/* list of "T(x*c) + L1 " temporaries */
	struct iv_info *next; /* structures are linked */
} IV_INFO;

typedef struct iv_hashtab_rec {
	LEAF *x, *c, *t;
	IR_OP op;
	TRIPLE *def_tp;
	LIST * update_triples;	/* triples that update the temp */
	struct iv_hashtab_rec *next;
} IV_HASHTAB_REC; 
extern IV_HASHTAB_REC **iv_hashtab;

# define	IV(leafp)	( (IV_INFO *) ((LEAF*) leafp)->visited )
# define IV_HASHTAB_SIZE 256
# define YES 1

extern LIST *cands;
extern IV_INFO *iv_info_list;
extern LIST *iv_def_list;
