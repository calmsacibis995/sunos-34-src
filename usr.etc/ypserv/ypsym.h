/*	@(#)ypsym.h 1.1 86/09/25 Copyr 1985 Sun Microsystems, Inc	*/

/*
 * This contains symbol and structure definitions for modules in the yellow
 * pages server.  
 */

#include <dbm.h>			/* Pull this in first */
#define DATUM
extern void dbmclose();			/* Refer to dbm routine not in dbm.h */
#ifdef NULL
#undef NULL				/* Remove dbm.h's definition of NULL */
#endif
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>

#define __YP_PATH_PREFIX "/etc/yp"
#define YPDBPATH_LENGTH sizeof(__YP_PATH_PREFIX)
#define ORDER_KEY "YP_LAST_MODIFIED"
#define ORDER_KEY_LENGTH (sizeof(ORDER_KEY) - 1)
#define MAX_ASCII_ORDER_NUMBER_LENGTH 10
#define MASTER_KEY "YP_MASTER_NAME"
#define MASTER_KEY_LENGTH (sizeof(MASTER_KEY) - 1)
#define MAX_MASTER_NAME 256
#define INPUT_FILE "YP_INPUT_FILE"
#define INPUT_FILE_LENGTH (sizeof(INPUT_FILE) - 1)

#ifndef YPXFR_PROC
#define YPXFR_PROC "/etc/yp/ypxfr"
#endif
#ifndef YPPUSH_PROC
#define YPPUSH_PROC "/etc/yp/yppush"
#endif

typedef void (*PFV)();
typedef int (*PFI)();
typedef unsigned int (*PFU)();
typedef long int (*PFLI)();
typedef unsigned long int (*PFULI)();
typedef short int (*PFSI)();
typedef unsigned short int (*PFUSI)();

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef NULL
#undef NULL
#define NULL 0
#endif

#define YPINTERTRY_TIME 10		/* Secs between tries for peer bind */
#define YPTOTAL_TIME 30			/* Total secs until timeout */
#define YPNOPORT ((unsigned short) 0)	/* Out-of-range port value */

/* External refs to cells and functions outside of the yp */

extern int errno;
extern int svc_fds;
extern char *malloc();
extern char *strcpy();
extern char *strcat();
extern long atol();

/* External refs to yp server data structures */

extern bool ypinitialization_done;
extern char ypdbpath[];
extern int ypdbpath_length;
extern struct timeval ypintertry;
extern struct timeval yptimeout;
extern char myhostname[MAX_MASTER_NAME + 1];
extern char order_key[];
extern char master_key[];

/* External refs to yp server-only functions */

extern bool ypcheck_map_existence();
extern bool ypset_current_map();
extern void ypclr_current_map();
extern bool ypbind_to_named_server();
extern void ypmkfilename();
extern int yplist_maps();


