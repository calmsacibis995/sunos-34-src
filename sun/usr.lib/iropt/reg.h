/*	@(#)reg.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


/*
 * Header file for register allocation.
 */

#define MAX_DREG 6			       /* save d0, d1 for CG */
#define MAX_AREG 4			       /* save a0, a1, a6, a7 for CG */
#define MAX_FREG 6			       /* save f0, f1 for CG */
#define MAX_FPAREG 12			       /* save fp0, fp1, fp2, fp3 for CG */
#define FIRST_DREG 7
#define FIRST_AREG 5
#define FIRST_FREG 7
#define FIRST_FPAREG 4

typedef enum
{
	A_TREE=0, D_TREE, F_TREE
} TREE;

typedef struct web
	{
		int web_no;
		LEAF *leaf;
		LIST *define_at, *use;
		int d_cont, a_cont, f_cont;
		struct web *same_reg;
		struct sort *sort_d, *sort_a, *sort_f;
		SET life_span, can_share;
		BOOLEAN import; /* used before defined */
		BOOLEAN short_life;  /* live in one block only */
	} WEB;

extern LIST *web_head, *web_tail;
extern int nwebs, n_dreg, n_areg, n_freg, max_freg;
extern int reg_life_wordsize;
extern int reg_share_wordsize;
extern LIST *sort_a_list, *sort_d_list, *sort_f_list;
extern BOOLEAN use68881, usefpa;
extern int fpa_base_reg, fpa_base_weight;

#define INIT_CONT -32	/* cycle time to save and resotre a register on */
			/* procedure entry and exit */
#define FINIT_CONT -60

typedef struct sort
	{
		int weight;
		WEB *web;
		struct sort *left, *right;
		int regno;
		LIST *lp;
	} SORT;

