/*      @(#)types.h 1.1 86/09/24 SMI      */

/*
 * Rpc additions to <sys/types.h>
 */
#ifndef __TYPES_RPC_HEADER__
#define __TYPES_RPC_HEADER__

#define	bool_t	int
#define	enum_t	int
#define	FALSE	(0)
#define	TRUE	(1)
#define __dontcare__	-1
#ifndef NULL
#	define NULL 0
#endif

#ifndef KERNEL
extern char *malloc();
#define mem_alloc(bsize)	malloc(bsize)
#define mem_free(ptr, bsize)	free(ptr)
#include <sys/types.h>
#else
extern char *kmem_alloc();
#define mem_alloc(bsize)	kmem_alloc((u_int)bsize)
#define mem_free(ptr, bsize)	kmem_free((caddr_t)(ptr), (u_int)(bsize))
#include "../h/types.h"
#endif

#endif !__TYPES_RPC_HEADER__
