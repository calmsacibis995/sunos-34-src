#ifndef lint
static char SccsId[] = "@(#)cache.c 1.1 86/09/25 SMI"; /* Copyright Sun Microsystems */
#endif

/* simple hash table cache for host names */

# include <stdio.h>
# include <ctype.h>
# include <sys/param.h>
# include <sys/socket.h>
# include <netdb.h>

# define HashSize 497

struct HashEnt
  {
    struct HashEnt *next;
    unsigned long addr;
    char name[64];
  } *Table[HashSize];

extern char *inet_ntoa();
extern struct hostent *gethostbyaddr();

InitHash()
{
  register struct HashEnt **h = Table;
  int i;

  for (i=0;i<HashSize;i++) *h++ = NULL;
}


char *LookupHost(addr)
    unsigned long addr;
  {
      /*
       * First look in the hash table, and return if found.
       * Otherwise malloc a new entry, chain onto the end,
       * and do a real host name lookup.  
       * Returns a pointer to the string part of the table entry.
       * Note the parameter must be treated as an UNSIGNED integer.
       */
	register struct HashEnt *h, *last;
	struct hostent *hp;

	h = Table[addr % HashSize];
	last = NULL;
	while (h != NULL) {
	    if (h->addr == addr) return(h->name);
	    last = h;
	    h = h->next;
	  }
	h = (struct HashEnt *)malloc(sizeof(struct HashEnt));
	if (h == NULL)
		return(inet_ntoa(addr));
	h->addr = addr;
	h->next = NULL;
	if (last==NULL)
	    Table[addr % HashSize] = h;
	else
	    last->next = h;

	hp = gethostbyaddr(&addr, 4, AF_INET);
	if (hp == NULL)
		strncpy(h->name,inet_ntoa(addr), sizeof(h->name));
	else
		strncpy(h->name, hp->h_name, sizeof(h->name));
	return(h->name);
  }
