
static char sccsid[] = "@(#)mtest.c	1.1 (Sun) 9/24/86";

/*
 * a continuous test of random sequences of malloc(), free(), and realloc()
 */

#include <stdio.h>

#define LISTSIZE 256
#define NACTIONS 4
#define MAXSIZE	 1024 

/* format of data blocks */
#define LEN 0
#define CHK 1
#define DATA 2

typedef int* ptr;
typedef enum { Free, Malloc, Valloc, Realloc } actions;
typedef enum { Vacant, Occupied } slottype;

ptr	freelist[LISTSIZE];	/* items freed since last allocation call */
ptr	busylist[LISTSIZE];	/* allocated items */

extern unsigned long random();
extern getpagesize();
extern ptr valloc(), malloc(), realloc();

int count;
actions act;

error(s,x1,x2,x3,x4)
	char *s;
{
	fprintf(stderr,s,x1,x2,x3,x4);
	fflush(stderr);
	abort();
}

actions
nextaction()
{
	return (actions)(random() % NACTIONS);
}

flip()
{
	return (random() & 1);
}

clearfree()
{
	register int n;
	for (n = 0; n < LISTSIZE; n++)
		freelist[n] = NULL;
}

clearbusy()
{
	register int n, *p;
	for (n = 0; n < LISTSIZE; n++) {
		p = busylist[n];
		if (p != NULL) {
			free(p);
			busylist[n] = NULL;
		}
	}
}

int
mksize(min, max)
{
	return ( random() % (max-min+1) ) + min;
}

ptr *
findslot(list,type)
	register ptr list[];
	register slottype type;
{
	register k,n;
	n = (random() % LISTSIZE);
	for ( k = (n+1) % LISTSIZE ; k != n ; k = (k+1) % LISTSIZE ) {
		if (type == Vacant) {
			/* need an empty slot */
			if (list[k] == NULL)
				return list + k;
		} else {
			/* need an occupied slot */
			if (list[k] != NULL)
				return list + k;
		}
	}
	return NULL;
}

filldata(list, n)
	register int *list;
	int n;
{
	register k;
	long x, chksum;

	chksum = 0;
	if (n > MAXSIZE) {
		error("filldata: invalid length (%d)\n", n);
	}
	for (k = DATA; k < n; k++) {
		x = random();
		list[k] = x;
		chksum += x;
	}
	list[LEN] = n;
	list[CHK] = chksum;
}

checkdata(list)
	register int list[];
{
	register k,n;
	long x, chksum;
	n = list[LEN];
	if (n > MAXSIZE) {
		error("checkdata: invalid length (%d)\n", n);
	}
	chksum = 0;
	for (k = DATA; k < n; k++) {
		chksum += list[k];
	}
	if (chksum != list[CHK]) {
		error("checkdata: invalid checksum\n");
	}
}

main(argc,argv)
	int argc;
	char *argv[];
{
	int n;
	int size;
	ptr *pp;
	ptr p;
	enum { Freelist, Busylist } which;
	int interval = 400;
	int pagesize = getpagesize();

	if (argc == 2)
		interval = atoi(argv[1]);

#ifdef	DEBUG
	malloc_debug(1);
#endif	DEBUG

	count = 0;

	for(;;) {
		
		if ((++count) % interval == 0 ) {
			prfree();
			mallocmap();
		}

		switch (act = nextaction()) {

		case Free:
			/*
			 * Find a previously allocated data block,
			 * check its contents, and free it.
			 */
			pp = findslot(busylist,Occupied);
			if (pp != NULL) {
				p = *pp;
				checkdata(p);
				free(p);
				*pp = NULL;
				pp = findslot(freelist, Vacant);
				if (pp == NULL) {
					clearfree();
					pp = findslot(freelist, Vacant);
				}
				*pp = p;
			}
			continue;

		case Valloc:
			/*
			 * Find a place to store a pointer to some data,
			 * allocate a random size block, initialize it,
			 * and stash it.
			 */
			pp = findslot(busylist, Vacant);
			if (pp != NULL) {
				size = mksize(DATA,MAXSIZE);
				p = valloc(size*sizeof(int));
				if ((int)p % pagesize) {
					error("valloc: misaligned (%#x)\n", p);
				}
				filldata(p,size);
				*pp = p;
			}
			clearfree();
			continue;

		case Malloc:
			/*
			 * Find a place to store a pointer to some data,
			 * allocate a random size block, initialize it,
			 * and stash it.
			 */
			pp = findslot(busylist, Vacant);
			if (pp == NULL) {
				clearbusy();
			} else {
				size = mksize(DATA,MAXSIZE);
				p = malloc(size*sizeof(int));
				filldata(p,size);
				*pp = p;
			}
			clearfree();
			continue;

		case Realloc:
			/*
			 * similer to Malloc, but sometimes try reallocating
			 * an object freed since the last call to malloc()
			 * or realloc().
			 */
			if (flip()) {
				/* try reallocating something in the freelist */
				pp = findslot(freelist, Occupied);
				which = Freelist;
			} else {
				/* try reallocating something that's busy */
				pp = findslot(busylist, Occupied);
				which = Busylist;
			}
			if (pp != NULL) {
				p = *pp;
				checkdata(p);
				size = mksize(DATA,MAXSIZE);
				if (size < p[LEN]) {
					checkdata(p);	/* shrinking */
					filldata(p,size);
				}
				p = realloc(p, size*sizeof(int));
				checkdata(p);	/* check old data */
				if (which == Freelist) {
					*pp = NULL; /* no longer free */
					pp = findslot(busylist, Vacant);
					if (pp == NULL) {
						/*
						 * no empty slots in busy list;
						 * make one up.
						 */
						free(busylist[0]);
						pp = &busylist[0];
					}
				}
				*pp = p;
			}
			clearfree();
			continue;

		} /*switch*/
	} /*for*/
}
