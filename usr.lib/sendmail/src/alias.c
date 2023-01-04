# include <pwd.h>
# include <sys/file.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
# include <signal.h>
# include "sendmail.h"
# include "conf.h"

# ifdef DBM
SCCSID(@(#)alias.c 1.1 86/09/25 SMI (with DBM)); /* from UCB 4.10 4/29/85 */
# else DBM
SCCSID(@(#)alias.c 1.1 86/09/25 SMI (without DBM)); /* from UCB 4.10 4/29/85 */
# endif DBM

/*
**  ALIAS -- Compute aliases.
**
**	Scans the alias file for an alias for the given address.
**	If found, it arranges to deliver to the alias list instead.
**	Uses libdbm database if -DDBM.
**
**	Parameters:
**		a -- address to alias.
**		sendq -- a pointer to the head of the send queue
**			to put the aliases in.
**
**	Returns:
**		none
**
**	Side Effects:
**		Aliases found are expanded.
**
**	Notes:
**		If NoAlias (the "-n" flag) is set, no aliasing is
**			done.
**
**	Deficiencies:
**		It should complain about names that are aliased to
**			nothing.
*/


#ifdef DBM
typedef struct
{
	char	*dptr;
	int	dsize;
} DATUM;
extern DATUM fetch();
#endif DBM

char *ypDomain = NULL;

alias(a, sendq)
	register ADDRESS *a;
	ADDRESS **sendq;
{
	register char *p;
	extern char *aliaslookup(), *yellowlookup();

# ifdef DEBUG
	if (tTd(27, 1))
		printf("alias(%s)\n", a->q_paddr);
# endif

	/* don't realias already aliased names */
	if (bitset(QDONTSEND, a->q_flags))
		return;

	CurEnv->e_to = a->q_paddr;

	/*
	**  Look up this name
	*/

	if (NoAlias) p = NULL;
	else p = aliaslookup(a->q_user);

	if (p == NULL)
		p = yellowlookup(a);

	if (p == NULL)
		return;

	/*
	**  Match on Alias.
	**	Deliver to the target list.
	*/

# ifdef DEBUG
	if (tTd(27, 1))
		printf("%s (%s, %s) aliased to %s\n",
		    a->q_paddr, a->q_host, a->q_user, p);
# endif
	message(Arpa_Info, "aliased to %s", p);
	AliasLevel++;
	sendtolist(p, a, sendq);
	AliasLevel--;
}
/*
**  ALIASLOOKUP -- look up a name in the alias file.
**
**	Parameters:
**		name -- the name to look up.
**
**	Returns:
**		the value of name.
**		NULL if unknown.
**
**	Side Effects:
**		none.
**
**	Warnings:
**		The return value will be trashed across calls.
*/

char *
aliaslookup(name)
	char *name;
{
# ifdef DBM
	DATUM rhs, lhs;

	/* create a key for fetch */
	lhs.dptr = name;
	lhs.dsize = strlen(name) + 1;
	rhs = fetch(lhs);
	return (rhs.dptr);
# else DBM
	register STAB *s;

	s = stab(name, ST_ALIAS, ST_FIND);
	if (s == NULL)
		return (NULL);
	return (s->s_alias);
# endif DBM
}
/*
**  YELLOWMATCH -- look up any token in the Yellow Pages.
**
**	Parameters:
**		string - string to look up
**		mac - macro to use as name of map
**
**	Returns:
**		True if the value was found in the database
**
**	Side Effects:
**		If token is found, enter into cache.
**
**	Warnings:
**		Will hang forever if no yellow pages server responds
*/

yellowmatch(string,mac)
    char *string;
    char mac;
  {
  	int insize, outsize;
	register STAB *s;
	char *mapname, *result, *macvalue();

	if (mac == 'y')
	  {
	    /*
	     * handle the special host name mapping macro.
	     */
	     struct hostinfo *h, *lookuphost();
	     h = lookuphost(string);
	     errno = 0;
	     return(h->h_exists);
	  }
	mapname = macvalue(mac,CurEnv);
	if (mapname==NULL) return(FALSE);
	s = stab(string, ST_CLASS, ST_FIND);
	if (s != NULL && bitnset(mac, s->s_class))
		return(TRUE);

	insize = strlen(string);
	if (ypDomain==NULL)
		{
		  yp_get_default_domain(&ypDomain);
		  if (ypDomain==NULL) return(FALSE);
# ifdef DEBUG
		  if (tTd(27, 1))
	    printf("Yellow pages domain is %s\n",ypDomain);
# endif DEBUG
		}
	if (yp_match(ypDomain,mapname,string, insize, &result, &outsize)
	 && yp_match(ypDomain,mapname,string, insize+1, &result, &outsize))
	  {
	  	errno = 0;
		return(FALSE);
	  }
# ifdef DEBUG
	if (tTd(27, 1))
	    printf("Yellow pages found %s as value for %s\n",result,string);
# endif DEBUG
	s = stab(string, ST_CLASS, ST_ENTER);
	setbitn(mac, s->s_class);
	free(result);
	return(TRUE);
  }

/*
**  YELLOWLOOKUP -- look up a name in the Yellow Pages.
**
**	Parameters:
**		a -- the address to look up.
**
**	Returns:
**		the value of name.
**		NULL if unknown.
**
**	Side Effects:
**		sets 
**
**	Warnings:
**		The return value will be trashed across calls.
*/

char *
yellowlookup(a)
	register ADDRESS *a;
  {
	char *result, *colon;
	int insize, outsize;
	extern char *AliasMap;
	
		/*
		 * if we did not find a local alias, then
		 * try a remote alias through yellow pages.
		 */
	if (AliasMap==NULL || *AliasMap=='\0') return(NULL);

	if (ypDomain==NULL)
		{
		  yp_get_default_domain(&ypDomain);
		  if (ypDomain==NULL) return(NULL);
# ifdef DEBUG
		  if (tTd(27, 1))
	    		printf("Yellow pages domain is %s\n",ypDomain);
# endif DEBUG
		}
	if (bitset(QWASLOCAL,a->q_flags)) return(NULL);
	insize = strlen(a->q_user)+1;
	if (yp_match(ypDomain,AliasMap,a->q_user, insize, &result, &outsize))
	  {
    	    errno = 0;
	    return(NULL);
	  }
  
# ifdef DEBUG
        if (tTd(27, 1))
		printf("%s maps to %s\n",a->q_user, result );
# endif DEBUG
	colon = index(result,':');
	if (index(result,'|') || 
	    (colon && strncmp(colon, ":include:", 9) == 0))
	  {
	  	/*
		 * Don't do domain-aliases for include files or pipes -
		 * just punt to the domain registry machine.
		 */
	    char buf[MAXNAME];
	    strcpy(buf,a->q_user);
	    strcat(buf,"@");
	    strcat(buf,ypDomain);
	    return(newstr(buf));
	  }
	a->q_flags |= QDOMAIN;
	return(result);
  }
/*
**  INITALIASES -- initialize for aliasing
**
**	Very different depending on whether we are running DBM or not.
**
**	Parameters:
**		aliasfile -- location of aliases.
**		init -- if set and if DBM, initialize the DBM files.
**
**	Returns:
**		none.
**
**	Side Effects:
**		initializes aliases:
**		if DBM:  opens the database.
**		if ~DBM: reads the aliases into the symbol table.
*/

# define DBMMODE	0666

initaliases(aliasfile, init)
	char *aliasfile;
	bool init;
{
#ifdef DBM
	int atcnt;
	char buf[MAXNAME];
	time_t modtime;
#endif DBM
	struct stat stb;

	if (aliasfile == NULL || stat(aliasfile, &stb) < 0)
	{
		NoAlias = TRUE;
		errno = 0;
		return;
	}

# ifdef DBM
	/*
	**  Check to see that the alias file is complete.
	**	If not, we will assume that someone died, and it is up
	**	to us to rebuild it.
	*/

	if (!init) dbminit(aliasfile);
	atcnt = SafeAlias * 2;
	if (atcnt > 0)
	{
		while (!init && atcnt-- >= 0 && aliaslookup("@") == NULL)
		{
			  /*
			   * we need to keep reopening the file in case
			   * the old one was removed and replaced with a new
			   * file (instead of copied or rewritten in place).
			   */
			sleep(30);
			dbmclose();
			dbminit(aliasfile);
		}
	}
	else
		atcnt = 1;

	/*
	**  See if the DBM version of the file is out of date with
	**  the text version.  If so, go into 'init' mode automatically.
	**	This only happens if our effective userid owns the DBM
	**	version or if the mode of the database is 666 -- this
	**	is an attempt to avoid protection problems.  Note the
	**	unpalatable hack to see if the stat succeeded.
	*/

	modtime = stb.st_mtime;
	(void) strcpy(buf, aliasfile);
	(void) strcat(buf, ".pag");
	stb.st_ino = 0;
	if (!init && (stat(buf, &stb) < 0 || stb.st_mtime < modtime || atcnt < 0))
	{
		errno = 0;
		if (AutoRebuild && stb.st_ino != 0 &&
		    ((stb.st_mode & 0777) == 0666 || stb.st_uid == geteuid()))
		{
			init = TRUE;
#ifdef LOG
			syslog(LOG_WARNING, "alias database being rebuilt");
#endif LOG
			message(Arpa_Info, "rebuilding alias database");
		}
		else
		{
#ifdef LOG
			syslog(LOG_WARNING, "alias database out of date");
#endif LOG
			message(Arpa_Info, "Warning: alias database out of date");
		}
	}


	/*
	**  If necessary, load the DBM file.
	**	If running without DBM, load the symbol table.
	*/

	if (init)
	{
		readaliases(aliasfile, TRUE, modtime);
	}
# else DBM
	readaliases(aliasfile, init, modtime);
# endif DBM
}
/*
**  READALIASES -- read and process the alias file.
**
**	This routine implements the part of initaliases that occurs
**	when we are not going to use the DBM stuff.
**
**	Parameters:
**		aliasfile -- the pathname of the alias file master.
**		init -- if set, initialize the DBM stuff.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Reads aliasfile into the symbol table.
**		Optionally, builds the .dir & .pag files.
*/

static
readaliases(aliasfile, init, modtime)
	char *aliasfile;
	bool init;
	time_t modtime;
{
	register char *p;
	char *p2;
	char *rhs;
	bool skipping;
	int naliases, bytes, longest;
	int seenRemote;
	FILE *af;
	int (*oldsigint)();
	ADDRESS al, bl;
	register STAB *s;
	char line[BUFSIZ];

	if ((af = fopen(aliasfile, "r")) == NULL)
	{
# ifdef DEBUG
		if (tTd(27, 1))
			printf("Can't open %s\n", aliasfile);
# endif
		errno = 0;
		NoAlias++;
		return;
	}

# ifdef DBM
# ifdef FLOCK
	/* see if someone else is rebuilding the alias file already */
	if (flock(fileno(af), LOCK_EX | LOCK_NB) < 0 && errno == EWOULDBLOCK)
	{
		/* yes, they are -- wait until done and then return */
		message(Arpa_Info, "Alias file is already being rebuilt");
		if (OpMode != MD_INITALIAS)
		{
			/* wait for other rebuild to complete */
			(void) flock(fileno(af), LOCK_EX);
		}
		fclose(af);
		errno = 0;
		return;
	}
# endif FLOCK
# endif DBM

	/*
	**  If initializing, create the new DBM files, then reopen them
	**  in case they did not previously exist.
	*/

	if (init)
	{
		oldsigint = signal(SIGINT, SIG_IGN);
		(void) strcpy(line, aliasfile);
		(void) strcat(line, ".dir");
		if (close(creat(line, DBMMODE)) < 0)
		{
			syserr("cannot make %s", line);
			(void) signal(SIGINT, oldsigint);
			return;
		}
		(void) strcpy(line, aliasfile);
		(void) strcat(line, ".pag");
		if (close(creat(line, DBMMODE)) < 0)
		{
			syserr("cannot make %s", line);
			(void) signal(SIGINT, oldsigint);
			return;
		}
		if (dbminit(aliasfile)<0)
		{
			syserr("cannot open database %s",aliasfile);
			(void) signal(SIGINT, oldsigint);
			return;
		}
	}

	/*
	**  Read and interpret lines
	*/

	FileName = aliasfile;
	LineNumber = 0;
	naliases = bytes = longest = 0;
	skipping = FALSE;
	while (fgets(line, sizeof (line), af) != NULL)
	{
		int lhssize, rhssize;

		LineNumber++;
		switch (line[0])
		{
		  case '#':
		  case '\n':
		  case '\0':
			skipping = FALSE;
			continue;

		  case ' ':
		  case '\t':
			if (!skipping)
				syserr("Non-continuation line starts with space");
			skipping = TRUE;
			continue;
		}
		skipping = FALSE;

		/*
		**  Process the LHS
		**	Find the final colon, and parse the address.
		**	It should resolve to a local name -- this will
		**	be checked later (we want to optionally do
		**	parsing of the RHS first to maximize error
		**	detection).
		*/

		seenRemote = 0;
		for (p = line; *p != '\0' && *p != ':' && *p != '\n'; p++)
			if ( *p == '@' || *p == '!') seenRemote = 1;
		if (*p++ != ':')
		{
			syserr("missing colon");
			continue;
		}
		if (parseaddr(line, &al, 0, ':') == NULL)
		    {
			syserr("illegal alias name");
			continue;
		    }
		loweraddr(&al);
		if (al.q_mailer != LocalMailer)
		    {
			if (seenRemote)
			{
			syserr("cannot alias non-local names");
			continue;
		        }
		    }

		/*
		**  Process the RHS.
		**	'al' is the internal form of the LHS address.
		**	'p' points to the text of the RHS.
		*/

		rhs = p;
		for (;;)
		{
			register char c;

			if (init)
			{
				/* do parsing & compression of addresses */
				c = *p;
				while (c != '\0')
				{
					seenRemote = 0;
					p2 = p;
					while (*p != '\n' && *p != ',' && *p != '\0') {
				if ( *p == '@' || *p == '!') seenRemote = 1;
						p++;
					}
					c = *p;
					if (c == '\n')
						c = '\0';
					*p = '\0';
					while (isspace(*p2)) p2++;
					if (*p2 != '\0' && 
					   seenRemote &&
					   parseaddr(p2, &bl, -1, ',') == NULL)
					    {
					      printf("Bad address: %s\n",p2);
					    }
					if (c != '\0')
						*p++ = c;
				}
			}
			else
			{
				p = &p[strlen(p)];
				if (p[-1] == '\n')
					*--p = '\0';
			}

			/* see if there should be a continuation line */
			c = fgetc(af);
			if (!feof(af))
				(void) ungetc(c, af);
			if (c != ' ' && c != '\t')
				break;

			/* read continuation line */
			if (fgets(p, sizeof line - (p - line), af) == NULL)
				break;
			LineNumber++;
		}
		/*
		**  Insert alias into symbol table or DBM file
		*/

		lhssize = strlen(al.q_user) + 1;
		rhssize = strlen(rhs) + 1;

# ifdef DBM
		if (init)
		{
			DATUM key, content;

			key.dsize = lhssize;
			key.dptr = al.q_user;
			content.dsize = rhssize;
			content.dptr = rhs;
			store(key, content);
			free(al.q_user);
			al.q_user = NULL;
		}
		else
# endif DBM
		{
			s = stab(al.q_user, ST_ALIAS, ST_ENTER);
			s->s_alias = newstr(rhs);
		}

		/* statistics */
		naliases++;
		bytes += lhssize + rhssize;
		if (rhssize > longest)
			longest = rhssize;
	}

# ifdef DBM
	if (init)
	{
		/* add the distinquished alias "@" */
		DATUM key, value;
		char last_modified[16];

		    /*
		     * Add the special Yellow pages entries. We can do this
		     * without harm even if this host is not itself a YP server
		     */
		key.dptr = "YP_LAST_MODIFIED";
		key.dsize = strlen(key.dptr);
		sprintf(last_modified,"%10.10d",modtime);
		value.dptr = last_modified;
		value.dsize = strlen(value.dptr);
		store(key, value);

		key.dptr = "YP_MASTER_NAME";
		key.dsize = strlen(key.dptr);
		value.dptr = macvalue('w',CurEnv);
		value.dsize = strlen(value.dptr);
		store(key, value);

		key.dsize = 2;
		key.dptr = "@";
		store(key, key);

		/* restore the old signal */
		(void) signal(SIGINT, oldsigint);
	}
# endif DBM

	/* closing the alias file drops the lock */
	(void) fclose(af);
	CurEnv->e_to = NULL;
	FileName = NULL;
	message(Arpa_Info, "%d aliases, longest %d bytes, %d bytes total",
			naliases, longest, bytes);
 	syslog(LOG_WARNING, "newaliases: %d aliases, longest %d bytes, %d bytes total",
			naliases, longest, bytes);
}
/*
**  FORWARD -- Try to forward mail
**
**	This is similar but not identical to aliasing.
**
**	Parameters:
**		user -- the name of the user who's mail we would like
**			to forward to.  It must have been verified --
**			i.e., the q_home field must have been filled
**			in.
**		sendq -- a pointer to the head of the send queue to
**			put this user's aliases in.
**
**	Returns:
**		none.
**
**	Side Effects:
**		New names are added to send queues.
*/

forward(user, sendq)
	ADDRESS *user;
	ADDRESS **sendq;
{
	char buf[60];
	extern bool safefile();

# ifdef DEBUG
	if (tTd(27, 1))
		printf("forward(%s)\n", user->q_paddr);
# endif DEBUG

	if (user->q_mailer != LocalMailer || bitset(QBADADDR, user->q_flags))
		return;
# ifdef DEBUG
	if (user->q_home == NULL)
		syserr("forward: no home");
# endif DEBUG

	/* good address -- look for .forward file in home */
	define('z', user->q_home, CurEnv);
	expand("\001z/.forward", buf, &buf[sizeof buf - 1], CurEnv);
	if (!safefile(buf, user->q_uid, S_IREAD))
		return;

	/* we do have an address to forward to -- do it */
	include(buf, "forwarding", user, sendq);
}
