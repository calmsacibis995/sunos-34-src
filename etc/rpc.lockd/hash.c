#ifndef lint
static char sccsid[] = "@(#)hash.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */
	/* hash.c
	 * rotuines handle insertion, deletion of hashed monitor, file entries
	 */

#include "prot_lock.h"

#define MAX_HASHSIZE 100


char *malloc();
char *xmalloc();
extern int debug;
extern int HASH_SIZE;
extern struct fs_rlck *rel_fe;

typedef struct fs_rlck cache_fp;
typedef struct fs_rlck cache_me;

cache_fp *table_fp[MAX_HASHSIZE];
cache_me *table_me[MAX_HASHSIZE];
int cache_fp_len = sizeof(struct fs_rlck);
int cache_me_len = sizeof(struct fs_rlck);

/*
 * find_fe returns the cached entry;
 * it returns NULL if not found;
 */
struct fs_rlck *
find_fe(a)
reclock *a;
{
	cache_fp *cp;

	cp = table_fp[hash(a->lck.fh_bytes)];
	while( cp != NULL) {
		if(strcmp(cp->svr, a->lck.svr) == 0 &&
		obj_cmp(&cp->fs.fh, &a->lck.fh)) {
			/*found */
			return(cp);
		}
		cp = cp->nxt;
	}
	return(NULL);
}

/*
 * find_me returns the cached entry;
 * it returns NULL if not found;
 */
struct fs_rlck *
find_me(svr, proc)
char *svr;
int proc;
{
	cache_me *cp;

	cp = table_me[hash(svr, proc)];
	while( cp != NULL) {
		if(strcmp(cp->svr, svr) == 0 &&
		cp->fs.procedure == proc) {
			/*found */
			return(cp);
		}
		cp = cp->nxt;
	}
	return(NULL);
}

void
insert_fe(fp)
struct fs_rlck *fp;
{
	int h;

	h = hash(fp->fs.fh_bytes);
	fp->nxt = table_fp[h];
	table_fp[h] = fp;
}

void
insert_me(mp)
struct fs_rlck *mp;
{
	int h;

	h = hash(mp->svr);
	mp->nxt = table_me[h];
	table_me[h] = mp;
}

void
release_fe()
{
	cache_fp *cp, *fp;
	cache_fp *cp_prev = NULL;
	cache_fp *next;
	int h;

	if(rel_fe == NULL) 
		return;
	fp = rel_fe;
	if(fp->rlckp == NULL) {
		h = hash(fp->fs.fh_bytes);
		next = table_fp[h];
		while((cp = next) != NULL) {
			next = cp->nxt;
			if(strcmp(cp->svr, fp->svr) == 0 &&
			obj_cmp(&cp->fs.fh, &fp->fs.fh)) {
				if(cp_prev == NULL) {
					table_fp[h] = cp->nxt;
				}
				else {
					cp_prev->nxt = cp->nxt;
				}
				free_fe(cp);
				rel_fe = NULL;
				return;
			}
			else {
				cp_prev = cp;
			}
		}
	}
}

release_me()
{
	/* we never free up monitor entry, the knowledge of contacting
	 * status monitor accumulates
	 */
}

