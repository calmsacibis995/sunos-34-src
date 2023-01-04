#ifndef lint
static char sccsid[] = "@(#)clnt_perror.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

/*
 * clnt_perror.c
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 */
#ifndef KERNEL
#include <stdio.h>
#endif

#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>

#ifndef KERNEL
extern char *sys_errlist[];
extern char *sprintf();
static char *auth_errmsg();
#endif

extern char *strcpy();
static char *rpc_errmsg();


#ifndef KERNEL
/*
 * Print reply error info
 */
char *
clnt_sperror(rpch, s)
	CLIENT *rpch;
	char *s;
{
	struct rpc_err e;
	void clnt_perrno();
	char *err;
	static char buf[100];
	char *str = buf;

	CLNT_GETERR(rpch, &e);

	(void) sprintf(str, "%s: ", s);  
	str += strlen(str);

	(void) strcpy(str, clnt_sperrno(e.re_status));  
	str += strlen(str);

	switch (e.re_status) {
	case RPC_SUCCESS:
	case RPC_CANTENCODEARGS:
	case RPC_CANTDECODERES:
	case RPC_TIMEDOUT:     
	case RPC_PROGUNAVAIL:
	case RPC_PROCUNAVAIL:
	case RPC_CANTDECODEARGS:
	case RPC_SYSTEMERROR:
	case RPC_UNKNOWNHOST:
	case RPC_PMAPFAILURE:
	case RPC_PROGNOTREGISTERED:
	case RPC_FAILED:
		break;

	case RPC_CANTSEND:
	case RPC_CANTRECV:
		(void) sprintf(str, "; errno = %s",
		    sys_errlist[e.re_errno]); 
		str += strlen(str);
		break;

	case RPC_VERSMISMATCH:
		(void) sprintf(str,
			"; low version = %lu, high version = %lu", 
			e.re_vers.low, e.re_vers.high);
		str += strlen(str);
		break;

	case RPC_AUTHERROR:
		err = auth_errmsg(e.re_why);
		(void) sprintf(str,"; why = ");
		str += strlen(str);
		if (err != NULL) {
			(void) sprintf(str, "%s",err);
		} else {
			(void) sprintf(str,
				"(unknown authentication error - %d)",
				(int) e.re_why);
		}
		str += strlen(str);
		break;

	case RPC_PROGVERSMISMATCH:
		(void) sprintf(str, 
			"; low version = %lu, high version = %lu", 
			e.re_vers.low, e.re_vers.high);
		str += strlen(str);
		break;

	default:	/* unknown */
		(void) sprintf(str, 
			"; s1 = %lu, s2 = %lu", 
			e.re_lb.s1, e.re_lb.s2);
		str += strlen(str);
		break;
	}
	(void) sprintf(str, "\n");
	return(buf);
}

void
clnt_perror(rpch, s)
	CLIENT *rpch;
	char *s;
{
	(void) fprintf(stderr,"%s",clnt_sperror(rpch,s));
}

#endif /* ! KERNEL */

/*
 * This interface for use by clntrpc
 */
char *
clnt_sperrno(num)
	enum clnt_stat num;
{
	char *rpcerror;
	static char prepend[] = "RPC: ";
	static char str[100];
	char *p;

	rpcerror = rpc_errmsg(num);
	(void) strcpy(str,prepend);
	p = &str[strlen(str)];
	if (rpcerror != NULL) {
		(void) strcpy(p,rpcerror);
	} else {
#ifndef KERNEL
		(void) sprintf(p, "(unknown error code - %d)",(int) num);
#else
		(void) strcpy(p, "(unknown error code)");
#endif
	}
	return(str);
}


#ifndef KERNEL
void
clnt_perrno(num)
	enum clnt_stat num;
{
	(void) fprintf(stderr,"%s",clnt_sperrno(num));
}
	
/*
 * A handle on why an rpc creation routine failed (returned NULL.)
 */
struct rpc_createerr rpc_createerr;

void
clnt_pcreateerror(s)
	char *s;
{

	(void) fprintf(stderr, "%s: ", s);
	clnt_perrno(rpc_createerr.cf_stat);
	switch (rpc_createerr.cf_stat) {
	case RPC_PMAPFAILURE:
		(void) fprintf(stderr, " - ");
		clnt_perrno(rpc_createerr.cf_error.re_status);
		break;

	case RPC_SYSTEMERROR:
		(void) fprintf(stderr, 
			" - %s", sys_errlist[rpc_createerr.cf_error.re_errno]);
		break;
	}
	(void) fprintf(stderr, "\n");
}
#endif

struct rpc_errtab {
	enum clnt_stat status;
	char *message;
};

static struct rpc_errtab  rpc_errlist[] = {
	{ RPC_SUCCESS, 
		"Success" }, 
	{ RPC_CANTENCODEARGS, 
		"Can't encode arguments" },
	{ RPC_CANTDECODERES, 
		"Can't decode result" },
	{ RPC_CANTSEND, 
		"Unable to send" },
	{ RPC_CANTRECV, 
		"Unable to receive" },
	{ RPC_TIMEDOUT, 
		"Timed out" },
	{ RPC_VERSMISMATCH, 
		"Incompatible versions of RPC" },
	{ RPC_AUTHERROR, 
		"Authentication error" },
	{ RPC_PROGUNAVAIL, 
		"Program unavailable" },
	{ RPC_PROGVERSMISMATCH, 
		"Program/version mismatch" },
	{ RPC_PROCUNAVAIL, 
		"Procedure unavailable" },
	{ RPC_CANTDECODEARGS, 
		"Server can't decode arguments" },
	{ RPC_SYSTEMERROR, 
		"Remote system error" },
	{ RPC_UNKNOWNHOST, 
		"Unknown host" },
	{ RPC_PMAPFAILURE, 
		"Port mapper failure" },
	{ RPC_PROGNOTREGISTERED, 
		"Program not registered"},
	{ RPC_FAILED, 
		"Failed (unspecified error)"}
};

static char *
rpc_errmsg(stat)
	enum clnt_stat stat;
{
	int i;
	
	for (i = 0; i < sizeof(rpc_errlist)/sizeof(struct rpc_errtab); i++) {
		if (rpc_errlist[i].status == stat) {
			return(rpc_errlist[i].message);
		}
	}
	return(NULL);
}

#ifndef KERNEL
struct auth_errtab {
	enum auth_stat status;	
	char *message;
};

static struct auth_errtab auth_errlist[] = {
	{ AUTH_OK,
		"Authentication OK" },
	{ AUTH_BADCRED,
		"Invalid client credential" },
	{ AUTH_REJECTEDCRED,
		"Server rejected credential" },
	{ AUTH_BADVERF,
		"Invalid client verifier" },
	{ AUTH_REJECTEDVERF,
		"Server rejected verifier" },
	{ AUTH_TOOWEAK,
		"Client credential too weak" },
	{ AUTH_INVALIDRESP,
		"Invalid server verifier" },
	{ AUTH_FAILED,
		"Failed (unspecified error)" },
};

static char *
auth_errmsg(stat)
	enum auth_stat stat;
{
	int i;
	
	for (i = 0; i < sizeof(auth_errlist)/sizeof(struct auth_errtab); i++) {
		if (auth_errlist[i].status == stat) {
			return(auth_errlist[i].message);
		}
	}
	return(NULL);
}
#endif /* ! KERNEL */
