/*	@(#)rpc.h 1.1 86/09/24 SMI	*/

/*
 * rpc.h, Just includes the billions of rpc header files necessary to 
 * do remote procedure calling.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */
#ifndef __RPC_HEADER__
#define __RPC_HEADER__

#include <rpc/types.h>		/* some typedefs */
#include <netinet/in.h>

/* external data representation interfaces */
#include <rpc/xdr.h>		/* generic (de)serializer */

/* Client side only authentication */
#include <rpc/auth.h>		/* generic authenticator (client side) */

/* Client side (mostly) remote procedure call */
#include <rpc/clnt.h>		/* generic rpc stuff */

/* semi-private protocol headers */
#include <rpc/rpc_msg.h>	/* protocol for rpc messages */
#include <rpc/auth_unix.h>	/* protocol for unix style cred */

/* Server side only remote procedure callee */
#include <rpc/svc.h>		/* service manager and multiplexer */
#include <rpc/svc_auth.h>	/* service side authenticator */

#endif !__RPC_HEADER__
