#ifndef lint
static	char sccsid[] = "@(#)debug.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

#include "iropt.h"
#include "reg.h"
#include "loop.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

extern int npages[], npagetags;

extern int nvardefs, nexprdefs;
extern int nvarrefs, nexprrefs;
extern LISTQ *proc_lq, *df_lq, *tmp_lq;
extern TRIPLE *triple_tab, *triple_top;
extern LEAF *leaf_tab;
extern SEGMENT *seg_top;

extern int pgsize, availchar_per_page;
extern PAGE *freepage_lifo;

static char * seg_content_names[] = {
	"REG_SEG", "STG_SEG", "TEXT_SEG"
};

queuesize(lqp)
LISTQ* lqp;
{
register PAGE *pp;
int n = 0;
	for(pp=lqp->head; pp; pp=pp->next) n++;
	return n;
}

exrefstats()
{
EXPR_REF *rp;
int n[8], i, nlist;
BLOCK *bp;
TRIPLE *tp, *tp2;
LIST *lp;
EXPR *ep, **epp;
	for(i=0;i<8;i++) n[i]=0;

	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			for(rp = ep->references; rp ; rp = rp->next_eref) {
				n[(int)rp->reftype]++;
			}
		}
	}

	for(i=0;i<8;i++) printf("%d %d\n",i,n[i]);
	nlist=0;
	for(bp=entry_block; bp; bp = bp->next) {
		TFOR(tp,bp->last_triple) {
			if( ISOP(tp->op, NTUPLE_OP)) {
				TFOR( tp2, (TRIPLE*) tp->right ) {
					LFOR(lp,tp->canreach) { nlist++; }
					LFOR(lp,tp->reachdef1) { nlist++; }
					LFOR(lp,tp->reachdef2) { nlist++; }
				}
			}
			LFOR(lp,tp->canreach) { nlist++; }
			LFOR(lp,tp->reachdef1) { nlist++; }
			LFOR(lp,tp->reachdef2) { nlist++; }
		}
	}
	printf("nlists %d \n",nlist);
}

pagestats()
{
int i, *freepages;
register PAGE *pp;
struct rusage rusage, *rup = &rusage;

	freepages = (int*) ckalloca(npagetags*sizeof(long));
  	getrusage(RUSAGE_SELF,rup);
	printf("df_lq %d ",queuesize(df_lq));
	printf("proc_lq %d ",queuesize(proc_lq));
	printf("tmp_lq %d ",queuesize(tmp_lq));
	printf("page summary %d faults\n",rup->ru_majflt);
	printf("allocated:\n"); 
	for(i=0;i < npagetags; i++ ) {
		printf(" %d %d\n",i,npages[i]);
		freepages[i] =0;
	}
	printf("freed:\n"); 
	for(pp=freepage_lifo;pp;pp=pp->next) {
		freepages[pp->used_for]++;
	}
	for(i=0;i < npagetags; i++ ) {
		printf(" %d %d\n",i,freepages[i]);
	}
}


dump_byte_tab(header,tab,nrows,ncols)
char *header, *tab;
int nrows, ncols;
{
	int row, col, val;
	char *index = tab;

	printf("%s\n   ",header);
	for(col=0;col<ncols;col++) {
		printf("%2d ",col);
	}
	putchar('\n');
	for(row=0;row<nrows;row++) {
		printf("%2d ",row);
		for(col=0;col<ncols;col++) {
			if(val = *(index++)) {
				printf("%2d ",val);
			} else {
				printf("   ");
			}
		}
		putchar('\n');
	}
}

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
VAR_REF *vrp;

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
		if(lp->overlap) {
			printf(" overlap: ");
			LFOR(listp,lp->overlap) {
				printf("L[%d] ", LCAST(listp,LEAF)->leafno);
			}
		}
		printf("\n");
}

print_triple(tp, indent)
register TRIPLE *tp;
int indent;
{
register LIST *lp;
register TRIPLE *tlp;
VAR_REF *vrp;
EXPR_REF *erp;
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
		if(tp->right->triple.left) {
			printf("\n");
			print_triple(tp->right,indent+2);
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
						printf(" B[%d] if ",((BLOCK*)tlp->left)->blockno);
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
		if( tp->can_access ) {
			printf(" can_access: ");
			LFOR(lp,tp->can_access){
					printf("L[%d] ", LCAST(lp,LEAF)->leafno);
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
		printf("\n");
	}
}

print_block(p, detailed)
register BLOCK *p;
BOOLEAN detailed;
{
register LIST *lp;
register TRIPLE *tlp;
int i=0;

	printf("[%d] %s label %d loop_weight %d next %d",p->blockno,
				(p->entryname ? p->entryname : ""), p->labelno, 
				p->loop_weight, (p->next ? p->next->blockno : -1 ));

	printf(" pred: ");
	if(p->pred) LFOR(lp,p->pred->next) {
		printf("%d ",  ((BLOCK *)lp->datap)->blockno );
	}
	printf("succ: ");
	if(p->succ) LFOR(lp,p->succ->next) {
		printf("%d ",  ((BLOCK *)lp->datap)->blockno );
	}
	printf("\ntriples:");
	if(p->last_triple) TFOR(tlp,p->last_triple->tnext) {
		if(detailed == TRUE) {
			print_triple(tlp, 1);
		} else {
			if( (++i)%35 == 0) printf("\n");
			printf("%d ",   tlp->tripleno);
		}
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
for(sp=seg_tab;sp<seg_top; sp++) print_segment(sp);
}

dump_leaves() 
{
LEAF *leafp, **leafparr;

	leafparr = (LEAF**) ckalloca((nleaves+1) * sizeof(char*));
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
	for(tp=triple_tab;tp<triple_top; tp++) {
		print_triple(tp,0);
	}
}

dump_blocks() 
{
	register BLOCK *tp;

	printf("\nBLOCKS\n");
	for(tp=entry_block;tp;tp=tp->next) print_block(tp, FALSE);
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
