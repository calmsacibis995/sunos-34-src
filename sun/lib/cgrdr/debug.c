#ifndef lint
static	char sccsid[] = "@(#)debug.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

# include "cg_ir.h"
# include <stdio.h>

static char * base_type_names[] = {
"undef", "farg", "char", "short", "int", "long", "float", "double",
"strty", "unionty", "enumty", "moety", "uchar", "ushort", "unsigned",
"ulong", "bool", "extendedf", "complex", "dcomplex", "string", "labelno", "void"
};

static char * seg_content_names[] = {
	"REG_SEG", "STG_SEG", "TEXT_SEG"
};

print_type(type)
TYPE type;
{
BOOLEAN isptr;
TWORD t;
LIST *lp;
	
	t=type.tword;
	isptr=FALSE;
	for(;; t = DECREF(t) ){
			if( ISPTR(t) ) {
				printf( " *to " );
				isptr=TRUE;
			}
			else if( ISFTN(t) ) printf( " f() ret " );
			else if( ISARY(t) ) printf( " [] of " );
			else {
				printf( " %s ", base_type_names[t]);
				return;
			}
	}
}

print_address(ap)
ADDRESS *ap;
{
SEGMENT *sp;

	sp=ap->seg;
	printf("%s",sp->name);
	printf("(%d,%d) ", sp->base,ap->offset);
	if(sp->descr.class == DATA_SEG) printf(" label %d",ap->labelno);
}

print_leaf(lp)
LEAF *lp;
{
register LIST *listp;

		printf("[%d] %s size %d", lp->leafno, (lp->pass1_id ? lp->pass1_id : "\"\""),lp->type.aux.size );
		print_type(lp->type);
		switch(lp->class) {
			case ADDR_CONST_LEAF:
				printf("ADDR_CONST ");
				print_address(&lp->val.addr);
				break;

			case VAR_LEAF:
				printf("VAR ");
				print_address(&lp->val.addr);
				break;

			case CONST_LEAF:
				printf(" CONST ");
				if(ISFTN(lp->type.tword)) {
					printf("%s",lp->val.const.c.cp);
				} else switch(BTYPE(lp->type.tword)) {
					case CHAR:
					case STRING:
						printf("%s",lp->val.const.c.cp);
						break;
					case LABELNO:
					case SHORT:
					case INT:
					case LONG:
						printf("%d",lp->val.const.c.i);
						break;
					case FLOAT:
						if(lp->val.const.isbinary == TRUE) {
							printf("0x%X", l_align(lp->val.const.c.bytep[0]));
						} else {
							printf("%s",lp->val.const.c.fp[0]);
						}
						break;
					case DOUBLE:
						if(lp->val.const.isbinary == TRUE) {
							printf("0x%X,0x%X", 
							 l_align(lp->val.const.c.bytep[0]),
							 l_align(lp->val.const.c.bytep[0]+4));
						} else {
							printf("%s",lp->val.const.c.fp[0]);
						}
						break;
					case COMPLEX:
						if(lp->val.const.isbinary == TRUE) {
							printf("(0x%X,0x%X)", 
							 l_align(lp->val.const.c.bytep[0]),
							 l_align(lp->val.const.c.bytep[1]));
						} else {
							printf("(%s,%s)",
								lp->val.const.c.fp[0],lp->val.const.c.fp[1]);
						}
						break;
					case DCOMPLEX:
						if(lp->val.const.isbinary == TRUE) {
							printf("(0x%X,0x%X , 0x%X,0x%X)", 
							 l_align(lp->val.const.c.bytep[0]),
							 l_align(lp->val.const.c.bytep[0]+4),
							 l_align(lp->val.const.c.bytep[1]),
							 l_align(lp->val.const.c.bytep[1]+4));
						} else {
							printf("(%s,%s)",
								lp->val.const.c.fp[0],lp->val.const.c.fp[1]);
						}
						break;
					case BOOL:
						printf("%s",(lp->val.const.c.i ? "true" : "false" ));
						break;
					default:
						printf("unknown leaf constant type");
						break;
				}
				break;
			default:
				printf(" unknown leaf class");
				break;
		}
		if(lp->overlap) {
			printf(" overlap: ");
			LFOR(listp,lp->overlap) {
				printf("L[%d] ", LCAST(listp,LEAF)->leafno);
			}
		}
# ifdef IROPT
{
VAR_REF *vrp;
		if(lp->references) {
			printf(" references: ");
			for(vrp = lp->references; vrp ; vrp = vrp->next_vref) {
				printf("%d ", vrp->refno);
			}
		}
		if(lp->dependent_exprs) {
			printf(" dependent_exprs: ");
			LFOR(listp,lp->dependent_exprs->next) {
				printf("E[%d] ", LCAST(listp,EXPR)->exprno);
			}
		}
}
# endif
		printf("\n");
}

print_triple(tp, indent)
register TRIPLE *tp;
int indent;
{
register LIST *lp;
register TRIPLE *tlp;
int i;

	for(i=0;i<indent;i++){
		printf("\t");
	}

	printf("[%d] ", tp->tripleno );

	if( ISOP(tp->op,VALUE_OP)) {
		print_type(tp->type);
	} else {
		printf("\t");
	}
	printf("%s ",op_descr[ORD(tp->op)].name);
	if(tp->op == PASS) {
		printf(" %s\n",tp->left->leaf.val.const.c.cp);
	} else if(tp->op == GOTO) {
		TRIPLE *labelref = (TRIPLE*) tp->right;
		if(labelref) {
			if(labelref->left->operand.tag == ISBLOCK) {
				printf(" B[%d]\n",((BLOCK*)labelref->left)->blockno);
			} else {
				printf(" L[%d]\n",((LEAF*)labelref->left)->leafno);
			}
		} else {
			printf("exit\n");
		}
	} else {
		if(tp->left)  switch(tp->left->operand.tag) {
			case ISTRIPLE:
				printf("T[%d]", tp->left->triple.tripleno );
				break;
			case ISLEAF:
				printf("%s L[%d]",
					(((LEAF*)tp->left)->pass1_id ?((LEAF*)tp->left)->pass1_id : ""),
					((LEAF*)tp->left)->leafno);
				break;
			case ISBLOCK:
				printf("B[%d]", ((BLOCK*)tp->left)->blockno);
				break;
			default:
				printf("print_triple: bad left operand");
				break;
		}
		if(tp->right) switch(tp->right->operand.tag) {
			case ISLEAF:
				printf("%s L[%d]",
					(((LEAF*)tp->right)->pass1_id ? ((LEAF*)tp->right)->pass1_id : ""),
					((LEAF*)tp->right)->leafno);
				break;
			case ISTRIPLE:
				if(ISOP(tp->op,BRANCH_OP)) {
					printf(" labels:");
					TFOR(tlp, (TRIPLE*) tp->right){
						if(tlp->left->operand.tag == ISBLOCK) {
							printf(" B[%d] if ",((BLOCK*)tlp->left)->blockno);
						} else {
							printf(" L[%d] if ",((LEAF*)tlp->left)->leafno);
						}
						printf("L[%d]", ((LEAF*)tlp->right)->leafno);
					}
				} else if(tp->op == SCALL || tp->op == FCALL) {
					if(tp->right) {
						printf("args :\n");
						TFOR(tlp,tp->right->triple.tnext) {
							print_triple(tlp,indent+2);
						}
					}
				} else {
					printf("T[%d]", tp->right->triple.tripleno );
				}
				break;
	
			default:
				printf("print_triple: bad right operand");
				break;
		}
		if( tp->can_access ) {
			printf(" can_access: ");
			LFOR(lp,tp->can_access){
					printf("L[%d] ", LCAST(lp,LEAF)->leafno);
			}
		}
# ifdef IROPT
{
VAR_REF *vrp;
EXPR_REF *erp;
		if(tp->reachdef1) {
			printf(" reachdef1: ");
			LFOR(lp,tp->reachdef1->next) {
				if(tp->left->operand.tag == ISTRIPLE) {
					printf("T[%d] ", ( LCAST(lp,EXPR_REF)->site.tp->tripleno));
				} else if(tp->left->operand.tag == ISLEAF ){
					printf("T[%d] ", ( LCAST(lp,VAR_REF)->site.tp->tripleno));
				} else {
					quit("print_triple: bad left operand");
				}
			}
		}
		if(tp->reachdef2) {
			printf(" reachdef2: ");
			LFOR(lp,tp->reachdef2->next){
				if(tp->right->operand.tag == ISTRIPLE) {
					printf("T[%d] ", ( LCAST(lp,EXPR_REF)->site.tp->tripleno));
				} else if(tp->right->operand.tag == ISLEAF ){
					printf("T[%d] ", ( LCAST(lp,VAR_REF)->site.tp->tripleno));
				} else {
					quit("print_triple: bad left operand");
				}
			}
		}
		if(tp->canreach) {
			printf(" canreach: ");
			LFOR(lp,tp->canreach->next){
				if(ISOP(tp->op,VALUE_OP)){
					printf("T[%d] ", LCAST(lp,EXPR_REF)->site.tp->tripleno);
				} else {
					printf("T[%d] ", LCAST(lp,VAR_REF)->site.tp->tripleno);
				}
			}
		}
		if( tp->var_refs ) {
			printf(" var refs: ");
			for(vrp=tp->var_refs;vrp && vrp->site.tp == tp;vrp= vrp->next){
				printf("%d ", vrp->refno);
			}
		}
}
# endif
		printf("\n");
	}
}

print_block(p)
register BLOCK *p;
{
register LIST *lp;
register TRIPLE *tlp;
int i=0;

	printf("[%d] %s label %d next %d",p->blockno,
				(p->entryname ? p->entryname : ""), p->labelno, 
				(p->next ? p->next->blockno : -1 ));

# ifdef IROPT
	printf("loop_weight %d",p->loop_weight);
	printf(" pred: ");
	if(p->pred) LFOR(lp,p->pred->next) {
		printf("%d ",  ((BLOCK *)lp->datap)->blockno );
	}
	printf("succ: ");
	if(p->succ) LFOR(lp,p->succ->next) {
		printf("%d ",  ((BLOCK *)lp->datap)->blockno );
	}
# endif
	printf("\ntriples:");
	if(p->last_triple) TFOR(tlp,p->last_triple->tnext) {
		if( (++i)%35 == 0) printf("\n");
		printf("%d ",   tlp->tripleno);
	}
	printf("\n");
}

print_segment(sp)
register SEGMENT *sp;
{
	printf("%s",sp->name);
	printf(" length %d",sp->len);
	printf(" %s ", seg_content_names[(int)sp->descr.content]);
	printf(" %s ",(sp->descr.external == EXTSTG_SEG) ? "EXTSTG_SEG" :"LCLSTG_SEG");
	printf("\n");
}

dump_segments()
{
register SEGMENT *sp;

printf("\nSEGMENTS\n");
for(sp=seg_tab;sp < &seg_tab[nseg]; sp++) print_segment(sp);
}

dump_leaves() 
{
register LEAF *lp, *leafp, **leafparr;
register LIST **lpp, *hash_link;

	leafparr = (LEAF**) ckalloc((nleaves+1) * sizeof(char*));
	leafparr[nleaves] = (LEAF*) NULL;
	printf("\nLEAVES\n");
	for(leafp = leaf_tab; leafp; leafp = leafp->next_leaf ) {
		leafparr[leafp->leafno] = leafp;
	}
	for(leafp = *leafparr++; leafp ; leafp = *leafparr++) {
				print_leaf(leafp);
	}
}

dump_triples() 
{
register TRIPLE *tp;

printf("\nTRIPLES\n");
	for(tp=triple_tab;tp<&triple_tab[ntriples]; tp++) {
		print_triple(tp,0);
	}
}

dump_blocks() 
{
register BLOCK *tp;

printf("\nBLOCKS\n");
for(tp=entry_block;tp;tp=tp->next) print_block(tp);
}

show_list(start)
LIST *start;
{
LIST *l;

l=start;
if(l)  {
	printf("here\tnext\tprev\tdatap\n");
	do {
		printf("%08X %08X %08X\n",l,l->next,l->datap);
		l=l->next;
	} while(l && l != start);
}
}
