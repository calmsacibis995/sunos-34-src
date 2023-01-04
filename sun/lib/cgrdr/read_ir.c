#ifndef lint
static	char sccsid[] = "@(#)read_ir.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "cg_ir.h"
#include <stdio.h>
#include <ctype.h>n
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>

BLOCK *new_block();
BLOCK *entry_block;
BLOCK *last_block;

int nseg, nleaves, ntriples, nblocks, nleaves, nlists, strtabsize;

SEGMENT *seg_tab;
static SEGMENT *seg_top;
BLOCK *entry_tab;
static BLOCK *entry_top;
TRIPLE *triple_tab;
static TRIPLE *triple_top;
LEAF *leaf_tab;
static LEAF *leaf_top;
static LIST *list_tab, *list_top;
static char *string_tab,*string_top;

extern HEADER hdr;
extern char *ir_file;
BOOLEAN ext_entry;
char *irfile_start, *irfile_top;

#define TRIPLE_IOVX 0
#define BLOCK_IOVX 1
#define LEAF_IOVX 2
#define SEG_IOVX 3
#define STRING_IOVX 4
#define LIST_IOVX 5
#define N_IOVX LIST_IOVX+1

#define DEINDEX(offset) (char*) (offset) = \
	((off_t)(offset) == (off_t) -1 ? (char*) 0 : \
		(char*) (irfile_start) + (off_t) (offset) )

BOOLEAN
read_ir(irfd)
int irfd;
{
register int i;
struct iovec iov[N_IOVX];
int size, here;
struct stat stat_buff, *stat_buffp = &stat_buff;


	ntriples = (hdr.block_offset - hdr.triple_offset)/ sizeof(TRIPLE);
	nblocks = (hdr.leaf_offset - hdr.block_offset)/ sizeof(BLOCK);
	nleaves = (hdr.seg_offset - hdr.leaf_offset)/ sizeof(LEAF);
	nseg = (hdr.string_offset - hdr.seg_offset)/ sizeof(SEGMENT);
	strtabsize = (hdr.list_offset - hdr.string_offset)/sizeof(char);
	nlists = (hdr.proc_size - hdr.list_offset)/ sizeof(LIST);

	irfile_start = (char*) ckalloc(hdr.proc_size);
	irfile_top = irfile_start + sizeof(HEADER);
	here = lseek(irfd, 0, 1);

	if(ntriples) {
		triple_tab = (TRIPLE*) irfile_top;
		iov[TRIPLE_IOVX].iov_base = (caddr_t) triple_tab;
		iov[TRIPLE_IOVX].iov_len = size = ntriples*sizeof(TRIPLE);
		irfile_top += size;
		triple_top = &triple_tab[ntriples];
	} else {
		triple_top = triple_tab = (TRIPLE*) NULL;
		iov[TRIPLE_IOVX].iov_len = 0;
	}
	
	if(nblocks) {
		entry_tab = (BLOCK*) irfile_top;
		iov[BLOCK_IOVX].iov_base = (caddr_t) entry_tab;
		iov[BLOCK_IOVX].iov_len = size = nblocks*sizeof(BLOCK);
		irfile_top += size;
		entry_top = &entry_tab[nblocks];
	} else {
		entry_top = entry_tab = (BLOCK*) NULL;
		iov[BLOCK_IOVX].iov_len = 0;
	}

	if(nleaves) {
		leaf_tab = (LEAF*) irfile_top;
		iov[LEAF_IOVX].iov_base = (caddr_t) leaf_tab;
		iov[LEAF_IOVX].iov_len = size = nleaves*sizeof(LEAF);
		irfile_top += size;
		leaf_top = &leaf_tab[nleaves];
	} else {
		leaf_top = leaf_tab = (LEAF*) NULL;
		iov[LEAF_IOVX].iov_len = 0;
	}

	if(nseg) {
		seg_tab = (SEGMENT*) irfile_top;
		iov[SEG_IOVX].iov_base = (caddr_t) seg_tab;
		iov[SEG_IOVX].iov_len = size = nseg*sizeof(SEGMENT);
		irfile_top += size;
		seg_top = &seg_tab[nseg];
	} else {
		seg_top = seg_tab = (SEGMENT*) NULL;
		iov[SEG_IOVX].iov_len = 0;
	}

	if(strtabsize) {
		string_tab = (char*) irfile_top;
		iov[STRING_IOVX].iov_base = (caddr_t) string_tab;
		iov[STRING_IOVX].iov_len = size = strtabsize;
		irfile_top += size;
		string_top = &string_tab[strtabsize];
	} else {
		string_top = string_tab = (char*) NULL;
		iov[STRING_IOVX].iov_len = 0;
	}

	if(nlists) {
		list_tab = (LIST*) irfile_top;
		iov[LIST_IOVX].iov_base = (caddr_t) list_tab;
		iov[LIST_IOVX].iov_len = size = nlists*sizeof(LIST);
		irfile_top += size;
		list_top = &list_tab[nlists];
	} else {
		list_top = list_tab = LNULL;
		iov[LIST_IOVX].iov_len = 0;
	}

	if(readv(irfd,iov,N_IOVX) == -1) {
		perror(ir_file);
		quit("read_ir: read error");
	}

	if(nleaves == 0 || ntriples ==0 || nblocks == 0) {
		return FALSE ;
	}

	DEINDEX(hdr.procname);
	proc_init();
	launder_triples();
	launder_entries();
	launder_segs();
	launder_leaves();
	if(nlists) launder_lists();
	if(ext_entry == FALSE) {
		return FALSE ;
	}
	
	return(TRUE);

}

LOCAL
launder_segs()
{
register SEGMENT *seg;

	for(seg=seg_tab; seg < &seg_tab[nseg]; seg ++) {
		DEINDEX(seg->name);
		seg->leaves = LNULL;
	}
}

LOCAL
launder_lists()
{
register LIST *p;

	for(p=list_tab; p < list_top; p ++) {
		DEINDEX(p->next);
		DEINDEX(p->datap);
	}
}

LOCAL
launder_leaves()
{ 
register LEAF *p;
register LIST *lp;
register int i;
LEAF leaf;
SEGMENT *segp;
int index;

	for(i=0,p=leaf_tab; i< nleaves; i++,p++) {
		if (i != p->leafno ) {
			quit("launder_leaves: leaves out of order in ir file");
		}

		DEINDEX(p->overlap);
		DEINDEX(p->pass1_id);
		DEINDEX(p->next_leaf);
		p->visited = FALSE;

		if(p->class == ADDR_CONST_LEAF || p->class == VAR_LEAF) {
			DEINDEX(p->val.addr.seg);
			index=hash_leaf(p->class,&p->val.addr);
		} else {
			if(p->val.const.isbinary == TRUE) {
				DEINDEX(p->val.const.c.bytep[0]);
				if( B_ISCOMPLEX(p->type.tword)) {
					DEINDEX(p->val.const.c.bytep[1]);
				}
			} else if(B_ISCHAR(p->type.tword) || ISFTN(p->type.tword) )  {
				DEINDEX(p->val.const.c.cp);
			} else if( B_ISREAL(p->type.tword) ) {
				DEINDEX(p->val.const.c.fp[0]);
			} else if( B_ISCOMPLEX(p->type.tword)) {
				DEINDEX(p->val.const.c.fp[0]);
				DEINDEX(p->val.const.c.fp[1]);
			}
			index=hash_leaf(CONST_LEAF,&p->val.const);
		}
        lp = new_list();
		(LEAF*) lp->datap = p;
		LAPPEND(leaf_hash_tab[index],lp);
	}
}
	
LOCAL
launder_entries()
{
register BLOCK *bp;
register LIST *lp;
TRIPLE *label;

	ext_entry = FALSE;
	for(bp=entry_tab; bp < entry_top; bp ++) {
		DEINDEX(bp->entryname);
		DEINDEX(bp->last_triple);
		DEINDEX(bp->next);
		if(bp->is_ext_entry == TRUE) {
			ext_entry=TRUE;
		} 
	}
	
	if( ext_entry == FALSE) {
		return;
	} else {
		entry_block = entry_tab;
	}

}

launder_triples()
{
register TRIPLE *p, *tlp;
register LIST *lp;

	for(p=triple_tab; p < triple_top; p ++) {
		DEINDEX(p->left);
		DEINDEX(p->right);
		DEINDEX(p->tnext);
		DEINDEX(p->tprev);
		DEINDEX(p->can_access);
		p->visited = FALSE;
	}
}
