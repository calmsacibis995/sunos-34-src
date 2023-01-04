#ifndef lint
static char sccsid[] = "@(#)prot_alloc.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */
	/* prot_alloc.c
	 * consists of routines used to allocate and free space
	 */

#include "prot_lock.h"

extern struct fs_rlck *rel_fe, *rel_me;			/* free indicator*/
int used_le, used_fe, used_me, used_res;		/* # of entry used*/
extern struct fs_rlck  *grant_q;
extern struct fs_rlck  *monitor_q;
char *xmalloc(), *malloc();
extern reclock *get_le();
extern remote_result *get_res();
extern struct fs_rlck *get_me(), *get_fe();
extern int lock_len, res_len;
extern int debug;

void
xfree(a)
char **a;
{
	if(*a != NULL) {
		free(*a);
		*a = NULL;
	}
}

release_res(resp)
remote_result *resp;
{
	xfree(&resp->cookie_bytes); 
	free((char *) resp);
}

free_le(a)
reclock *a;
{
	used_le--;

	/* free up all space allocated through malloc */
	xfree(&a->lck.svr);
	xfree(&a->lck.fh_bytes);
	xfree(&a->lck.caller_name);
	xfree(&a->lck.oh_bytes);
	xfree(&a->cookie_bytes);
	xfree(&a->lck.clnt_name);
	free((char *) a);
}

release_le(a)
reclock *a;
{
	if(a->rel == 1) {		/* release bit is on */
		used_le--;
		add_mon(a, 0);

		/* free up all space allocated through malloc */
		xfree(&a->lck.svr);
		xfree(&a->lck.fh_bytes);
		xfree(&a->lck.caller_name);
		xfree(&a->lck.oh_bytes);
		xfree(&a->cookie_bytes);
		xfree(&a->lck.clnt_name);
		if(a->pre_le != NULL) {
			if(debug) printf("release_le: pre_le not free yet\n");
			free_le(a->pre_le);
			a->pre_le = NULL;
		}
		if(a->pre_fe != NULL) {
			if(debug) printf("release_le: pre_fe not free yet\n");
			free_fe(a->pre_fe);
			a->pre_fe = NULL;
		}

		free((char *) a);
	}
}

free_fe(fp)
struct fs_rlck *fp;
{
	used_fe--;
	xfree(&fp->svr);
	xfree(&fp->fs.fh_bytes);
	free((char *) fp);
}


/*
 * allocate space and zero it;
 * in case of malloc error, print console msg and return NULL;
 */
char *
xmalloc(len)
unsigned len;
{
	char *new;

	if((new = malloc(len)) == 0) {
		perror("malloc");
		return(NULL);
	}
	else {
		bzero(new, len);
		return(new);
	}
}


/*
 * these routines are here in case we try to optimize calling to malloc 
 */
reclock *
get_le()
{
	used_le ++;
	return( (reclock *) xmalloc(lock_len) );
}

struct fs_rlck *
get_fe()
{
	used_fe ++;
	return( (struct fs_rlck *) xmalloc(sizeof(struct fs_rlck)) );
}

struct fs_rlck *
get_me()
{
	used_me ++;
	return( (struct fs_rlck *) xmalloc(sizeof(struct fs_rlck)) );
}

remote_result *
get_res()
{
	used_res ++;
	return( (remote_result *) xmalloc(res_len) );
}

