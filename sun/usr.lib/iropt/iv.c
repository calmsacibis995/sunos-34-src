#ifndef lint
static	char sccsid[] = "@(#)iv.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Induction variable transformation - closely follows " An Algorithm for
 * Reduction of Operator Strength" Cocke & Kennedy - CACM  Nov 77, Vol 20 No 11
 */

#include "iropt.h"
#include "loop.h"
#include <stdio.h>

LIST *cands;
IV_INFO *iv_info_list;
LEAF ** ivrc_tab;
int n_iv, n_rc;
char *afct_tab;
LISTQ *iv_lq;
LIST *iv_def_list;
IV_HASHTAB_REC** iv_hashtab;
TRIPLE *add_triple();

#define T(x,c) hash(x,MULT,c)->t

LOCAL
compute_afct()
{
LEAF **ivp0, *targ;
int indx, row_bytesize, row_wordsize;
IV_INFO *ivp;
TRIPLE *def_tp, *add_op;
char *afct_p, *afct_i, *afct_j;
long *tmp;
BOOLEAN change;
int n, i, j;
LIST *lp;

	ivp0 = ivrc_tab = (LEAF**) ckalloc(n_iv + n_rc,sizeof(LEAF*));
	indx = 0;
	for(ivp=iv_info_list; ivp; ivp = ivp->next ) {
		if(ivp->is_iv == TRUE) {
			ivp->indx = indx++;
			*ivp0 = ivp->leafp;
			ivp0++;
		}
	}
	for(ivp=iv_info_list; ivp; ivp = ivp->next ) {
		if(ivp->is_rc == TRUE) {
			ivp->indx = indx++;
			*ivp0 = ivp->leafp;
			ivp0++;
		}
	}

	row_bytesize = roundup(n_iv + n_rc , sizeof(int)/sizeof(char));
	row_wordsize = row_bytesize/(sizeof(int)/sizeof(char));
	afct_tab = (char*) ckalloc(n_iv,row_bytesize);
	
	LFOR(lp, iv_def_list) {
		def_tp = (TRIPLE*) lp->datap;	
		targ = (LEAF*) def_tp->left;
		ivp = IV(targ);
		if(!ivp->afct) {
			ivp->afct = afct_tab + (ivp->indx*row_bytesize);
		}
		afct_p = IV(targ)->afct;
		afct_p[IV(targ)->indx] = YES; /* step A1 */
		if(def_tp->right->operand.tag == ISTRIPLE) { /* A2 and A3 */
			add_op = (TRIPLE*) def_tp->right;
			afct_p[IV(add_op->left)->indx] = YES; 
			if(ISOP(add_op->op,BIN_OP)) {
				afct_p[IV(add_op->right)->indx] = YES; 
			}
		} else {
			afct_p[IV(def_tp->right)->indx] = YES; 
		}
	}

	change = TRUE;
	tmp = (int *) ckalloca(row_wordsize*sizeof(int));
	while(change == TRUE) {
		change = FALSE;
		for(i=0, afct_i = afct_tab; i < n_iv; afct_i += row_bytesize, i++) {
			for(n = 0; n< row_wordsize; n++ ) {
				tmp[n] = ((int*)afct_i)[n];
			}
			for(j=0, afct_j=afct_tab;j < n_iv;afct_j += row_bytesize, j++){
				if(afct_i[j] == YES) {
					for(n = 0; n<row_wordsize; n++) {
						tmp[n] |= ((int*)afct_j)[n];
					}
				}
			}
			for(n = 0; n< row_wordsize; n++ ) {
				if(tmp[n] != ((int*)afct_i)[n]) {
					change=TRUE;
					break;
				}
			}
			if(change == TRUE) {
				for(n = 0; n< row_wordsize; n++ ) {
					((int*)afct_i)[n] |= tmp[n];
				}
			}
		}
	}
}

/* check that a definition of a possible iv is one of the legitimate forms */

BOOLEAN
check_definition(def)
register TRIPLE *def;
{
register TRIPLE *add_op;

	if(def->op == ASSIGN) {
		if(def->right->operand.tag == ISLEAF) {
			return TRUE;
		} else if(def->right->operand.tag == ISTRIPLE) {
			add_op = (TRIPLE*) def->right;
			switch(add_op->op) {
				case PLUS:
				case MINUS:
					if(	add_op->left->operand.tag == ISLEAF && 
						add_op->right->operand.tag == ISLEAF) {
							return TRUE;
					}
					break;

				case NEG:
					if(	add_op->left->operand.tag == ISLEAF ) {
						return TRUE;
					}
					break;
				}
		}
	}
	return FALSE;
}

find_iv(loop)
LOOP *loop;
{
LIST *lp;
LEAF *leafp;
BOOLEAN change;
TRIPLE *tp;
IV_INFO *ivp, ivp_next;
BLOCK *bp;
LEAF *leaf;
VAR_REF *rp;

	/*
	** if triple is a sto, neg, add or sub add targ to IV
	** the rc set consists of all vars with no defs in the region and all
	** consts that appear in a "possible iv" defining instruction 
	** this is done to keep from lugging all const leaves around
	*/
	LFOR(lp, loop->blocks) {
		bp = (BLOCK*) lp->datap;
		TFOR(tp, bp->last_triple) {
			if(tp->op == ASSIGN) {
				if(tp->right->operand.tag == ISLEAF) {
					if(ISCONST(tp->right)) set_rc(tp->right);
					set_iv(tp);
				} else if(tp->right->operand.tag == ISTRIPLE) {
					register TRIPLE *add_op;
					add_op = (TRIPLE*) tp->right;
					switch(add_op->op) {
						case PLUS:
						case MINUS:
							if(	add_op->left->operand.tag == ISLEAF && 
								add_op->right->operand.tag == ISLEAF) {

								if(ISCONST(add_op->left)) set_rc(add_op->left);
								if(ISCONST(add_op->right))set_rc(add_op->right);
								set_iv(tp);
							}
							break;
						case NEG:
							if(	add_op->left->operand.tag == ISLEAF ) {
								if(ISCONST(add_op->left)) set_rc(add_op->left);
								set_iv(tp);
							}
							break;
					}
				}
			}
		}
	}

	/*
	**	ensure that all definitions in the block are of the allowed
	**	form
	*/
	LFOR(lp, iv_def_list) {
		tp = (TRIPLE*) lp->datap;
		if(ivp = IV(tp->left) ) { /* possible iv */
			leafp = (LEAF*) tp->left;
			rp = leafp->references;
			while(rp) {
				if( (rp->reftype == VAR_DEF ||rp->reftype == VAR_AVAIL_DEF) &&
					(INLOOP(rp->site.bp->blockno, loop->loopno))
				) {
					if(check_definition(rp->site.tp) == FALSE) {
						unset_iv(ivp);
						break;
					}
				}
				rp = rp->next_vref;
			}
		}
	}


	/*
	**	if a1 not in IV U RC or a2 not in IV U RC remove targ from IV
	*/
	change = TRUE;
	while(change) {
		change = FALSE;
		LFOR(lp, iv_def_list) {
			tp = (TRIPLE*) lp->datap;
			if(ivp = IV(tp->left) ) { /* possible iv */
				if(tp->right->operand.tag == ISLEAF) {
					if((!IV(tp->right) || IV(tp->right)->is_rc )) {
						/* assign by a RC is a RC NOT an IV */
						/* a1 not in IV */
						change = TRUE;
						unset_iv(ivp);
					}
				} else if(tp->right->operand.tag == ISTRIPLE) {
					TRIPLE *add_op;
					add_op = (TRIPLE*) tp->right;
					switch(add_op->op) {
						case PLUS:
						case MINUS:
							if(!IV(add_op->left)){
								change = TRUE;
								unset_iv(ivp);
								break;
							}
							if(!IV(add_op->right)){
								change = TRUE;
								unset_iv(ivp);
								break;
							}
							if(IV(add_op->left)->is_iv == TRUE &&
							   IV(add_op->right)->is_iv == TRUE){
								change = TRUE;
								unset_iv(ivp);
							}
							break;

						case NEG:
							if(!IV(add_op->left)){
								change = TRUE;
								unset_iv(ivp);
							}
							break;
					}
				}
			}
		}
	}
	cleanup_iv_list();
	cleanup_iv_def_list();
}

LOCAL
unset_iv(ivp)
register IV_INFO *ivp;
{
	if(ivp->is_iv) {
		IV(ivp->leafp) = NULL; 
		n_iv --; 
		ivp->is_iv = FALSE;
	}
}

LOCAL
set_iv(def_tp)
TRIPLE *def_tp;
{
register LEAF *targ;
register IV_INFO *ivp;
register LIST *lp;

	lp = NEWLIST(iv_lq);
	(TRIPLE*) lp->datap = def_tp;
	LAPPEND(iv_def_list,lp);
	targ = (LEAF*) def_tp->left;

	if( !(ivp = IV(targ)) ) {
		ivp = (IV_INFO*) ckalloc(1,sizeof(IV_INFO));
		IV(targ) = ivp;
		ivp->leafp = targ;
		ivp->next = iv_info_list;
		iv_info_list = ivp;
		n_iv++;
	} else if(ivp->is_rc == TRUE) {
		quit("set_iv: possible induction var marked as region const");
	}
	ivp->is_iv =  TRUE;
}

set_rc(leafp)
LEAF *leafp;
{
register IV_INFO *ivp;

	if( !(ivp = IV(leafp)) ) {
		ivp = (IV_INFO*) ckalloc(1,sizeof(IV_INFO));
		IV(leafp) = ivp;
		ivp->leafp = leafp;
		ivp->next = iv_info_list;
		iv_info_list = ivp;
		n_rc++;
	}
	ivp->is_rc =  TRUE;
}

/* the iv_info list now contains leaves which were found not to */
/* be ivs - unlist them */
LOCAL
cleanup_iv_list()
{ 
register IV_INFO *ivp, *next_ivp;

	while( iv_info_list && 
				iv_info_list->is_rc == FALSE && iv_info_list->is_iv == FALSE) {
		IV(iv_info_list->leafp) = NULL;
		iv_info_list = iv_info_list->next;
	}
	ivp = iv_info_list;
	if(ivp) {
		while(ivp->next) {
			next_ivp = ivp->next;
			if(next_ivp->is_rc == FALSE && next_ivp->is_iv == FALSE) {
				ivp->next = next_ivp->next;
				IV(next_ivp->leafp) = NULL;
			} else {
				ivp = ivp->next;
			}
		}
	}
}

LOCAL
cleanup_iv_def_list()
{ 
register LIST *lp, *tmp, *new_list;
register LEAF * targ;

	new_list = LNULL;
	LFOR(lp, iv_def_list) {
		targ = (LEAF*) LCAST(lp,TRIPLE)->left;
		if( (!IV(targ)) || IV(targ)->is_rc == TRUE) {
			continue;
		} else {
			tmp = NEWLIST(iv_lq);
			(TRIPLE*) tmp->datap = (TRIPLE*) lp->datap;
			LAPPEND(new_list,tmp);
		}
	}
	iv_def_list = new_list;
}

find_cands(loop)
LOOP *loop;
{
register LIST *lp, *new_cand;
register TRIPLE *tp;
register BLOCK *bp;
register int left, right;

	cands = LNULL;
	LFOR(lp, loop->blocks) {
		bp = (BLOCK*) lp->datap;
		TFOR(tp, bp->last_triple) {
			if(tp->op == MULT && B_ISINT(tp->type.tword) ) {
				if(	tp->left->operand.tag == ISLEAF ) {
					if(IV(tp->left)) {
						left = (IV(tp->left)->is_iv == TRUE ? 1 : 0 );
					} else if(ISCONST(tp->left)) {
						left = 0;
					} else {
						continue;
					}
				} else {
					continue;
				}

				if(	tp->right->operand.tag == ISLEAF ) {
					if(IV(tp->right)) {
						right = (IV(tp->right)->is_iv == TRUE ? 1 : 0 );
					} else if(ISCONST(tp->right)) {
						right = 0;
					} else {
						continue;
					}
				} else {
					continue;
				}

				if( left ^ right ) { /* left or right must be iv but not both */
					new_cand = NEWLIST(iv_lq);
					(TRIPLE*) new_cand->datap = tp;
					LAPPEND(cands,new_cand);
					if(ISCONST(tp->left)) set_rc(tp->left);
					if(ISCONST(tp->right)) set_rc(tp->right);
				}
			}
		}
	}
}

iv_init()
{
LIST **lpp, *hash_link, *lp;
LEAF *leafp;

	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(hash_link, *lpp) {
			leafp = (LEAF*)hash_link->datap;
			IV(leafp) = NULL;
		}
	}
	iv_def_list = LNULL;
	iv_info_list = (IV_INFO*) NULL;
	n_iv = n_rc = 0;
	iv_lq = new_listq();
	iv_hashtab = (IV_HASHTAB_REC**) ckalloc(IV_HASHTAB_SIZE,sizeof(IV_HASHTAB_REC*));
}

do_induction_vars(loop)
LOOP *loop;
{
	find_iv(loop);
	find_cands(loop);
	compute_afct();
	doiv_r23();
	if(SHOWCOOKED == TRUE) {
		dump_ivs();
	}
	doiv_r45(loop);
	doiv_r67(loop);
	doiv_r89(loop);
	dotest_repl(loop);
}

dotest_repl(loop)
LOOP *loop;
{
register LIST *lp;
LEAF *c, *x, *k, **xp, **kp, *base;
TRIPLE *tp, *def_tp;
register BLOCK *bp;
IV_HASHTAB_REC *htrp, *hash(), *new_iv_hashtab_rec();

	LFOR(lp, loop->blocks) {
		bp = (BLOCK*) lp->datap;
		TFOR(tp, bp->last_triple) {
			switch(tp->op) {
				case EQ:
				case NE:
				case LE:
				case LT:
				case GE:
				case GT:
					if( tp->left->operand.tag == ISLEAF &&
						tp->right->operand.tag  == ISLEAF &&
						B_ISINT(tp->left->leaf.type.tword) &&
						B_ISINT(tp->right->leaf.type.tword) 
					) {
						xp = (LEAF**) &(tp->left);
						kp = (LEAF**) &(tp->right);
						if(IV(*xp) && IV(*xp)->is_iv == TRUE) {
							x= *xp; k = *kp;
						} else if(IV(*kp) && IV(*kp)->is_iv == TRUE) {
							xp = (LEAF**) &(tp->right);
							kp = (LEAF**) &(tp->left);
							x= *xp; k = *kp;
						} else {
							continue;
						}
						if(	((IV(k) && IV(k)->is_rc== TRUE)) || (ISCONST(k)) ) {
							if(IV(x)->clist == LNULL) {
								continue;
							}
							if(NULL && IV(x)->plus_temps != LNULL) { /*FIXME */
								htrp=(IV_HASHTAB_REC*)IV(x)->plus_temps->datap;
								*xp = htrp->t; 
								base = htrp->c;

							/* the temp for c*k will be modified to c*k +base.
							** To ensure it's not used anywhere else, a new temp
							** is created and the op is altered
							*/
								htrp = new_iv_hashtab_rec(c,MULT,k,loop);
								htrp->op = LEAFOP;
								def_tp = htrp->def_tp;
								if(def_tp != TNULL) {
									def_tp->right = (NODE*)
										add_triple(def_tp->right,PLUS, 
												def_tp->right,base, base->type);
								} else { /* c*k folded */
									TYPE type;
									TRIPLE *after;
									LEAF *new_temp(), *t;

									type.tword = UNDEF;
									type.aux.size = 0;
									after = loop->preheader->last_triple->tprev;
									after = add_triple(after,PLUS, htrp->t, 
															base, base->type);
									if(after->expr && after->expr->save_temp) {
										t = after->expr->save_temp;
									} else {
										t = new_temp(after);
										if(after->expr) {
											tp->expr->save_temp = t;
										}
									}
									after = add_triple(after,ASSIGN,
															t, after, type);
									htrp->def_tp = after;
								}
								*kp = htrp->t;
							} else {
								c = (LEAF*) IV(x)->clist->datap;
								*xp = T(x,c);
								if(	(htrp = hash(c,MULT,k)) == 
											(IV_HASHTAB_REC*) NULL &&
									(htrp = hash(k,MULT,c)) == 
											(IV_HASHTAB_REC*) NULL
								) {
									htrp = new_iv_hashtab_rec(c,MULT,k,loop);
								}
								*kp = htrp->t;
							}
						}
					}
					break;
			}
		}
	}
}
     
print_iv(ivp)
IV_INFO *ivp;
{
char *cp;
int i;
LIST *lp, *clist;

	printf("%d L[%d] %s %s next %d", ivp->indx, ivp->leafp->leafno,
		(ivp->is_rc ? "is_rc" : "" ) , (ivp->is_iv ? "is_iv" : "" ) , 
		(ivp->next ? ivp->next->indx : -1 )
	);
	if(ivp->afct) {
		printf("afct ");
		for(i=0,cp=ivp->afct; i< (n_iv+n_rc); i++,cp++) {
			if(*cp == YES) printf("%d ",i);
		}
	}
	if(ivp->clist != LNULL) {
		printf("clist ");
		clist = ivp->clist;
		LFOR(lp,clist) {
			printf("L[%d] ",LCAST(lp,LEAF)->leafno); 
		}
	}
	putchar('\n');
}

dump_ivs()
{
LIST **lpp, *hash_link, *lp;
LEAF *leafp;
IV_INFO *ivp;
TRIPLE *cand;

	printf("IV summary\n");
	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(hash_link, *lpp) {
			leafp = (LEAF*)hash_link->datap;
			if(ivp = IV(leafp)){
				print_iv(ivp);
			}
		}
	}
	if(cands != LNULL) {
		printf("cands:");
		LFOR(lp,cands) {
			cand = (TRIPLE*) lp->datap;
			printf(" T[%d](L[%d]*L[%d]) ",
				cand->tripleno, cand->left->leaf.leafno,
				cand->right->leaf.leafno);
		}
		putchar('\n');
	}
	if(iv_def_list != LNULL) {
		printf("iv_def_list:");
		LFOR(lp,iv_def_list) {
			printf(" T[%d]",LCAST(lp,TRIPLE)->tripleno);
		}
	}
	putchar('\n');
}

doiv_r23()
{
register LIST *lp;
register char *cp;
register int i, lim;
LEAF *x, *y, *c;
TRIPLE *cand;

	lim = n_iv + n_rc;
	LFOR(lp,cands) {
		cand = (TRIPLE*) lp->datap;
		if(IV(cand->left)->is_iv == TRUE) {
			x = (LEAF*) cand->left;
			c = (LEAF*) cand->right;
		} else {
			x = (LEAF*) cand->right;
			c = (LEAF*) cand->left;
		}

		for(i=0, cp = IV(x)->afct; i < lim ;  i++, cp ++) {
			if(*cp == YES) {
				y = ivrc_tab[i];
				insert_list(&(IV(y)->clist), c, iv_lq);
			}
		}
	}
}

doiv_r45(loop)
LOOP *loop;
{
LEAF *x, *c;
int i, lim;
LIST *cx, *lp;

	/*
	**	Steps R4 and R5 : For each x in IV U RC; for each c in C(x) :
	**	append temp(x,c) = x * c to the loop preheader and enter
	**	the temp in a hash table where it can be addressed by (x,c)
	*/
	lim = n_iv + n_rc;
	for(i=0; i< lim; i++) {
		x = ivrc_tab[i];
		cx = IV(x)->clist;
		LFOR(lp,cx) {
			c = (LEAF*) lp->datap;
			new_iv_hashtab_rec(x,MULT,c,loop);
		}
	}
}

LOCAL IV_HASHTAB_REC *
hash(x,op,c)
register LEAF *x, *c;
IR_OP op;
{
register IV_HASHTAB_REC *htrp;
register int key;

	key = ( ((int)x ^ (int)c) & ( 0xff00 ) ) >> 8;
	for(htrp = iv_hashtab[key]; htrp; htrp = htrp->next) {
		if(htrp->x == x && htrp->c == c && htrp->op == op) {
			return htrp;
		}
	}
	return (IV_HASHTAB_REC*) NULL;
}

doiv_r67(loop)
LOOP *loop;
{
LEAF *c, *targ;
TRIPLE *def_tp, *after;
LIST *clist, *lp, *lp2, *new_l;
TYPE type;
IV_HASHTAB_REC *tc_hr, *hash();

	type.tword = UNDEF;
	type.aux.size = 0;
	LFOR(lp,iv_def_list) {
		def_tp = (TRIPLE*) lp->datap;
		targ = (LEAF*) def_tp->left;
		if(IV(targ)->clist == LNULL) {
			continue;	
		}
		clist = IV(targ)->clist;
		after = def_tp;
		LFOR(lp2, clist) {
			c = (LEAF*) lp2->datap;
			tc_hr = hash(targ,MULT,c);
			if(def_tp->right->operand.tag == ISLEAF) {
				after = add_triple(after, ASSIGN, 
							tc_hr->t, T(def_tp->right,c), type);
			} else if(def_tp->right->operand.tag == ISTRIPLE) {
				TRIPLE *add_op;
				add_op = (TRIPLE*) def_tp->right;
				switch(add_op->op) {
					case PLUS:
					case MINUS:
						after = add_triple(after, add_op->op, 
									T(add_op->left,c), T(add_op->right,c),
									add_op->type);
						after = add_triple(after, ASSIGN, 
									tc_hr->t, after, type);
						break;

					case NEG:
						after = add_triple(after, NEG, 
									T(add_op->left,c), (NODE*) NULL,
									add_op->type);
						after = add_triple(after, ASSIGN, 
									tc_hr->t, after, type);
						break;

					default:
						quit("doiv_r67: bad op in iv_def_list");
				}
			}
			new_l = NEWLIST(iv_lq);
			(TRIPLE*) new_l->datap = after;
			LAPPEND(tc_hr->update_triples,new_l);
		}
	}
}

TRIPLE *
replace_operand(old_operand,with)
NODE *old_operand;
LEAF *with;
{
TRIPLE *parent, *find_parent(), *tp;

	parent = find_parent(old_operand);
	if(ISOP(parent->op,UN_OP)) {
		(LEAF*) parent->left = with;
	} else if(parent->left == old_operand) {
		(LEAF*) parent->left = with;
	} else {
		(LEAF*) parent->right = with;
	}
	if(old_operand->operand.tag == ISTRIPLE) {
		tp = (TRIPLE*) old_operand;
		tp->tprev->tnext = tp->tnext;
		tp->tnext->tprev = tp->tprev;
	}
	return parent;
}

doiv_r89(loop)
LOOP *loop;
{
LEAF *c, *x, *cand_sibling;
TRIPLE *cand, *parent, *grandparent, *def_tp;
LIST *lp, *lp2, *new_l;
IV_HASHTAB_REC *hr_mult, *hr_plus, *hash();

	LFOR(lp,cands) {
		cand = (TRIPLE*) lp->datap;
		if(IV(cand->left)->is_iv == TRUE) {
			x = (LEAF*) cand->left;
			c = (LEAF*) cand->right;
		} else {
			x = (LEAF*) cand->right;
			c = (LEAF*) cand->left;
		}
		hr_mult = hash(x,MULT,c);
		parent = replace_operand(cand, hr_mult->t);
		if(parent->op == PLUS) {
			if(parent->left == (NODE*) hr_mult->t) {
				if(	parent->right->operand.tag == ISLEAF &&
					( 
						((IV(parent->right) && IV(parent->right)->is_rc== TRUE))
						||
					    (ISCONST(parent->right)) 
					)){
					cand_sibling = (LEAF*) parent->right;
				} else {
					continue;
				}
			} else {
				if(	parent->left->operand.tag == ISLEAF &&
					( 
						((IV(parent->left) && IV(parent->left)->is_rc== TRUE))
						||
					    (ISCONST(parent->left))
					)){
					cand_sibling = (LEAF*) parent->left;
				} else {
					continue;
				}
			}
			if( ((hr_plus = hash(hr_mult->t,PLUS,cand_sibling)) == 
											(IV_HASHTAB_REC*) NULL)
			) {
				hr_plus = new_iv_hashtab_rec(hr_mult->t,PLUS,cand_sibling,loop);
				new_l = NEWLIST(iv_lq);
				(IV_HASHTAB_REC*) new_l->datap = hr_plus;
				LAPPEND(IV(x)->plus_temps,new_l);
				update_plus_temp(hr_mult,hr_plus,cand_sibling);
			}
			grandparent = replace_operand(parent,hr_plus->t);
		}
	}
}

/* insert updates for a new plus temp (C1+X*C2) everywhere (X*C2) is updated */
LOCAL
update_plus_temp(hr_mult,hr_plus,cand_sibling)
IV_HASHTAB_REC *hr_mult, *hr_plus;
LEAF *cand_sibling;
{
LIST *lp, *update_list;
TRIPLE *def_tp, *after;
TYPE type;
LEAF *incr;

	type.tword = UNDEF;
	type.aux.size = 0;
	update_list = hr_mult->update_triples;
	LFOR(lp, update_list) {
		after = def_tp = (TRIPLE*) lp->datap;
		if(def_tp->right->operand.tag == ISLEAF) { 
			/* t1 <- t2  : recompute (t1+L1) */
			after = add_triple(after, PLUS, hr_mult->t,
						cand_sibling,  cand_sibling->type);
			after = add_triple(after, ASSIGN, 
						hr_plus->t, after, type);
		} else if( def_tp->right->operand.tag == ISTRIPLE) {
			TRIPLE *add_op;
			add_op = (TRIPLE*) def_tp->right;
			switch(add_op->op) {
				case PLUS:
				case MINUS:
					if(hr_mult->t == (LEAF*) add_op->left) {
						/* t1 <- t1 + k  : update (t1+L1) */
						incr = (LEAF*) add_op->right;
					} else if(hr_mult->t == (LEAF*) add_op->right) {
						incr = (LEAF*) add_op->left;
					} else {
						/* t1 <- t2 + k  : recompute (t1+L1) */
						after = add_triple(after, PLUS, hr_mult->t,
									cand_sibling,  cand_sibling->type);
						after = add_triple(after, ASSIGN, 
									hr_plus->t, after, type);
						break;
					}
					after = add_triple(after, add_op->op, 
								hr_plus->t, incr, add_op->type);
					after = add_triple(after, ASSIGN, 
								hr_plus->t, after, type);
					break;

				case NEG:
					if(hr_mult->t == (LEAF*) add_op->left) {
						/* t1 <- -t1 : change sign of (t1+L1) */
						after = add_triple(after, NEG, 
									hr_plus->t, (NODE*) NULL, add_op->type);
						after = add_triple(after, ASSIGN, 
									hr_plus->t, after, type);
					} else {
						/* t1 <- -t2 : recompute (t1+L1) */
						after = add_triple(after, PLUS, hr_mult->t,
									cand_sibling,  cand_sibling->type);
						after = add_triple(after, ASSIGN, 
									hr_plus->t, after, type);
					}
					break;

				default:
					quit("update_plus_temp: bad op in update_triples list");
			}
		}
	}
}

TRIPLE *
find_parent(def_tp) 
register TRIPLE *def_tp;
{
register TRIPLE *use_tp, *right, *argp;

	TFOR(use_tp,def_tp) {
		if(ISOP(use_tp->op, USE1_OP) && (TRIPLE*) use_tp->left == def_tp) {
			return use_tp;
		}else if(ISOP(use_tp->op,USE2_OP) && (TRIPLE*)use_tp->right == def_tp){
			return use_tp;
		} else if( ISOP(use_tp->op, NTUPLE_OP)) {
			right = (TRIPLE*) use_tp->right;
			if(right != TNULL) TFOR( argp, right ) {
				if( (TRIPLE*) argp->left == def_tp) {
					return argp;
				}
			}
		}
	}
	quita("find_parent: couldn't find parent of T[%d]", def_tp->tripleno);
}

LOCAL IV_HASHTAB_REC *
new_iv_hashtab_rec(x,op,c,loop)
IR_OP op;
LEAF *x, *c;
LOOP *loop;
{
IV_HASHTAB_REC *htrp;
LEAF *t, *ileaf(), *new_temp();
TRIPLE *after, *new_t;
int val, key;
TYPE temp_type(), type;

	htrp = (IV_HASHTAB_REC*) ckalloc(1,sizeof(IV_HASHTAB_REC));
	htrp->x = x;
	htrp->c = c;
	htrp->op = op;
	if( ISCONST(x) && ISCONST(c) && 
		(BTYPE(x->type.tword)== x->type.tword && B_ISINT(x->type.tword))
		&&
		(BTYPE(c->type.tword)== c->type.tword && B_ISINT(c->type.tword))
	  ) 
	{ /* can fold */
		if(op == MULT) {
			val = x->val.const.c.i  * c->val.const.c.i;
		} else if(op == PLUS) {
			val = x->val.const.c.i  + c->val.const.c.i;
		} else {
			quit("new_iv_hashtab_rec: bad op");
		}
		htrp->t = ileaf(val);
		htrp->def_tp == TNULL;
	} else { /* really need a triple */
		after = loop->preheader->last_triple->tprev; /*before goto*/
		/* make sure the temp if of type pointer if any of its operand are */
		type = temp_type(x->type, c->type);
		new_t = add_triple(after, op, x , c, type);
		if(new_t->expr && new_t->expr->save_temp) {
			t = new_t->expr->save_temp;
		} else {
			t = new_temp(new_t);
			if(new_t->expr) {
				new_t->expr->save_temp = t;
			}
		}
		htrp->t = t;
		htrp->def_tp = add_triple(new_t, ASSIGN, t, new_t, type);
	}
	key = ( ((int)x ^ (int)c ) & ( 0xff00 ) ) >> 8;
	htrp->next = iv_hashtab[key];
	iv_hashtab[key] = htrp;
	return htrp;
}

/* determine type of temp to hold l op r ; check for 
** (ARY of BTYPE) + INT -> * to BTYPE by coverting ARYs to PTRs
*/
TYPE
temp_type(l,r)
TYPE l,r;
{
TYPE t;
BOOLEAN same_irtype(), isptr;
register unsigned long tword, btype;

	if( same_irtype(l, r) == TRUE) {
		return l;
	}

	if( BTYPE(l.tword) == l.tword && B_ISINT(l.tword) ) {
		l = r;
	}

	tword = l.tword;
	btype = BTYPE(tword);
	isptr = FALSE;
	for(;; tword = DECREF(tword) ) {
		if( ISPTR(tword) ) {
			isptr = TRUE;
			btype = INCREF(btype);
		} else if( ISARY(tword)) {
		} else if( ISFTN(tword)) { /* INCFTN...*/
			btype = ((( (btype)&~BTMASK)<<TSHIFT)|FTN|( (btype)&BTMASK));
		} else {
			break;
		}

	}
	if(isptr) {
		t.tword = btype;
		t.aux.size = sizeof(char*);
	} else {
		t = l;
	}
	return t;
}

/*make sure that whenever we generate a new triple we update the expr hash tab*/
LOCAL TRIPLE *
add_triple(after,op,left,right,type)
register TRIPLE *after;
register IR_OP op;
register NODE *left, *right;
TYPE type;
{
TRIPLE *append_triple();
EXPR *find_expr();
register TRIPLE *new_triple;

	new_triple = append_triple(after,op,left,right,type);
	if( ISOP(op,VALUE_OP) ) {
		new_triple->expr = find_expr(new_triple);
	}
	return new_triple;
}
