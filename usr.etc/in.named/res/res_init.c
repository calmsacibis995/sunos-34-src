/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)res_init.c 1.1 86/09/25 SMI"; /* from UCB 6.2 11/26/85 */
#endif not lint

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/nameser.h>
#include <arpa/resolv.h>

/*
 * Resolver configuration file. Contains the address of the
 * inital name server to query and the default domain for
 * non fully qualified domain names.
 */

#ifdef CONFFILE
char    *conffile = CONFFILE;
#else
char    *conffile = "/etc/resolv.conf";
#endif

/*
 * Resolver state default settings
 */

#ifndef RES_TIMEOUT
#define RES_TIMEOUT 6
#endif

struct state _res = {
    RES_TIMEOUT,                 /* retransmition time interval */
    4,                           /* number of times to retransmit */
    RES_RECURSE|RES_DEFNAMES,    /* options flags */
    1,                           /* number of name servers */
};

/*
 * Set up default settings.  If the configuration file exist, the values
 * there will have precedence.  Otherwise, the server address is set to
 * INADDR_ANY and the default domain name comes from the gethostname().
 *
 * The configuration file should only be used if you want to redefine your
 * domain or run without a server on your machine.
 *
 * Return 0 if completes successfully, -1 on error
 */
res_init()
{
    register FILE *fp;
    char buf[BUFSIZ], *cp;
    extern u_long inet_addr();
    extern char *index();
    extern char *strcpy(), *strncpy();
#ifdef DEBUG
    extern char *getenv();
#endif DEBUG
    int n = 0;    /* number of nameserver records read from file */
    int a = 0;	 /* address preference counter */

    _res.nsaddr.sin_addr.s_addr = /** INADDR_ANY; **/ 0xEF000001;
    _res.nsaddr.sin_family = AF_INET;
    _res.nsaddr.sin_port = htons(NAMESERVER_PORT);
    _res.nscount = 1;
    getdomainname(_res.defdname, MAXDNAME );

    if ((fp = fopen(conffile, "r")) != NULL) {
        /* read the config file */
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            /* read default domain name */
            if (!strncmp(buf, "domain", sizeof("domain") - 1)) {
                cp = buf + sizeof("domain") - 1;
                while (*cp == ' ' || *cp == '\t')
                    cp++;
                if (*cp == '\0')
                    continue;
                (void)strncpy(_res.defdname, cp, sizeof(_res.defdname));
                _res.defdname[sizeof(_res.defdname) - 1] = '\0';
                if ((cp = index(_res.defdname, '\n')) != NULL)
                    *cp = '\0';
                continue;
            }
            /* read nameservers to query */
            if (!strncmp(buf, "nameserver", 
               sizeof("nameserver") - 1) && (n < MAXNS)) {
                cp = buf + sizeof("nameserver") - 1;
                while (*cp == ' ' || *cp == '\t')
                    cp++;
                if (*cp == '\0')
                    continue;
                _res.nsaddr_list[n].sin_addr.s_addr = inet_addr(cp);
                if (_res.nsaddr_list[n].sin_addr.s_addr == (unsigned)-1) 
                    _res.nsaddr_list[n].sin_addr.s_addr = INADDR_ANY;
                    _res.nsaddr_list[n].sin_family = AF_INET;
                    _res.nsaddr_list[n].sin_port = htons(NAMESERVER_PORT);
                    if ( ++n >= MAXNS) { 
                       n = MAXNS;
#ifdef DEBUG
                       if ( _res.options & RES_DEBUG )
                          printf("MAXNS reached, reading resolv.conf\n");
#endif DEBUG
                }
                continue;
	    }
	    /* read addresses to prefer */
	    if ((strncmp(buf, "address", 7) == 0) && (a < MAXADDR)) {
		cp = buf + 7;
		while (*cp == ' ' || *cp == '\t')
		    cp++;
		if (*cp == '\0')
		    continue;
		_res.sort_list[a].s_addr = inet_addr(cp);
		if (_res.sort_list[a].s_addr == (unsigned)-1)
		    continue;
		if (++a >= MAXADDR) {
		    a = MAXADDR;
#ifdef DEBUG
		    if (_res.options & RES_DEBUG)
			printf("MAXADDR reached, reading resolv.conf\n");
#endif /* DEBUG */
		}
            }
        }
        if ( n > 1 ) 
            _res.nscount = n;
	_res.ascount = a;
        (void) fclose(fp);
    }
    if (_res.defdname[0] == 0) {
        if (gethostname(buf, sizeof(_res.defdname)) == 0 &&
           (cp = index(buf, '.')))
             (void)strcpy(_res.defdname, cp + 1);
    }

#ifdef DEBUG
    /* Allow user to override the local domain definition */
    if ((cp = getenv("LOCALDOMAIN")) != NULL)
        (void)strncpy(_res.defdname, cp, sizeof(_res.defdname));
#endif DEBUG
    _res.options |= RES_INIT;
    return(0);
}
