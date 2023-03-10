#ifndef lint
static	char sccsid[] = "@(#)getpwent.c 1.1 86/09/24 SMI";
#endif

/* 
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <pwd.h>
#include <rpcsvc/ypclnt.h>


#define MAXINT 0x7fffffff;

extern void rewind();
extern long strtol();
extern int strcmp();
extern int strlen();
extern int fclose();
extern char *strcpy();
extern char *strncpy();
extern char *malloc();

void setpwent(), endpwent();

static char domain[256];
static struct passwd NULLPW = {NULL, NULL, 0, 0, 0, NULL, NULL, NULL, NULL};
static char PASSWD[]	= "/etc/passwd"; 
static char EMPTY[] = "";
static FILE *pwf = NULL;	/* pointer into /etc/passwd */
static char *yp;		/* pointer into yellow pages */
static int yplen;
static char *oldyp = NULL;	
static int oldyplen;
static struct passwd *interpret();
static struct passwd *interpretwithsave();
static struct passwd *save();
static struct passwd *getnamefromyellow();
static struct passwd *getuidfromyellow();



static struct list {
    char *name;
    struct list *nxt;
} *minuslist;			/* list of - items */

struct passwd *
getpwnam(name)
	register char *name;
{
	struct passwd *pw;
	char line[BUFSIZ+1];

	setpwent();
	if (!pwf)
		return NULL;
	while (fgets(line, BUFSIZ, pwf) != NULL) {
		if ((pw = interpret(line, strlen(line))) == NULL)
			continue;
		if (matchname(line, &pw, name)) {
			endpwent();
			return pw;
		}
	}
	endpwent();
	return NULL;
}

struct passwd *
getpwuid(uid)
	register uid;
{
	struct passwd *pw;
	char line[BUFSIZ+1];

	setpwent();
	if (!pwf)
		return NULL;
	while (fgets(line, BUFSIZ, pwf) != NULL) {
		if ((pw = interpret(line, strlen(line))) == NULL)
			continue;
		if (matchuid(line, &pw, uid)) {
			endpwent();
			return pw;
		}
	}
	endpwent();
	return NULL;
}




void
setpwent()
{
	if (getdomainname(domain, sizeof(domain)) < 0) {
		(void) fprintf(stderr, 
		    "setpwent: getdomainname system call missing\n");
		exit(1);
	}
	if (pwf == NULL)
		pwf = fopen(PASSWD, "r");
	else
		rewind(pwf);
	if (yp)
		free(yp);
	yp = NULL;
	freeminuslist();
}



void
endpwent()
{
	if (pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
	if (yp)
		free(yp);
	yp = NULL;
	freeminuslist();
	endnetgrent();
}


struct passwd *
fgetpwent(f)
	FILE *f;
{
	char line1[BUFSIZ+1];

	if(fgets(line1, BUFSIZ, f) == NULL)
		return(NULL);
	return (interpret(line1, strlen(line1)));
}

static char *
pwskip(p)
	register char *p;
{
	while(*p && *p != ':' && *p != '\n')
		++p;
	if (*p == '\n')
		*p = '\0';
	else if (*p != '\0')
		*p++ = '\0';
	return(p);
}



struct passwd *
getpwent()
{
	char line1[BUFSIZ+1];
	static struct passwd *savepw;
	struct passwd *pw;
	char *user; 
	char *mach;
	char *dom;

	if (domain[0] == 0 && getdomainname(domain, sizeof(domain)) < 0) {
		(void) fprintf(stderr, 
		    "getpwent: getdomainname system call missing\n");
		exit(1);
	}
	if (pwf == NULL && (pwf = fopen(PASSWD, "r")) == NULL) {
		return (NULL); 
	}

	for (;;) {
		if (yp) {
			pw = interpretwithsave(yp, yplen, savepw); 
			free(yp);
			if (pw == NULL)
				return(NULL);
			getnextfromyellow();
			if (!onminuslist(pw)) {
				return(pw);
			}
		} else if (getnetgrent(&mach,&user,&dom)) {
			if (user) {
				pw = getnamefromyellow(user, savepw);
				if (pw != NULL && !onminuslist(pw)) {
					return(pw);
				}
			}
		} else {
			endnetgrent();
			if (fgets(line1, BUFSIZ, pwf) == NULL)  {
				return(NULL);
			}
			if ((pw = interpret(line1, strlen(line1))) == NULL)
				return(NULL);
			switch(line1[0]) {
			case '+':
				if (strcmp(pw->pw_name, "+") == 0) {
					getfirstfromyellow();
					savepw = save(pw);
				} else if (line1[1] == '@') {
					savepw = save(pw);
					if (innetgr(pw->pw_name+2,(char *) NULL,"*",domain)) {
						/* include the whole yp database */
						getfirstfromyellow();
					} else {
						setnetgrent(pw->pw_name+2);
					}
				} else {
					/* 
					 * else look up this entry in yellow pages
				 	 */
					savepw = save(pw);
					pw = getnamefromyellow(pw->pw_name+1, savepw);
					if (pw != NULL && !onminuslist(pw)) {
						return(pw);
					}
				}
				break;
			case '-':
				if (line1[1] == '@') {
					if (innetgr(pw->pw_name+2,(char *) NULL,"*",domain)) {
						/* everybody was subtracted */
						return(NULL);
					}
					setnetgrent(pw->pw_name+2);
					while (getnetgrent(&mach,&user,&dom)) {
						if (user) {
							addtominuslist(user);
						}
					}
					endnetgrent();
				} else {
					addtominuslist(pw->pw_name+1);
				}
				break;
			default:
				if (!onminuslist(pw)) {
					return(pw);
				}
				break;
			}
		}
	}
}

static
matchname(line1, pwp, name)
	char line1[];
	struct passwd **pwp;
	char *name;
{
	struct passwd *savepw;
	struct passwd *pw = *pwp;

	switch(line1[0]) {
		case '+':
			if (strcmp(pw->pw_name, "+") == 0) {
				savepw = save(pw);
				pw = getnamefromyellow(name, savepw);
				if (pw) {
					*pwp = pw;
					return 1;
				}
				else
					return 0;
			}
			if (line1[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,name,domain)) {
					savepw = save(pw);
					pw = getnamefromyellow(name,savepw);
					if (pw) {
						*pwp = pw;
						return 1;
					}
				}
				return 0;
			}
			if (strcmp(pw->pw_name+1, name) == 0) {
				savepw = save(pw);
				pw = getnamefromyellow(pw->pw_name+1, savepw);
				if (pw) {
					*pwp = pw;
					return 1;
				}
				else
					return 0;
			}
			break;
		case '-':
			if (line1[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,name,domain)) {
					*pwp = NULL;
					return 1;
				}
			}
			else if (strcmp(pw->pw_name+1, name) == 0) {
				*pwp = NULL;
				return 1;
			}
			break;
		default:
			if (strcmp(pw->pw_name, name) == 0)
				return 1;
	}
	return 0;
}

static
matchuid(line1, pwp, uid)
	char line1[];
	struct passwd **pwp;
{
	struct passwd *savepw;
	struct passwd *pw = *pwp;
	char group[256];

	switch(line1[0]) {
		case '+':
			if (strcmp(pw->pw_name, "+") == 0) {
				savepw = save(pw);
				pw = getuidfromyellow(uid, savepw);
				if (pw) {
					*pwp = pw;
					return 1;
				} else {
					return 0;
				}
			}
			if (line1[1] == '@') {
				(void) strcpy(group,pw->pw_name+2);
				savepw = save(pw);
				pw = getuidfromyellow(uid,savepw);
				if (pw && innetgr(group,(char *) NULL,pw->pw_name,domain)) {
					*pwp = pw;
					return 1;
				} else {
					return 0;
				}
			}
			savepw = save(pw);
			pw = getnamefromyellow(pw->pw_name+1, savepw);
			if (pw && pw->pw_uid == uid) {
				*pwp = pw;
				return 1;
			} else
				return 0;
			break;
		case '-':
			if (line1[1] == '@') {
				(void) strcpy(group,pw->pw_name+2);
				pw = getuidfromyellow(uid,&NULLPW);
				if (pw && innetgr(group,(char *) NULL,pw->pw_name,domain)) {
					*pwp = NULL;
					return 1;
				}
			} else if (uid == uidof(pw->pw_name+1)) {
				*pwp = NULL;
				return 1;
			}
			break;
		default:
			if (pw->pw_uid == uid)
				return 1;
	}
	return 0;
}

static
uidof(name)
	char *name;
{
	struct passwd *pw;
	
	pw = getnamefromyellow(name, &NULLPW);
	if (pw)
		return pw->pw_uid;
	else
		return MAXINT;
}

static
getnextfromyellow()
{
	int reason;
	char *key;
	int keylen;

	reason = yp_next(domain, "passwd.byname",oldyp, oldyplen, &key
	    ,&keylen,&yp,&yplen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		yp = NULL;
	}
	if (oldyp)
		free(oldyp);
	oldyp = key;
	oldyplen = keylen;
}

static
getfirstfromyellow()
{
	int reason;
	char *key;
	int keylen;
	
	reason =  yp_first(domain, "passwd.byname", &key, &keylen, &yp, &yplen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
		yp = NULL;
	}
	if (oldyp)
		free(oldyp);
	oldyp = key;
	oldyplen = keylen;
}

static struct passwd *
getnamefromyellow(name, savepw)
	char *name;
	struct passwd *savepw;
{
	struct passwd *pw;
	int reason;
	char *val;
	int vallen;
	
	reason = yp_match(domain, "passwd.byname", name, strlen(name)
		, &val, &vallen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		return NULL;
	} else {
		pw = interpret(val, vallen);
		free(val);
		if (pw == NULL)
			return NULL;
		if (savepw->pw_passwd && *savepw->pw_passwd)
			pw->pw_passwd =  savepw->pw_passwd;
		if (savepw->pw_gecos && *savepw->pw_gecos)
			pw->pw_gecos = savepw->pw_gecos;
		if (savepw->pw_dir && *savepw->pw_dir)
			pw->pw_dir = savepw->pw_dir;
		if (savepw->pw_shell && *savepw->pw_shell)
			pw->pw_shell = savepw->pw_shell;
		return pw;
	}
}

static struct passwd *
getuidfromyellow(uid, savepw)
	int uid;
	struct passwd *savepw;
{
	struct passwd *pw;
	int reason;
	char *val;
	int vallen;
	char uidstr[20];
	
	(void) sprintf(uidstr, "%d", uid);
	reason = yp_match(domain, "passwd.byuid", uidstr, strlen(uidstr)
		, &val, &vallen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		return NULL;
	} else {
		pw = interpret(val, vallen);
		free(val);
		if (pw == NULL)
			return NULL;
		if (savepw->pw_passwd && *savepw->pw_passwd)
			pw->pw_passwd =  savepw->pw_passwd;
		if (savepw->pw_gecos && *savepw->pw_gecos)
			pw->pw_gecos = savepw->pw_gecos;
		if (savepw->pw_dir && *savepw->pw_dir)
			pw->pw_dir = savepw->pw_dir;
		if (savepw->pw_shell && *savepw->pw_shell)
			pw->pw_shell = savepw->pw_shell;
		return pw;
	}
}

static struct passwd *
interpretwithsave(val, len, savepw)
	char *val;
	struct passwd *savepw;
{
	struct passwd *pw;
	
	if ((pw = interpret(val, len)) == NULL)
		return NULL;
	if (savepw->pw_passwd && *savepw->pw_passwd)
		pw->pw_passwd =  savepw->pw_passwd;
	if (savepw->pw_gecos && *savepw->pw_gecos)
		pw->pw_gecos = savepw->pw_gecos;
	if (savepw->pw_dir && *savepw->pw_dir)
		pw->pw_dir = savepw->pw_dir;
	if (savepw->pw_shell && *savepw->pw_shell)
		pw->pw_shell = savepw->pw_shell;
	return pw;
}

static struct passwd *
interpret(val, len)
	char *val;
{
	register char *p;
	char *end;
	long x;
	static struct passwd passwd;
	static char line[BUFSIZ+1];
	register int ypentry;

	(void) strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	line[len+1] = 0;

	/*
 	 * Set "ypentry" if this entry references the Yellow Pages;
	 * if so, null UIDs and GIDs are allowed (because they will be
	 * filled in from the matching Yellow Pages entry).
	 */
	ypentry = (*p == '+');

	passwd.pw_name = p;
	p = pwskip(p);
	passwd.pw_passwd = p;
	p = pwskip(p);
	if (*p == ':' && !ypentry)
		/* check for non-null uid */
		return (NULL);
	x = strtol(p, &end, 10);
	p = end;
	if (*p++ != ':' && !ypentry)
		/* check for numeric value - must have stopped on the colon */
		return (NULL);
	passwd.pw_uid = x;
	if (*p == ':' && !ypentry)
		/* check for non-null gid */
		return (NULL);
	x = strtol(p, &end, 10);	
	p = end;
	if (*p++ != ':' && !ypentry)
		/* check for numeric value - must have stopped on the colon */
		return (NULL);
	passwd.pw_gid = x;
	passwd.pw_quota = 0;
	passwd.pw_comment = EMPTY;
	passwd.pw_gecos = p;
	p = pwskip(p);
	passwd.pw_dir = p;
	p = pwskip(p);
	passwd.pw_shell = p;
	while(*p && *p != '\n') p++;
	*p = '\0';
	return(&passwd);
}

static
freeminuslist() {
	struct list *ls;
	
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		free(ls->name);
		free((char *) ls);
	}
	minuslist = NULL;
}

static
addtominuslist(name)
	char *name;
{
	struct list *ls;
	char *buf;
	
	ls = (struct list *) malloc(sizeof(struct list));
	buf = malloc((unsigned) strlen(name) + 1);
	(void) strcpy(buf, name);
	ls->name = buf;
	ls->nxt = minuslist;
	minuslist = ls;
}

/* 
 * save away psswd, gecos, dir and shell fields, which are the only
 * ones which can be specified in a local + entry to override the
 * value in the yellow pages
 */
static struct passwd *
save(pw)
	struct passwd *pw;
{
	static struct passwd *sv;

	/* free up stuff from last call */
	if (sv) {
		free(sv->pw_passwd);
		free(sv->pw_gecos);
		free(sv->pw_dir);
		free(sv->pw_shell);
		free((char *) sv);
	}
	sv = (struct passwd *) malloc(sizeof(struct passwd));

	sv->pw_passwd = malloc((unsigned) strlen(pw->pw_passwd) + 1);
	(void) strcpy(sv->pw_passwd, pw->pw_passwd);

	sv->pw_gecos = malloc((unsigned) strlen(pw->pw_gecos) + 1);
	(void) strcpy(sv->pw_gecos, pw->pw_gecos);

	sv->pw_dir = malloc((unsigned) strlen(pw->pw_dir) + 1);
	(void) strcpy(sv->pw_dir, pw->pw_dir);

	sv->pw_shell = malloc((unsigned) strlen(pw->pw_shell) + 1);
	(void) strcpy(sv->pw_shell, pw->pw_shell);

	return sv;
}

static
onminuslist(pw)
	struct passwd *pw;
{
	struct list *ls;
	register char *nm;

	nm = pw->pw_name;
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		if (strcmp(ls->name,nm) == 0) {
			return(1);
		}
	}
	return(0);
}
