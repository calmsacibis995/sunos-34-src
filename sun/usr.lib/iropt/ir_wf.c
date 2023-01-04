#ifndef lint
static	char sccsid[] = "@(#)ir_wf.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

# include "iropt.h"
# define first_leaf leaf_tab
# define first_block entry_block

#include <stdio.h>

extern LEAF* leaf_tab;
extern int regmask;

char *
new_string(cp)
char *cp;
{
char *start;
char *add_bytes();
	start = add_bytes(cp,strlen(cp));
	return start;
}

#define SIZEOF_TRIPLE (sizeof(TRIPLE) - 7*sizeof(char*))
#define SIZEOF_LEAF (sizeof(LEAF) -  7*sizeof(char*))
#define SIZEOF_BLOCK (sizeof(BLOCK) - 4*sizeof(char*))

#define roundup(a,b)    ( b * ( (a+b-1)/b) )

#define ENINDEX_TRIPLE(ptr)  (TRIPLE*) (ptr ? (hdr.triple_offset + \
			((TRIPLE*)ptr)->tripleno*SIZEOF_TRIPLE ) : -1 )

#define ENINDEX_BLOCK(ptr)  (BLOCK*) ( ptr ? (hdr.block_offset + \
			((BLOCK*)ptr)->blockno*SIZEOF_BLOCK ) : -1 )

#define ENINDEX_LEAF(ptr)  (LEAF*) (ptr ? (hdr.leaf_offset + \
			((LEAF*)ptr)->leafno*SIZEOF_LEAF ) : -1 ) 

#define ENINDEX_SEGMENT(ptr)  (SEGMENT*) (ptr ? (hdr.seg_offset + \
			(((SEGMENT*)ptr) - seg_tab)*sizeof(SEGMENT) ) : -1 )

static HEADER hdr;
static FILE *irfile;
static int top_list;
static long start;

/*	write out an IR file.  First determine how big the file will be by
**	counting everything except list blocks, renumber structures as we go.
**	From this we can calculate the offset at which triples will start, where 
**	blocks will start, etc.  Then use  the structure number to calculate its offset
*/
write_irfile(procno, procname, proc_type, filep)
FILE *filep;
int procno, proc_type;
char *procname;
{

	bzero(&hdr,sizeof(HEADER));
	irfile = filep;

	hdr.procno = procno;
	hdr.procname = (char*) new_string(procname);
	hdr.regmask = regmask;
	hdr.proc_type = proc_type;
	
	start = ftell(irfile);
	find_offsets();
	hdr.procname = (char*) enindex_stringptr(hdr.procname);
	top_list = hdr.list_offset;
		/* save space for the header - don't know nlists yet*/
	fseek(irfile, sizeof(HEADER),1); 
	write_triples();
	write_blocks();
	write_leaves();
	write_segments();
	write_strings();

	fseek(irfile, start, 0);
	hdr.proc_size = top_list;
	hdr.file_status = OPTIMIZED;
	fwrite(&hdr,sizeof(HEADER),1,irfile);
	fseek(irfile, start+top_list, 0);
}

LOCAL
find_offsets()
{
BLOCK *bp;
TRIPLE *tp, *first, *tp2, *tp3;
LEAF *leafp;
int charno, tripleno, leafno, blockno,n;
STRING_BUFF *sbp;

	charno =  tripleno =  leafno =  blockno = 0;

	hdr.triple_offset = sizeof(HEADER);
	for(bp=first_block; bp; bp=bp->next) {
		first = (TRIPLE*) NULL;
		if(bp->last_triple) first=bp->last_triple->tnext;
		TFOR(tp,first){
			if(ISOP(tp->op,NTUPLE_OP)) {
				register TRIPLE *tp3, *tp2 = (TRIPLE*) tp->right;
				TFOR(tp3, tp2) {
					tp3->tripleno = tripleno++;
				}
			}
			tp->tripleno = tripleno++;
		}
	}
	if(tripleno != ntriples) {
		/* quit("find_offsets: ntriples wrong"); FIXME */
	}

	hdr.block_offset = hdr.triple_offset + tripleno*SIZEOF_TRIPLE;
	for(bp=first_block; bp; bp = bp->next) {
		bp->blockno = blockno++;
	}
	if(blockno != nblocks) {
		quit("find_offsets: nblocks wrong");
	}

	hdr.leaf_offset = hdr.block_offset + nblocks*SIZEOF_BLOCK;
	for(leafp=first_leaf; leafp; leafp = leafp->next_leaf) {
		leafp->leafno = leafno++;
	}
	if(leafno != nleaves) {
		quit("find_offsets: nleaves wrong");
	}

	hdr.seg_offset = hdr.leaf_offset + nleaves*SIZEOF_LEAF;

	hdr.string_offset = hdr.seg_offset + nseg*sizeof(SEGMENT);
	for(sbp=first_string_buff; sbp; sbp=sbp->next) {
			charno += (sbp->top - sbp->data);
	}

	hdr.list_offset = hdr.string_offset + roundup(charno,sizeof(long));
}

LOCAL 
write_segments()
{
register SEGMENT *seg;

	for(seg = seg_tab; seg < &seg_tab[nseg] ;seg++) {
		seg->name = (char*) enindex_stringptr(seg->name);
	}
	fwrite(&seg_tab[0],sizeof(SEGMENT),nseg,irfile);
}

LOCAL
write_strings()
{
STRING_BUFF *sbp;
int n, strtabsize;
long zeroes = 0x00000000;

	strtabsize = 0;
	for(sbp=first_string_buff; sbp; sbp=sbp->next) {
		if( n = sbp->top - sbp->data ) {
			strtabsize += n;
			fwrite(sbp->data, sizeof(char), n, irfile);
		}
	}
	if(n=(strtabsize%sizeof(long))) {
		fwrite(&zeroes,sizeof(char),sizeof(long)-n,irfile);
	}
}

LOCAL
write_triples()
{
BLOCK *bp;
register TRIPLE *first, *tp;
TRIPLE *tp_tnext;

	for(bp=first_block; bp; bp=bp->next) {
		first = (TRIPLE*) NULL;
		if(bp->last_triple) first=bp->last_triple->tnext;
		for(tp=first; tp; tp = (tp_tnext == first ? TNULL : tp_tnext)){

			if(ISOP(tp->op,NTUPLE_OP)) {
			register TRIPLE *tpr;
			TRIPLE  *tpr_tnext;

				for(tpr=(TRIPLE*)tp->right; tpr; 
						tpr = (tpr_tnext == (TRIPLE*) tp->right ? TNULL : tpr_tnext)
				){
					tpr->left = (NODE *) enindex_node(tpr->left);
					tpr->right = (NODE *) enindex_node(tpr->right);
					tpr->tprev = (TRIPLE *) ENINDEX_TRIPLE(tpr->tprev);
					tpr_tnext = tpr->tnext;
					tpr->tnext = (TRIPLE *) ENINDEX_TRIPLE(tpr->tnext);
					tpr->can_access = (LIST *) enindex_list(tpr->can_access);
					fwrite(tpr,SIZEOF_TRIPLE, 1, irfile);
				}
			}

            tp->left = (NODE *) enindex_node(tp->left);
			tp->right = (NODE *) enindex_node(tp->right);
			tp->tprev = (TRIPLE *) ENINDEX_TRIPLE(tp->tprev);
			tp_tnext = tp->tnext;
			tp->tnext = (TRIPLE *) ENINDEX_TRIPLE(tp->tnext);
			tp->can_access = (LIST *) enindex_list(tp->can_access);
			fwrite(tp, SIZEOF_TRIPLE, 1, irfile);
		}
	}
}

LOCAL
write_blocks()
{
BLOCK *bp, *bp_next;

	for(bp=first_block; bp; bp=bp_next) {
		bp->entryname = (char *) enindex_stringptr(bp->entryname);
		bp->last_triple = ENINDEX_TRIPLE(bp->last_triple);
		bp->pred = (LIST*) enindex_list(bp->pred);
		bp->succ = (LIST*) enindex_list(bp->succ);
		bp_next = bp->next;
		bp->next = (BLOCK*) ENINDEX_BLOCK(bp->next);
		fwrite(bp,SIZEOF_BLOCK, 1, irfile);
	}
}

LOCAL
write_leaves()
{
LEAF *p, *p_next;

	for(p = first_leaf; p ; p = p_next ) {
		if(p->class == VAR_LEAF || p->class == ADDR_CONST_LEAF)  {
			p->val.addr.seg = ENINDEX_SEGMENT(p->val.addr.seg);
		} else {
			if(p->val.const.isbinary == TRUE) {
				p->val.const.c.bytep[0] = 
					(char *) enindex_stringptr(p->val.const.c.bytep[0]);
				if( (p->type.tword == BTYPE(p->type.tword)) &&
					B_ISCOMPLEX(p->type.tword)) {
					p->val.const.c.bytep[1] = 
						(char *) enindex_stringptr(p->val.const.c.bytep[1]);
				}
			} else if( (p->type.tword == BTYPE(p->type.tword) &&
						B_ISCHAR(p->type.tword)) || ISFTN(p->type.tword) )  {
				p->val.const.c.cp = 
					(char *) enindex_stringptr(p->val.const.c.cp);
			} else if( (p->type.tword == BTYPE(p->type.tword)) &&
						B_ISREAL(p->type.tword) ){
				p->val.const.c.fp[0] = 
					(char *) enindex_stringptr(p->val.const.c.fp[0]);
			} else if( (p->type.tword == BTYPE(p->type.tword)) &&
						B_ISCOMPLEX(p->type.tword)) {
				p->val.const.c.fp[0] = 
					(char *) enindex_stringptr(p->val.const.c.fp[0]);
				p->val.const.c.fp[1] = 
					(char *) enindex_stringptr(p->val.const.c.fp[1]);
			}
		}
		p_next = p->next_leaf;
		p->next_leaf = ENINDEX_LEAF( p->next_leaf );
		p->pass1_id = (char *) enindex_stringptr(p->pass1_id);
		p->overlap = (LIST *) enindex_list(p->overlap);
		fwrite(p, SIZEOF_LEAF, 1, irfile);
	}
}

LOCAL
enindex_node(p)
register NODE *p;
{

	if(p == (NODE*)NULL) return(-1);

	switch(p->operand.tag) {
		case ISTRIPLE:
			return( hdr.triple_offset + ((TRIPLE*)p)->tripleno*SIZEOF_TRIPLE );

		case ISLEAF:
			return( hdr.leaf_offset + ((LEAF*)p)->leafno*SIZEOF_LEAF );

		case ISBLOCK:
			return( hdr.block_offset + ((BLOCK*)p)->blockno*SIZEOF_BLOCK );

		default:
			quit("enindex_node: bad tag");
	}
}

/* COPY a list into an output buffer and change pointers to indices */
/* - there should be some sort of cashing to prevent multiple copies - FIXME */
LOCAL
enindex_list(list)
LIST *list;
{
register LIST *lp;
LIST list_struct, *newl = &list_struct;
int first, last, here;
	
	if(list == LNULL) {
		return -1;
	}
	here = ftell(irfile);
	fseek(irfile, start + top_list, 0);
	first = top_list;
	LFOR(lp,list) {
		newl->next = (LIST*) (top_list + sizeof(LIST));
		newl->datap = (union list_u*) enindex_node(lp->datap);
		fwrite(newl,sizeof(LIST),1,irfile);
		top_list += sizeof(LIST);
	}
	/* first patch last->next = first */
	fseek(irfile,-sizeof(LIST),1);
	fwrite(&first,sizeof(LIST*),1,irfile);
	/* and restore */
	fseek(irfile,here,0);
	return first;
}

/* turn a pointer to astring in a string_buffer into an ir file offset*/
LOCAL int
enindex_stringptr(cp)
register char *cp;
{
register STRING_BUFF *sbp;
register int offset;

	if(!cp) return(-1);

	offset = hdr.string_offset;
	for(sbp = first_string_buff;sbp;sbp=sbp->next) {

		if(cp >= sbp->data && cp < sbp->top ) {
			return( offset + (cp - sbp->data));
		}
		offset += (sbp->top - sbp->data);
	}
	quit("enindex_stringptr : string not in a STRING_BUFF");
}

/* add bytes to the string buffer list - get a new string buffer if needed */
char *
add_bytes(strp,len)
register char *strp;
register int len;
{
int size;
STRING_BUFF *tmp;
register char *start, *cursor;

	if( !string_buff || ((string_buff->top + len -1) >= string_buff->max)) {
		size = ( len > STRING_BUFSIZE ? len : STRING_BUFSIZE);
		tmp = (STRING_BUFF*) ckalloc(1,sizeof(STRING_BUFF)+size);
		tmp->next = (STRING_BUFF*) NULL;

		if(!string_buff) {
			first_string_buff = tmp;
		} else {
			string_buff->next = tmp;
		}
		string_buff = tmp;

		string_buff->data= string_buff->top = (char*)tmp + sizeof(STRING_BUFF);
		string_buff->max = string_buff->top + size;
	}

	cursor=string_buff->top;
	start = cursor;
	string_buff->top += len;
	while( len-- > 0) *cursor++ = *strp++;
	return start ;
}
