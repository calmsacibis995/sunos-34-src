/*      rpcinfo.c     1.4     87/01/07     */

/*
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

/*
 * rpcinfo: ping a particular rpc program
 *     or dump the portmapper
 */

#include <rpc/rpc.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#include <ctype.h>

#define MAXHOSTLEN 256

#define	MAX_VERS	4294967295

main(argc, argv)
	char **argv;
{
	if (argc < 2) {
		usage();
		exit(1);
	}
	if (argv[1][0] == '-') {
		switch(argv[1][1]) {
			case 't':
				tcpping(argc-1, argv+1);
				break;
			case 'p':
				pmapdump(argc-1, argv+1);
				break;
			case 'u':
				udpping(argc-1, argv+1);
				break;
			default:
				usage();
				exit(1);
				break;
		}
	}
	else
		usage();
}
		
udpping(argc, argv)
	char **argv;
{
	struct timeval to;
	struct sockaddr_in addr;
	enum clnt_stat rpc_stat;
	CLIENT *client;
	u_long prognum, vers, minvers, maxvers;
	int sock = RPC_ANYSOCK;
	struct hostent *hp;
	struct rpcent *rpc;
	struct rpc_err rpcerr;
	int failure;
    
	if (argc < 3 || argc > 4) {
		usage();
		exit(1);
	}
	if (isalpha(argv[2][0])) {
		rpc = getrpcbyname(argv[2]);
		if (rpc == NULL) {
			fprintf(stderr, "%s is unknown name\n", argv[2]);
			exit(1);
		}
		prognum = rpc->r_number;
	}
	else
		prognum = atoi(argv[2]);
	if ((hp = gethostbyname(argv[1])) == NULL) {
	    fprintf(stderr, "can't find %s\n", argv[1]);
	    exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = *(int *)hp->h_addr;
	failure = 0;
	if (argc == 3) {
		/*
		 * A call to version 0 should fail with a program/version
		 * mismatch, and give us the range of versions supported.
		 */
		addr.sin_port = 0;
		to.tv_sec = 5;
		to.tv_usec = 0;
		if ((client = clntudp_create(&addr, prognum, 0,
		    to, &sock)) == NULL) {
			clnt_pcreateerror("rpcinfo");
			printf("program %lu is not available\n",
			    prognum);
			exit(1);
		}
		to.tv_sec = 10;
		to.tv_usec = 0;
		rpc_stat = clnt_call(client, NULLPROC, xdr_void, (char *)NULL,
		    xdr_void, (char *)NULL, to);
		if (rpc_stat == RPC_PROGVERSMISMATCH) {
			clnt_geterr(client, &rpcerr);
			minvers = rpcerr.re_vers.low;
			maxvers = rpcerr.re_vers.high;
		} else if (rpc_stat == RPC_SUCCESS) {
			/*
			 * Oh dear, it DOES support version 0.
			 * Let's try version MAX_VERS.
			 */
			addr.sin_port = 0;
			to.tv_sec = 5;
			to.tv_usec = 0;
			if ((client = clntudp_create(&addr, prognum, MAX_VERS,
			    to, &sock)) == NULL) {
				clnt_pcreateerror("rpcinfo");
				printf("program %lu version %lu is not available\n",
				    prognum, MAX_VERS);
				exit(1);
			}
			to.tv_sec = 10;
			to.tv_usec = 0;
			rpc_stat = clnt_call(client, NULLPROC, xdr_void,
			    (char *)NULL, xdr_void, (char *)NULL, to);
			if (rpc_stat == RPC_PROGVERSMISMATCH) {
				clnt_geterr(client, &rpcerr);
				minvers = rpcerr.re_vers.low;
				maxvers = rpcerr.re_vers.high;
			} else if (rpc_stat == RPC_SUCCESS) {
				/*
				 * It also supports version MAX_VERS.
				 * Looks like we have a wise guy.
				 * OK, we give them information on all
				 * 4 billion versions they support...
				 */
				minvers = 0;
				maxvers = MAX_VERS;
			} else {
				(void) pstatus(client, prognum, MAX_VERS);
				exit(1);
			}
		} else {
			(void) pstatus(client, prognum, vers);
			exit(1);
		}
		clnt_destroy(client);
		for (vers = minvers; vers <= maxvers; vers++) {
			addr.sin_port = 0;
			to.tv_sec = 5;
			to.tv_usec = 0;
			if ((client = clntudp_create(&addr, prognum, vers,
			    to, &sock)) == NULL) {
				clnt_pcreateerror("rpcinfo");
				printf("program %lu version %lu is not available\n",
				    prognum, vers);
				exit(1);
			}
			to.tv_sec = 10;
			to.tv_usec = 0;
			rpc_stat = clnt_call(client, NULLPROC, xdr_void,
			    (char *)NULL, xdr_void, (char *)NULL, to);
			if (pstatus(client, prognum, vers) < 0)
				failure = 1;
			clnt_destroy(client);
		}
	}
	else {
		vers = atoi(argv[3]);
		to.tv_sec = 5;
		to.tv_usec = 0;
		if ((client = clntudp_create(&addr, prognum, vers,
		    to, &sock)) == NULL) {
			clnt_pcreateerror("rpcinfo");
			printf("program %lu version %lu is not available\n",
			    prognum, vers);
			exit(1);
		}
		to.tv_sec = 10;
		to.tv_usec = 0;
		rpc_stat = clnt_call(client, 0, xdr_void, (char *)NULL,
		    xdr_void, (char *)NULL, to);
		if (pstatus(client, prognum, vers) < 0)
			failure = 1;
	}
	if (failure)
		exit(1);
}

tcpping(argc, argv)
	int argc;
	char **argv;
{
	struct timeval to;
	struct sockaddr_in addr;
	enum clnt_stat rpc_stat;
	CLIENT *client;
	u_long prognum, vers, minvers, maxvers;
	int sock = RPC_ANYSOCK;
	struct hostent *hp;
	struct rpcent *rpc;
	struct rpc_err rpcerr;
	int failure;

	if (argc < 3 || argc > 4) {
		usage();
		exit(1);
	}
	if (isalpha(argv[2][0])) {
		rpc = getrpcbyname(argv[2]);
		if (rpc == NULL) {
			fprintf(stderr, "%s is unknown name\n", argv[2]);
			exit(1);
		}
		prognum = rpc->r_number;
	}
	else
		prognum = atoi(argv[2]);
	if ((hp = gethostbyname(argv[1])) == NULL) {
	    fprintf(stderr, "can't find %s\n", argv[1]);
	    exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = *(int *)hp->h_addr;
	failure = 0;
	if (argc == 3) {
		/*
		 * A call to version 0 should fail with a program/version
		 * mismatch, and give us the range of versions supported.
		 */
		addr.sin_port = 0;
		if ((client = clnttcp_create(&addr, prognum, 0,
		    &sock, 0, 0)) == NULL) {
			clnt_pcreateerror("rpcinfo");
			printf("program %lu is not available\n",
			    prognum, vers);
			exit(1);
		}
		to.tv_sec = 10;
		to.tv_usec = 0;
		rpc_stat = clnt_call(client, NULLPROC, xdr_void, (char *)NULL,
		    xdr_void, (char *)NULL, to);
		if (rpc_stat == RPC_PROGVERSMISMATCH) {
			clnt_geterr(client, &rpcerr);
			minvers = rpcerr.re_vers.low;
			maxvers = rpcerr.re_vers.high;
		} else if (rpc_stat == RPC_SUCCESS) {
			/*
			 * Oh dear, it DOES support version 0.
			 * Let's try version MAX_VERS.
			 */
			addr.sin_port = 0;
			if ((client = clnttcp_create(&addr, prognum, MAX_VERS,
			    &sock, 0, 0)) == NULL) {
				clnt_pcreateerror("rpcinfo");
				printf("program %lu version %lu is not available\n",
				    prognum, MAX_VERS);
				exit(1);
			}
			to.tv_sec = 10;
			to.tv_usec = 0;
			rpc_stat = clnt_call(client, NULLPROC, xdr_void,
			    (char *)NULL, xdr_void, (char *)NULL, to);
			if (rpc_stat == RPC_PROGVERSMISMATCH) {
				clnt_geterr(client, &rpcerr);
				minvers = rpcerr.re_vers.low;
				maxvers = rpcerr.re_vers.high;
			} else if (rpc_stat == RPC_SUCCESS) {
				/*
				 * It also supports version MAX_VERS.
				 * Looks like we have a wise guy.
				 * OK, we give them information on all
				 * 4 billion versions they support...
				 */
				minvers = 0;
				maxvers = MAX_VERS;
			} else {
				(void) pstatus(client, prognum, MAX_VERS);
				exit(1);
			}
		} else {
			(void) pstatus(client, prognum, vers);
			exit(1);
		}
		clnt_destroy(client);
		for (vers = minvers; vers <= maxvers; vers++) {
			addr.sin_port = 0;
			if ((client = clnttcp_create(&addr, prognum, vers,
			    &sock, 0, 0)) == NULL) {
				clnt_pcreateerror("rpcinfo");
				printf("program %lu version %lu is not available\n",
				    prognum, vers);
				exit(1);
			}
			to.tv_usec = 0;
			to.tv_sec = 10;
			rpc_stat = clnt_call(client, 0, xdr_void, (char *)NULL,
			    xdr_void, (char *)NULL, to);
			if (pstatus(client, prognum, vers) < 0)
				failure = 1;
			clnt_destroy(client);
		}
	}
	else {
		vers = atoi(argv[3]);
		addr.sin_port = 0;
		if ((client = clnttcp_create(&addr, prognum, vers, &sock,
		    0, 0)) == NULL) {
			clnt_pcreateerror("rpcinfo");
			printf("program %lu version %lu is not available\n",
			    prognum, vers);
			exit(1);
		}
		to.tv_usec = 0;
		to.tv_sec = 10;
		rpc_stat = clnt_call(client, 0, xdr_void, (char *)NULL,
		    xdr_void, (char *)NULL, to);
		if (pstatus(client, prognum, vers) < 0)
			failure = 1;
	}
	if (failure)
		exit(1);
}

/*
 * This routine should take a pointer to an "rpc_err" structure, rather than
 * a pointer to a CLIENT structure, but "clnt_perror" takes a pointer to
 * a CLIENT structure rather than a pointer to an "rpc_err" structure.
 * As such, we have to keep the CLIENT structure around in order to print
 * a good error message.
 */
int
pstatus(client, prognum, vers)
	register CLIENT *client;
	u_long prognum;
	u_long vers;
{
	struct rpc_err rpcerr;

	clnt_geterr(client, &rpcerr);
	if (rpcerr.re_status != RPC_SUCCESS) {
		clnt_perror(client, "rpcinfo");
		printf("program %lu version %lu is not available\n",
		    prognum, vers);
		return (-1);
	} else {
		printf("program %lu version %lu ready and waiting\n",
		    prognum, vers);
		return (0);
	}
}

pmapdump(argc, argv)
	int argc;
	char **argv;
{
	struct sockaddr_in server_addr;
	struct hostent *hp;
	struct pmaplist *head = NULL;
	char hoststr[MAXHOSTLEN];
	int socket = RPC_ANYSOCK;
	struct timeval minutetimeout;
	char *hostnm;
	register CLIENT *client;
	enum clnt_stat rpc_stat;
	struct rpcent *rpc;
	
	if (argc > 2) {
		usage();
		exit(1);
	}
	if (argc == 2) {
		hostnm = argv[1];
		if ((hp = gethostbyname(hostnm)) == NULL) {
			fprintf(stderr, "cannot get addr for '%s'\n", hostnm);
			exit(0);
		}
		bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr,hp->h_length);
	} else {
		if (hp = gethostbyname("localhost"))
			bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr,
			    hp->h_length);
		else
			server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	}

	server_addr.sin_family = AF_INET;
	minutetimeout.tv_sec = 60;
	minutetimeout.tv_usec = 0;
	server_addr.sin_port = htons(PMAPPORT);
	if ((client = clnttcp_create(&server_addr, PMAPPROG,
	    PMAPVERS, &socket, 50, 500)) == NULL) {
		clnt_pcreateerror("rpcinfo: can't contact portmapper");
		exit(1);
	}
	if ((rpc_stat = clnt_call(client, PMAPPROC_DUMP, xdr_void, NULL,
	    xdr_pmaplist, &head, minutetimeout)) != RPC_SUCCESS) {
		fprintf(stderr, "rpcinfo: can't contact portmapper: ");
		clnt_perror(client, "rpcinfo");
		exit(1);
	}
	if (head == NULL) {
		printf("No remote programs registered.\n");
	} else {
		printf("   program vers proto   port\n");
		for (; head != NULL; head = head->pml_next) {
			printf("%10ld%5ld",
			    head->pml_map.pm_prog,
			    head->pml_map.pm_vers);
			if (head->pml_map.pm_prot == IPPROTO_UDP)
				printf("%6s",  "udp");
			else if (head->pml_map.pm_prot == IPPROTO_TCP)
				printf("%6s", "tcp");
			else
				printf("%6ld",  head->pml_map.pm_prot);
			printf("%7ld",  head->pml_map.pm_port);
			rpc = getrpcbynumber(head->pml_map.pm_prog);
			if (rpc)
				printf("  %s\n", rpc->r_name);
			else
				printf("\n");
		}
	}
}

usage()
{
	fprintf(stderr, "Usage: rpcinfo -u host prognum [versnum]\n");
	fprintf(stderr, "       rpcinfo -t host prognum [versnum]\n");
	fprintf(stderr, "       rpcinfo -p [host]\n");
}
