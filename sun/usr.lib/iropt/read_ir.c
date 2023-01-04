#ifndef lint
static	char sccsid[] = "@(#)read_ir.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include "iropt.h"
#include <stdio.h>
#include <ctype.h>n
#include <sys/types.h>
#include <sys/uio.h>

BLOCK *new_block();
BLOCK *entry_block;
BLOCK *last_block;

int nseg, nleaves, ntriples, nblocks;
static int nlists, strtabsize;

SEGMENT *seg_tab, *seg_top;
BLOCK *entry_tab, *entry_top;
TRIPLE *triple_tab, *triple_top;
LEAF *leaf_tab, *leaf_top;
static char *string_tab,*string_top;
static LIST *list_tab, *list_top;

LIST *ext_entries;

extern LISTQ *proc_lq, *tmp_lq;
extern HEADER hdr;
extern char *ir_file;
extern char *heap_start;

LIST *labelno_consts, *indirgoto_targets;
int n_label_defs;

int irfd;

STRING_BUFF *string_buff, *first_string_buff;

#define NULL_INDEX -1
/*  FIXME
	see ir_common.h for the magic #s to use here. this should be done by cpp
*/
#define SIZEOF_SHORT_TRIPLE (sizeof(TRIPLE) - 7*sizeof(char*))
#define SIZEOF_SHORT_LEAF (sizeof(LEAF) -  7*sizeof(char*))
#define SIZEOF_SHORT_BLOCK (sizeof(BLOCK) - 4*sizeof(char*))

#define DEINDEX_BLOCK(index) ((BLOCK*)( (int)(index) == NULL_INDEX ? NULL : \
	(((int)(index) - hdr.block_offset) + (char*)entry_tab)))

#define DEINDEX_TRIPLE(index) ((TRIPLE*)( (int)(index) == NULL_INDEX ? NULL : \
	(((((int)(index) - hdr.triple_offset)/SIZEOF_SHORT_TRIPLE)*sizeof(TRIPLE))\
		+ (char*) triple_tab)))

#define DEINDEX_LEAF(index) ((LEAF*)( ( (int)(index) == NULL_INDEX ? NULL : \
	((((int)(index) - hdr.leaf_offset)/SIZEOF_SHORT_LEAF)*sizeof(LEAF))\
		+ (char*) leaf_tab)) )

#define DEINDEX_NODE(index) (((int)(index) < hdr.block_offset ?\
	(NODE*) DEINDEX_TRIPLE(index) :\
	(NODE*) DEINDEX_LEAF(index)))

#define DEINDEX_LIST(index) ((LIST*)((int)(index) == NULL_INDEX ? NULL : \
	(((int)(index) - hdr.list_offset) + (char*) list_tab)))

#define DEINDEX_STRING(index) ((char*)((int)(index) == NULL_INDEX ? NULL : \
	(((int)(index) - hdr.string_offset) + (char*) string_tab)))

#define DEINDEX_SEG(index) ((SEGMENT*)((int)(index) == NULL_INDEX ? NULL : \
	(((int)(index) - hdr.seg_offset) + (char*) seg_tab)))

/* a list datap item could be pointing at anything ...*/
LOCAL
deindex_ldata(index)
int index;
{
	if( index == NULL_INDEX) {
		return NULL;
	} else if(index < hdr.block_offset ) {
		return ((int) DEINDEX_TRIPLE(index));
	} else if(index < hdr.leaf_offset ) {
		return ((int) DEINDEX_BLOCK(index));
	} else if(index < hdr.seg_offset ) {
		return ((int) DEINDEX_LEAF(index));
	} else if(index < hdr.string_offset ) {
		return ((int) DEINDEX_SEG(index));
	} else if(index < hdr.list_offset ) {
		return ((int) DEINDEX_STRING(index));
	} else if(index < hdr.proc_size ) {
		return ((int) DEINDEX_LIST(index));
	} else {
		quit("deindex_ldata: index out of range");
	}
}

BOOLEAN
read_ir(irfile_descriptor)
int irfile_descriptor;
{
register int i;
int size;
char *irfile_space;

	irfd = irfile_descriptor;
	ntriples = (hdr.block_offset - hdr.triple_offset)/ SIZEOF_SHORT_TRIPLE;
	nblocks = (hdr.leaf_offset - hdr.block_offset)/ SIZEOF_SHORT_BLOCK;
	nleaves = (hdr.seg_offset - hdr.leaf_offset)/ SIZEOF_SHORT_LEAF;
	nseg = (hdr.string_offset - hdr.seg_offset)/ sizeof(SEGMENT);
	strtabsize = (hdr.list_offset - hdr.string_offset)/sizeof(char);
	nlists = (hdr.proc_size - hdr.list_offset)/ sizeof(LIST);

	irfile_space = heap_start;
	if(ntriples) {
		triple_tab = (TRIPLE*) irfile_space;
		irfile_space += ntriples*sizeof(TRIPLE);
		triple_top = &triple_tab[ntriples];
	} else {
		triple_top = triple_tab = (TRIPLE*) NULL;
	}
	
	if(nblocks) {
		entry_tab = (BLOCK*) irfile_space;
		irfile_space += nblocks*sizeof(BLOCK);
		entry_top = &entry_tab[nblocks];
	} else {
		entry_top = entry_tab = (BLOCK*) NULL;
	}

	if(nleaves) {
		leaf_tab = (LEAF*) irfile_space;
		irfile_space += nleaves*sizeof(LEAF);
	} else {
		leaf_top = leaf_tab = (LEAF*) NULL;
	}

	if(nseg) {
		seg_tab = (SEGMENT*) irfile_space;
		irfile_space += nseg*sizeof(SEGMENT);
		seg_top = &seg_tab[nseg];
	} else {
		seg_top = seg_tab = (SEGMENT*) NULL;
	}


	if(strtabsize) {
		string_tab = (char*) irfile_space;
		irfile_space += strtabsize;
		string_top = &string_tab[strtabsize];
	} else {
		string_top = string_tab = (char*) NULL;
	}

	if(nlists) {
		list_tab = (LIST*) irfile_space;
		irfile_space += nlists*sizeof(LIST);
		list_top = &list_tab[nlists];
	} else {
		list_top = list_tab = LNULL;
	}
	hdr.procname = DEINDEX_STRING(hdr.procname);
	heap_setup(irfile_space - heap_start);

	proc_init();

	if(ntriples) launder_triples();
	if(nblocks) launder_entries();
	if(nleaves) launder_leaves();
	if(nseg) launder_segs();
	launder_strings();
	if(nlists) launder_lists();

	if(nleaves == 0 || ntriples ==0 || nblocks == 0 || ext_entries == LNULL) {
		return FALSE ;
	}
	ir_setup();
	return TRUE ;
}

LOCAL
launder_strings()
{
	if(strtabsize) {
		if( read(irfd, string_tab, strtabsize ) != strtabsize ) {
			perror("");
			quit("read_ir: read error");
		}
	}

	/* allocate an intial string_buffer for the strings read in */
	first_string_buff=string_buff=(STRING_BUFF*) ckalloc(1,sizeof(STRING_BUFF));
	string_buff->next = (STRING_BUFF*) NULL;
	string_buff->data = string_tab;
	string_buff->max = string_buff->top = string_top;
}

LOCAL
launder_segs()
{
register SEGMENT *seg;
int seg_tab_size;
register LEAF *leafp;
LIST *lp;

	seg_tab_size = nseg*sizeof(SEGMENT);
	if( read(irfd, seg_tab, seg_tab_size) != seg_tab_size ) {
		perror("");
		quit("read_ir: read error");
	}
	for(seg=seg_tab; seg < &seg_tab[nseg]; seg ++) {
		seg->name = DEINDEX_STRING(seg->name);
		seg->leaves = LNULL;
	}
}

LOCAL
launder_lists()
{
register LIST *p;
int list_tab_size;

	list_tab_size = nlists*sizeof(LIST);
	if( read(irfd, list_tab, list_tab_size) != list_tab_size ) {
		perror("");
		quit("read_ir: read error");
	}

	for(p=list_tab; p < list_top; p ++) {
		p->next = DEINDEX_LIST(p->next);
		p->datap = (LDATA *) deindex_ldata(p->datap);
	}
}

LOCAL
launder_leaves()
{ 
register LEAF *p;
register LIST *lp;
register int i;
register int index;
LEAF leaf;
SEGMENT *segp;
char *short_leaf_tab;
int short_leaf_tab_size;

	short_leaf_tab_size = nleaves * SIZEOF_SHORT_LEAF;
	short_leaf_tab = (char*) ckalloca(short_leaf_tab_size);
	if( read(irfd,short_leaf_tab, short_leaf_tab_size)
		!= short_leaf_tab_size) {
		perror("");
		quit("read_ir: read error");
	}

	leaf_top = (LEAF*) NULL;
	for(i=0,p=leaf_tab; i< nleaves; i++,p++) {
		bcopy(short_leaf_tab, p, SIZEOF_SHORT_LEAF);
		short_leaf_tab += SIZEOF_SHORT_LEAF;
		if (i != p->leafno ) {
			quit("launder_leaves: leaves out of order in ir file");
			/*FIXME debugging test */
		}

		p->overlap = DEINDEX_LIST(p->overlap);
		p->pass1_id = DEINDEX_STRING(p->pass1_id);
		if(leaf_top) {
			leaf_top->next_leaf = p;
		}
		leaf_top = p;
		p->next_leaf = (LEAF*) NULL;
		p->visited = FALSE;
		if( p->class == VAR_LEAF || p->class == ADDR_CONST_LEAF ) {
			p->val.addr.seg = DEINDEX_SEG(p->val.addr.seg );
		} else {
			if(p->val.const.isbinary == TRUE) {
				p->val.const.c.bytep[0]=DEINDEX_STRING(p->val.const.c.bytep[0]);
				if( B_ISCOMPLEX(p->type.tword)) {
					p->val.const.c.bytep[1] = 
						DEINDEX_STRING(p->val.const.c.bytep[1]);
				}
			} else if(B_ISCHAR(p->type.tword) || ISFTN(p->type.tword) )  {
				p->val.const.c.cp = DEINDEX_STRING( p->val.const.c.cp );
			} else if( B_ISREAL(p->type.tword) ) {
				p->val.const.c.fp[0] = DEINDEX_STRING( p->val.const.c.fp[0] );
			} else if( B_ISCOMPLEX(p->type.tword)) {
				p->val.const.c.fp[0] = DEINDEX_STRING( p->val.const.c.fp[0] );
				p->val.const.c.fp[1] = DEINDEX_STRING( p->val.const.c.fp[1] );
			}
		}


	}
}
	
LOCAL
launder_entries()
{
register BLOCK *bp;
register LIST *lp;
TRIPLE *label;
char *short_block_tab;
int short_block_tab_size;

	short_block_tab_size = nblocks * SIZEOF_SHORT_BLOCK;
	short_block_tab = (char*) ckalloca(short_block_tab_size);
	if( read(irfd,short_block_tab, short_block_tab_size)
		!= short_block_tab_size) {
		perror("");
		quit("read_ir: read error");
	}


	ext_entries = LNULL;
	for(bp=entry_tab; bp < entry_top; bp ++) {
		bcopy(short_block_tab, bp, SIZEOF_SHORT_BLOCK);
		short_block_tab += SIZEOF_SHORT_BLOCK;
		bp->entryname = DEINDEX_STRING(bp->entryname);
		bp->last_triple = DEINDEX_TRIPLE(bp->last_triple);
		bp->pred = DEINDEX_LIST(bp->pred);
		bp->succ = DEINDEX_LIST(bp->succ);
		bp->loop_weight = 1;
		
		if(bp->is_ext_entry == TRUE) {
			lp=NEWLIST(proc_lq);
			(BLOCK*) lp->datap = bp;
			LAPPEND(ext_entries,lp);
		} 
	}
	
}

LOCAL
make_null_entry()
{
TYPE type;
TRIPLE *append_triple(), *refs;
BLOCK *entry;
LIST *lp;

	type.tword = UNDEF;
	type.aux.size =0;
	refs = TNULL;
	last_block = (BLOCK*) NULL;

	entry_block = new_block();
	entry_block->last_triple = append_triple(TNULL,LABELDEF, ileaf(0), type);
	LFOR(lp,ext_entries) {
		entry = (BLOCK*) lp->datap;
		refs = append_triple(refs,LABELREF, ileaf(entry->labelno), ileaf(0), type);
		add_labelref(refs);
	}
	entry_block->last_triple = 
		append_triple(entry_block->last_triple,SWITCH, ileaf(0), refs, type);
}

launder_triples()
{
register TRIPLE *p, *tlp;
register LIST *lp;
char *short_triple_tab;
int short_triple_tab_size;

	short_triple_tab_size = ntriples * SIZEOF_SHORT_TRIPLE;
	short_triple_tab = (char*) ckalloca(short_triple_tab_size);
	if(read(irfd,short_triple_tab, short_triple_tab_size) 
		!= short_triple_tab_size  ) {
		perror("");
		quit("read_ir: read error");
	}

	for(p=triple_tab; p < triple_top; p ++) {
		bcopy(short_triple_tab, p, SIZEOF_SHORT_TRIPLE);
		short_triple_tab += SIZEOF_SHORT_TRIPLE;
		p->left = DEINDEX_NODE(p->left);
		p->right = DEINDEX_NODE(p->right);
		p->tnext = DEINDEX_TRIPLE(p->tnext);
		p->tprev = DEINDEX_TRIPLE(p->tprev);
		p->can_access = DEINDEX_LIST(p->can_access);
		p->visited = FALSE;
	}
}

/* additional house keeping that can be done only after other parts
** of the ir file are available
*/
LOCAL
ir_setup()
{

	/* set up label ref/def structures for cf */
	{
	register TRIPLE *p, *tlp;
	register LIST *lp;
		for(p=triple_tab; p < triple_top; p ++) {
			switch (p->op) {
				case ADDROF:
						if(p->left->operand.tag == ISLEAF) {
							p->left->leaf.no_reg = TRUE;
						} else {
							quit("launder_triples: address of expression");
						}
						break;

				case GOTO:	 
				case SWITCH:
				case CBRANCH:
				case REPEAT:	 
						TFOR(tlp,(TRIPLE*)p->right) {
							add_labelref(tlp);
						}
						break;

				case INDIRGOTO:	 
						TFOR(tlp,(TRIPLE*)p->right) {
							add_labelref(tlp);
						}
						if(indirgoto_targets == LNULL) {
							TFOR(tlp,(TRIPLE*)p->right) {
								lp = NEWLIST(proc_lq);
								(TRIPLE*) lp->datap = tlp;
								LAPPEND(indirgoto_targets,lp);
							}
						}
						break;

				case LABELDEF:
					add_labelref(p); 
					n_label_defs++;
					break;
					
			}
		}
	}

	/* set up leaf hash tab */
	{
	register LEAF *leafp;
	register LIST *lp;
	register SEGMENT *seg;
	register int index;

		for(leafp=leaf_tab; leafp; leafp=leafp->next_leaf) {
			if( leafp->class == VAR_LEAF || leafp->class == ADDR_CONST_LEAF ) {
				index=hash_leaf(leafp->class,&leafp->val.addr);
				seg = leafp->val.addr.seg;
				if(leafp->class == VAR_LEAF) {
					lp = NEWLIST(proc_lq);
					(LEAF*) lp->datap = leafp;
					LAPPEND(seg->leaves,lp);
				}
			} else { /* a CONST LEAF */
				if(leafp->type.tword == LABELNO) {
					lp = NEWLIST(proc_lq);
					(LEAF*) lp->datap = leafp;
					LAPPEND(labelno_consts,lp);
				}
				index=hash_leaf(CONST_LEAF,&leafp->val.const);
			}
			lp = NEWLIST(proc_lq);
			(LEAF*) lp->datap = leafp;
			LAPPEND(leaf_hash_tab[index],lp);
		}
	}

	/* and ensure blocks are in correct order */
	if( ext_entries->next == ext_entries ) {
		entry_block = (BLOCK*) ext_entries->datap;
	} else {
		make_null_entry();
	}

	last_block = entry_block;
	entry_block->blockno = 0; /* relied on by various df routines */
	nblocks  = 1;
	{ 
	BLOCK *bp;
		for(bp=entry_tab; bp < entry_top; bp ++) {
			if(bp==entry_block) {
				continue;
			} else {
				last_block->next = bp;
				last_block = bp;
				bp->blockno = nblocks++;
			}
		}
	}
	last_block->next = (BLOCK*) NULL;
}
